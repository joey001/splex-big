	/*
 * splex.cpp
 *
 *  Created on: Nov 13, 2014
 *      Author: zhou
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <sys/time.h>
#include "graph.h"

/**
 * Bucket structure to quickly find the vertices with given degree
 * vertex with degree v are listed slots[i]
 */
typedef struct BKElem{
	int vid;
	struct BKElem *prev;
	struct BKElem *next;
}BKElem;

typedef struct{
	BKElem* first_elem;
	int num;
}BKSlots;
typedef BKSlots *Bucket;

typedef struct VInfo{
	int vid;
	int inside; 	//inside S or not
	int in_deg;		//the degree to set S
	int *address;	//The position of vertex v in adj_in is adress[v], so adj_in[adress[v]] is vertex v
	int *adj_in_set;	//the adjacent vertices in C
	BKElem *pos_bkt;	//the position in bucket
	VInfo *prev_in;		// the previous vertex in C
	VInfo *next_in;		//the next vertex in C
}VInfo;

typedef struct Plex{
	int *verlist;
	int size;
}Plex;
typedef struct LogStructure{
	float iave_time_us;
	float total_time;
	float best_found_time;

	int total_iters;
	int best_found_iter;
	int total_restart;
}Log;

#define DEBUG 0
#define TABU 1
#define DETAIL 0
#define PART_OUT_BUCKT 1
#define GRAPH_VERTICES_LIMIT 2000
#define MAX_VAL 9999999

//char* graph_file_name = "/home/zhou/workspace/splex-unfix/graph_test.clq";
char graph_file_name[1000]="/home/zhou/splex/benchmarks/dimacs/MANN_a27.clq";
int param_s = 3;
int param_max_iteration = 100000000;
int param_runcnt = 10;
int param_best = 351;
int param_cycle = 10000;

FILE *frec;
GraphType *org_g;
GraphType *prun_g;
GraphType *cpl_g;
//Bucket in_bkt;
Bucket out_bkt;
int bk_max = param_s;

VInfo *vinfo;

VInfo *in_set; 	/*points to the last vertex in the current plex*/
int in_size; /*size of  plex*/
VInfo *out_set;
int out_size;

//int *of_address;	/*The position of each vertex in overflow list*/
//int *of_list;	/*A list of over flow vertex*/
int of_size;	/*The number of over flow vertex*/

Plex cur_best;	/*current best plex*/
Plex final_best;
Log cur_log;


int *tabu_add;
int *conf_change;


int maxAdjValue = -1;
int vmax = -1;
int** inAdjMatrix;
int *inAdjSize;



#define Adj_in(v,ith) (vinfo[v].adj_in_set[ith])
#define Degree(v) ((cpl_g->adj_list[v]).degree)
#define Min(a, b) ((a)<(b)?(a):(b))

void allocateMemory(){
	int vnum = cpl_g->v_num;
	vinfo = new VInfo[vnum];

	for (int idx = 0; idx < cpl_g->v_num; idx++){
		vinfo[idx].address = new int[vnum];
		vinfo[idx].adj_in_set = new int[vnum];
		vinfo[idx].pos_bkt = new BKElem;
	}

	inAdjMatrix = new int*[vnum];
	inAdjSize = new int[vnum];
	for (int idx = 0; idx < vnum; idx++){
		inAdjMatrix[idx] = new int[vnum];
	}
	/*Allocate two bucket lists*/
//	in_bkt = new BKSlots[vnum];
	out_bkt = new BKSlots[vnum];

	/*Allocate a tabu list*/
	tabu_add = new int[vnum];

	/*Configuration checking */
	conf_change = new int[vnum];

	cur_best.size = 0;
	cur_best.verlist = new int[vnum];

	of_size = 0;
}

/*******************************Utility******************************************/
/**
 * Generate a random list
 */
void randomizeList(int *randlist, int len){
	int idx = 0;
	assert(randlist != NULL);

	for (idx = 0; idx < len; idx++){
		randlist[idx] = idx;
	}
	for (idx = 0; idx < len; idx++){
		int randid = rand() % len;
		int tmp = randlist[idx];
		randlist[idx] = randlist[randid];
		randlist[randid] = tmp;
	}
}

/**
 * Addd an element to current bucket
 */
void addBKElem(Bucket bkt, int slot, BKElem *elem){
	BKElem *old_first = bkt[slot].first_elem;
	elem->next = old_first;
	elem->prev = NULL;
	if (old_first != NULL){
		old_first->prev = elem;
	}
	bkt[slot].first_elem = elem;
	bkt[slot].num++;
}

void removeBKElem(Bucket bkt, int slot, BKElem *elem){
	//the elem must locates in bkt[slot]
//	assert(vinfo[elem->vid].in_deg == slot); //根据本身逻辑作出ASSERTATION
	BKElem *prev = elem->prev;
	if (prev != NULL){
		prev->next = elem->next;
	}else{ //prev is NULL pointer
		bkt[slot].first_elem = elem->next;
	}
	BKElem *next = elem->next;
	if (next != NULL){
		next->prev = prev;
	}
	//Ensure safty
	elem->prev = NULL;
	elem->next = NULL;
	bkt[slot].num--;
}

int checkPlex(Plex *p){
	int *indeg = new int[p->size];
	int rt = 1;
	memset(indeg, 0, sizeof(int) * p->size);
	for (int i = 0; i < p->size; i++){
		int vi = p->verlist[i];
		for (int j = i+1; j < p->size; j++){
			int vj = p->verlist[j];
			if(cpl_g->matrix[vi][vj] != 0){
				indeg[i]++;
				indeg[j]++;
			}
			if (indeg[i] > param_s - 1){
				fprintf(stderr, "Vertex %d has inside degree %d\n", vi, indeg[i]);
				rt = 0;
			}
			if (indeg[j] > param_s - 1){
				fprintf(stderr, "Vertex %d has inside degree %d\n", vi, indeg[j]);
				rt = 0;
			}
		}
	}
	delete[] indeg;
	return rt;
}

void showCurrentSoution(FILE *fout){
	VInfo *pvinfo = in_set;
	int cnt=0;
	fprintf(fout, "Inside vertex(%d):",in_size);
	while (pvinfo != NULL){
		if (pvinfo->in_deg == param_s -1)
			fprintf(fout, " %d(st%d) ", pvinfo->vid, pvinfo->in_deg);
		else
			fprintf(fout, " %d(%d) ", pvinfo->vid, pvinfo->in_deg);
		pvinfo = pvinfo->prev_in;
		cnt++;
	}
	assert(cnt == in_size);
	fprintf(fout, "\n");
}


void showBucket(FILE *fout, Bucket bkt){
	fprintf(frec, "out bucket:\n");
	for (int i = 0; i< cpl_g->v_num; i++){
		if (bkt[i].num == 0) continue;

		fprintf(fout, "slot %d[%d]:", i, bkt[i].num);
		int n = 0;
		BKElem *p = bkt[i].first_elem;
		while (p != NULL){
			n++;
			fprintf(fout, "%d ",p->vid );
			p = p->next;
		}
		fprintf(fout, "\n");
		assert(bkt[i].num == n);
	}
	fprintf(fout, "\n");
}

#if (DEBUG)
/**
 * Print the summarry of current contex
 */
void showCurrentContex(FILE *fout, int iter){
	fprintf(fout, "------------------------ITERATION %d---------------------------\n",iter);
	fprintf(fout,"current solution size %d ofsize %d\n",in_size, of_size);
	showCurrentSoution(fout);
	fprintf(fout, "The two bucket\n");
//	showBucket(fout, in_bkt);
	showBucket(fout, out_bkt);

	fprintf(fout, "The tabu vertices  vertex(iteration left)\n");
	for (int i = 0; i < cpl_g->v_num; i++){
		if (tabu_add[i] > iter){
			fprintf(fout, "%d(%d) ", i, tabu_add[i]-iter);
		}
	}
	fprintf(fout,"\n");
}

#else
#define showOverflowVertices(fout)
#define showCurrentContex(fout, iter)
#endif
/*---------------------------------------------Solve functions--------------------------------------------------------------*/

void clearlog(){
	cur_log.total_iters = 0;
	cur_log.best_found_iter = 0;
	cur_log.best_found_time = 0.0f;
	cur_log.total_time = 0.0f;
	cur_log.iave_time_us = 0.0f;
	cur_log.total_restart = 0;
}


/**
 * 初始化数据结构
 */
void initializeSearch(){
	int vnum = cpl_g->v_num;

	for (int idx = 0; idx < vnum; idx++){
		vinfo[idx].vid = idx;
		vinfo[idx].inside = 0;
		vinfo[idx].in_deg = 0;

		(vinfo[idx].pos_bkt)->vid = idx;
		(vinfo[idx].pos_bkt)->prev = NULL;
		(vinfo[idx].pos_bkt)->next = NULL;
	}

	/*clear two help bucket*/
	for (int idx = 0; idx < vnum; idx++){
		out_bkt[idx].num = 0;
		out_bkt[idx].first_elem = NULL;
	}
	/*Add bucket elements randomly*/
	int *randlist = new int[vnum];
	randomizeList(randlist, vnum);
	for (int i = 0; i < vnum; i++){
		BKElem *pe = vinfo[randlist[i]].pos_bkt;
		addBKElem(out_bkt, 0, pe);
	}
	delete[] randlist;

	/*initialize C*/
	in_set = NULL;
	in_size = 0;

	cur_best.size = 0;

	of_size = 0;

	/*Initialize tabu_add*/
	memset(tabu_add, 0, sizeof(int) * vnum);
	for (int i = 0; i < vnum; i++){
		conf_change[i] = 1;
	}
}


void increaseInsideDegree(int v, int vjoin){
	int deg = vinfo[v].in_deg;

	vinfo[v].adj_in_set[deg] = vjoin;
	vinfo[v].address[vjoin] = deg;
	vinfo[v].in_deg++;
}

void decreaseInsideDegree(int v, int vremove){
	int deg = vinfo[v].in_deg;
	int rm_pos = vinfo[v].address[vremove];

	int last = vinfo[v].adj_in_set[deg-1];
	vinfo[v].adj_in_set[rm_pos] = last;
	vinfo[v].address[last] = rm_pos;
	vinfo[v].in_deg--;
}

void expand(int v){
	assert(vinfo[v].inside == 0);

	vinfo[v].inside = 1;
	vinfo[v].prev_in = in_set;
	vinfo[v].next_in = NULL;
	if (in_set != NULL)
		in_set->next_in = &(vinfo[v]);
	in_set = &(vinfo[v]);
	in_size++;

	if (vinfo[v].in_deg >= param_s){
		of_size++;
	}
#if (PART_OUT_BUCKT)
	if (vinfo[v].in_deg <= bk_max + 1)
#endif
		removeBKElem(out_bkt, vinfo[v].in_deg, vinfo[v].pos_bkt);

	Arc *parc = cpl_g->adj_list[v].first_arc;
	while (parc != NULL){
		int vi = parc->adjvex;
		if (vinfo[vi].inside){
			//removeBKElem(in_bkt, vinfo[vi].in_deg, vinfo[vi].pos_bkt);
			//addBKElem(in_bkt, vinfo[vi].in_deg+1, vinfo[vi].pos_bkt);
			increaseInsideDegree(vi, v);
			if (vinfo[vi].in_deg == param_s){
				of_size++;
			}
		}else{
			increaseInsideDegree(vi, v);
#if (PART_OUT_BUCKT)
			if (vinfo[vi].in_deg <= bk_max+1){ /*in_deg is still less than param_s+1*/
				removeBKElem(out_bkt, vinfo[vi].in_deg-1, vinfo[vi].pos_bkt);
				addBKElem(out_bkt, vinfo[vi].in_deg, vinfo[vi].pos_bkt);
			}else if (vinfo[vi].in_deg == bk_max +2)	/*in_deg becomes large than param_s+2*/
				removeBKElem(out_bkt, vinfo[vi].in_deg-1, vinfo[vi].pos_bkt);
#else
			removeBKElem(out_bkt, vinfo[vi].in_deg-1, vinfo[vi].pos_bkt);
			addBKElem(out_bkt, vinfo[vi].in_deg, vinfo[vi].pos_bkt);
#endif
		}
		conf_change[vi] = 1;
		parc = parc->next_arc;
	}
	/*DEBUG check*/
#if (DEBUG)
	fprintf(frec ,"add %d\n",v);
#endif
}

void drop(int v){
	/*将节点从C中删除*/
	assert(vinfo[v].inside == 1);
	vinfo[v].inside = 0;
	if (vinfo[v].prev_in != NULL)
		(vinfo[v].prev_in)->next_in = vinfo[v].next_in;
	if (vinfo[v].next_in != NULL) /*v是C中最后一个节点*/
		(vinfo[v].next_in)->prev_in = vinfo[v].prev_in;
	else{
		in_set = vinfo[v].prev_in;
	}
	vinfo[v].prev_in = NULL;
	vinfo[v].next_in = NULL;
	in_size--;

	/*Move v from in_bucket to out_bucket*/
	//removeBKElem(in_bkt, vinfo[v].in_deg, vinfo[v].pos_bkt);
#if (PART_OUT_BUCKT)
	if (vinfo[v].in_deg <= bk_max+1)
#endif
		addBKElem(out_bkt, vinfo[v].in_deg, vinfo[v].pos_bkt);

	if (vinfo[v].in_deg >= param_s){ /*If v is in overflow list*/
		of_size--;
	}
	/*If v is droped change the state*/
	conf_change[v] = 0;

	Arc *parc = cpl_g->adj_list[v].first_arc;
	while (parc != NULL){
		int vi = parc->adjvex;
		if (vinfo[vi].inside){
			//removeBKElem(in_bkt, vinfo[vi].in_deg, vinfo[vi].pos_bkt);
			//addBKElem(in_bkt, vinfo[vi].in_deg-1, vinfo[vi].pos_bkt);
			decreaseInsideDegree(vi, v);
			if (vinfo[vi].in_deg == param_s-1){
				of_size--;
			}
		}else{ /*outside*/
			decreaseInsideDegree(vi, v);
#if (PART_OUT_BUCKT)
			if (vinfo[vi].in_deg == bk_max+1){
				addBKElem(out_bkt, vinfo[vi].in_deg, vinfo[vi].pos_bkt);
			}else if (vinfo[vi].in_deg < bk_max+1){
				removeBKElem(out_bkt, vinfo[vi].in_deg+1, vinfo[vi].pos_bkt);
				addBKElem(out_bkt, vinfo[vi].in_deg, vinfo[vi].pos_bkt);
			}
#else
			removeBKElem(out_bkt, vinfo[vi].in_deg+1, vinfo[vi].pos_bkt);
			addBKElem(out_bkt, vinfo[vi].in_deg, vinfo[vi].pos_bkt);
#endif
		}
		conf_change[vi] = 1;
		parc = parc->next_arc;
	}
#if (DEBUG)
	fprintf(frec, "drop %d\n",v);
#endif
}

/*
 * Save current solution to plex
 */
inline void saveCurentSolution(Plex *plex){
	plex->size = in_size;
#if (DETAIL)
	int cnt = 0;
	VInfo *pvinfo = in_set;
	while (pvinfo != NULL){
		plex->verlist[cnt] = pvinfo->vid;
		pvinfo = pvinfo->prev_in;
		cnt++;
	}
	assert(cnt == in_size);
#endif
}


/**
 * print the plex in f in ascending order
 */
void printPlex(FILE *f, Plex *plex){
	int *vmark = new int[cpl_g->v_num];

	fprintf(f, "size %d :", plex->size);

	memset(vmark, 0, sizeof(int) * cpl_g->v_num);
	for (int i = 0; i < plex->size; i++){
		vmark[plex->verlist[i]] = 1;
	}
	for (int i = 0; i < cpl_g->v_num; i++){
		if (vmark[i] == 1)
		fprintf(f, "%d ", i+1);
	}
	fprintf(f, "\n");
	delete[] vmark;
}

/*----------------------------------------Local search procedure------------------------------------*/

void initConnectTable(int iter){
	maxAdjValue = -1;
	vmax = -1;
	memset(inAdjSize , 0,  sizeof(int) * cpl_g->v_num);
}

int evaluateVertex(int iter, int vertex, int *inside){
	for (int i = 0; i < vinfo[vertex].in_deg; i++){
		int vin = Adj_in(vertex, i);
		if (inAdjSize[vin] >= 1){
			for (int i = 0; i < inAdjSize[vin];i++){
				/*Only when two vertices are not connected and their common inside neigbor is a saturated vertex*
				 * can the two vertices be possible switched inside at the same time*/
				if (!(cpl_g->matrix[vertex][inAdjMatrix[vin][i]])){
					*inside = vin;
					return inAdjMatrix[vin][i];
				}
			}
		}
		//此处还需要争对s>=2的情况仔细考虑比较
		if (vinfo[vin].in_deg == param_s - 1){
			inAdjMatrix[vin][inAdjSize[vin]++] = vertex;
		}
	}
	return -1;
}

void showConnectTable(){
	int vnum = cpl_g->v_num;
	for (int i = 0 ;i < vnum; i++){
		if (inAdjSize[i] > 0){
			printf("%d:",i);
			for (int j = 0; j < inAdjSize[i]; j++)
				printf("%d ",inAdjMatrix[i][j]);
			printf("\n");
		}
	}
}
void repairTabu(int fixed_v, int iter, int tabu_base){
	while (of_size > 0){
		int vchose;
		int largest = -1;
		for (int i = 0; i < vinfo[fixed_v].in_deg; i++){
			int adj_v_in = vinfo[fixed_v].adj_in_set[i];
			if (vinfo[adj_v_in].in_deg > largest){
				vchose = adj_v_in;
				largest = vinfo[adj_v_in].in_deg;
			}else if (vinfo[adj_v_in].in_deg == largest){
				if (rand() % 2 == 1)
					vchose = adj_v_in;
			}
		}
		drop(vchose);
		if (tabu_base > 0)
			tabu_add[vchose] = iter + tabu_base;
	}
}

/**
 * number if saturated inside neighbors of v,
 */
inline int sumSatuInside(int v){
	assert(vinfo[v].inside == 0);
	int sum = 0;
	for (int i = 0; i < vinfo[v].in_deg; i++){
		if (vinfo[Adj_in(v, i)].in_deg == param_s-1)
			sum++;
	}
	return sum;
}

/**
 * Find one to drop
 */
#if (PART_OUT_BUCKT)
int findPerturbated(int iter){
	int small = cpl_g->v_num;
	int best_add = -1;

	int thresh = rand() % 2 ? param_s+1 : param_s+2;
	for (int vi = 0; vi < cpl_g->v_num; vi++){
		if (!vinfo[vi].inside && vinfo[vi].in_deg >= thresh){
			if (tabu_add[vi] <= iter && vinfo[vi].in_deg <= small){
				small = vinfo[vi].in_deg;
				if (best_add == -1) best_add = vi;
				else if (Degree(vi) > Degree(best_add)){
					best_add = vi;
				}else if(Degree(vi) == Degree(best_add)){
					if (rand() % 2) best_add = vi;
				}
			}
		}
	}
	return best_add;
}
#else
int findAddCandConOvFlow(int iter, int thresh){
	int small = cpl_g->v_num;
	int best_add = -1;

	for (int vi = 0; vi < cpl_g->v_num; vi++){
		if (!vinfo[vi].inside && vinfo[vi].in_deg >= thresh){
			if (tabu_add[vi] <= iter && vinfo[vi].in_deg <= small){
				small = vinfo[vi].in_deg;
				if (best_add == -1) best_add = vi;
				else if (Degree(vi) > Degree(best_add)){
					best_add = vi;
				}else if(Degree(vi) == Degree(best_add)){
					if (rand() % 2) best_add = vi;
				}
			}
		}
	}
	return best_add;
}
#endif

int findDrop(){
	VInfo *p = in_set;
	int max_deg = 0;;
	int choice;
	while (p != NULL){
		if (Degree(p->vid) > max_deg) {
			max_deg = Degree(p->vid);
			choice = p->vid;
		}else if(Degree(p->vid) == max_deg && rand() % 2)
			choice = p->vid;
		p = p->prev_in;
	}
	return choice;

}
int localDirectTabuMove(int iter){
	int connect = 0;
	int inside_satu;
	int max_deg = -1;
	int plateauv = -1;

	initConnectTable(iter);
	showCurrentContex(frec, iter);
	int sum = 0;
	while(connect <= param_s){
		BKElem *pelem = out_bkt[connect].first_elem;
		sum += out_bkt[connect].num;
		while(pelem != NULL){
			int vcur = pelem->vid;
			/*connect < param_s*/
			if (connect == 0){
				/*vcur is not tabu and connect to non of the vertices inside, it can be directly
				 * added into the expanded set*/
				if (tabu_add[vcur] <= iter || in_size + 1 > cur_best.size){
						expand(vcur);
						return 1;
				}
			}else if (connect <= param_s){
				if (tabu_add[vcur] <= iter || in_size + 1 > cur_best.size){		//descent
					int satucon = sumSatuInside(vcur) ;
					if (satucon == 0 && connect < param_s){	/*connect to non of the saturated numbers*/
						expand(vcur);
						return 1;
					}else if (satucon == 1){	// satucon == 1
						int rt = evaluateVertex(iter, vcur, &inside_satu);
						if (-1 != rt){	/*+2  -1*/
							int base = out_bkt[connect].num; /*The size will change after switch*/
							expand(vcur);
							drop(inside_satu);
							tabu_add[inside_satu] = iter + 7 + rand() % base;
							if (of_size > 0) {
								repairTabu(vcur, iter, 7 + rand() % base);
							}
							return 1;
						}
					}
				}
				if (tabu_add[vcur] <= iter){
					if (Degree(vcur) > max_deg){
						max_deg =Degree(vcur) ;
						plateauv = vcur;
					}else if (Degree(vcur) == max_deg && rand() % 2){
						plateauv = vcur;
					}
				}
			}
			pelem = pelem->next;
		}
		connect++;
	}
	/*Plateau*/
	if (sum * 3 < cpl_g->v_num && plateauv != -1){
		expand(plateauv);
		if (of_size > 0)
			repairTabu(plateauv, iter, 10 + rand()%sum);
		return 2;
	}
	/*Descent*/
	int best_add = findPerturbated(iter);
	if (best_add != -1){
		expand(best_add);
		repairTabu(best_add, iter, 7);
		return 1;
	}
	return 0;
}

void createInitSolution(){
	int connect = 0;
	while (connect < 1){
		if (out_bkt[connect].num > 0){
			int addv =(out_bkt[connect].first_elem)->vid;
			expand(addv);
#if TABU
			//(addv, 0, 0);
#else
			repairConfig(addv);
#endif
		}
		else
			connect++;
	}
}
/**
 * max_unimprove, the maximum unimproved iterator
 * maxiter, the maximum iteration times
 */
void solve_tabu(int iter_per_start, int maxiter, Plex *final_best){
	int iter = 0;
	int end = 0;
	int restart = 0;
	final_best->size = 0;
#if (DEBUG)
	if (final_best->verlist == NULL)
		final_best->verlist = new int[cpl_g->v_num];
#endif
	clearlog();

	allocateMemory();
	clock_t start_time = clock();
	struct timeval prec_start;
	gettimeofday(&prec_start, NULL);
	while (!end){
		/*Initialize the data structure and creat a init solution*/
		initializeSearch();
		createInitSolution();
		saveCurentSolution(&cur_best);

		int no_improve_iter = 0;
		int tabu_iter = 0; 	//iteration counter in each restart
//		while (no_improve_iter < iter_per_start){
		while (tabu_iter < iter_per_start && (float)(clock() - start_time) / CLOCKS_PER_SEC < 6000){
#if (TABU)
			int move = localDirectTabuMove(iter);
#else
			int move = localConfigMove(iter);
#endif
			if (move == 0)
				break;
			if (in_size > cur_best.size){
				saveCurentSolution(&cur_best);
				no_improve_iter = 0;
			}else{
				no_improve_iter++;
			}
			/*Print information*/
			if (iter % 10000 == 0){
				printf("%d: current %d,current best %d, final best %d\n", iter, in_size,cur_best.size,
						final_best->size);
			}
			tabu_iter++;
			iter++;
			/*If global optimum is found or execeed the max iteration, stop*/
			if (cur_best.size >= param_best || iter >= maxiter){
				end = 1;
				break;
			}
		}
		restart++;
		if (cur_best.size > final_best->size){
			final_best->size = cur_best.size;
#if (DETAIL)
			memcpy(final_best->verlist, cur_best.verlist, sizeof(int) * cpl_g->v_num);
#endif
			cur_log.best_found_time = (float)(clock() - start_time) / CLOCKS_PER_SEC;
			cur_log.best_found_iter = iter;
		}
	}
	printf("best: %d in iteration %d\n",final_best->size, iter);
	cur_log.total_iters = iter;
	cur_log.total_time = (float)(clock() - start_time) / CLOCKS_PER_SEC;
	cur_log.total_restart = restart;
	struct timeval prec_end;
	gettimeofday(&prec_end, NULL);
	cur_log.iave_time_us = (float)(prec_end.tv_sec - prec_start.tv_sec ) / iter *1000000 +
			(float)(prec_end.tv_usec - prec_start.tv_usec) /iter;
}

/*Create a file to record the values*/
FILE *createRecordFile(){
	char path_cwd[FILENAME_MAX];
	char *graph_name = basename(graph_file_name);
	char file_name[FILENAME_MAX];

	getcwd(path_cwd, FILENAME_MAX);
	sprintf(file_name, "%s/%s.rec",path_cwd, graph_name);
	FILE *recfile = fopen(file_name, "a");
	return recfile;
}

void showUsage(){
	printf("splex [-f filename] [-s plex] [-r runcount] [-b bes_known]");
}
void readParameters(int argc, char **argv){
	for (int i = 1; i < argc; i+=2){
		if (argv[i][0] != '-' || argv[i][2] != 0){
			showUsage();
			exit(0);
		}else if (argv[i][1] == 'f'){
			strncpy(graph_file_name, argv[i+1],1000);
		}else if(argv[i][1] == 's'){
			param_s = atoi(argv[i+1]);
		}else if (argv[i][1] == 'r'){
			param_runcnt = atoi(argv[i+1]);
		}else if (argv[i][1] == 'b'){
			param_best = atoi(argv[i+1]);
		}else if (argv[i][1] == 'c'){
			param_cycle = atoi(argv[i+1]);
		}
	}
	/*check parameters*/
	if (strlen(graph_file_name) == 0){
		fprintf(stderr,"No file name\n");
		exit(1);
	}
}

int main(int argc, char** argv){
	int prundeg = 0;

	org_g = new GraphType;
	prun_g = new GraphType;
	cpl_g = new GraphType;

	srand((unsigned int)time(NULL));
	readParameters(argc, argv);
	frec = createRecordFile();
//	frec = stdout;

	/*load the orignal graph*/
	loadGraph(org_g, graph_file_name);
	if (org_g->v_num > GRAPH_VERTICES_LIMIT){
		prundeg = prunningGraph(org_g, prun_g, GRAPH_VERTICES_LIMIT);
		convertComplementGraphMatrix(prun_g, cpl_g);
	}else{
		/*Convert the original graph to corresponding completed graph,
		 * we solve the r-quasi indipendent set problem*/
		convertComplementGraphMatrix(org_g, cpl_g);
	}
#if (DEBUG)
	showGraph(frec, cpl_g);
#endif

	Plex *all_best = new Plex[param_runcnt];
	Log *all_log = new Log[param_runcnt];

	assert(param_runcnt > 0);

//	preSetupConMap();
	for (int i = 0; i < param_runcnt; i++){
		all_best[i].verlist = NULL;
		/*start solve tabu*/
		solve_tabu(param_cycle, param_max_iteration , &(all_best[i]));
		assert(all_best[i].size - param_s > prundeg);
#if (DETAIL)
		fprintf(frec, "\nstart %dth search \n", i);
		if (!checkPlex(&(all_best[i]))){
			exit(1);
		}
		printf("total running time %f\n", cur_log.total_time);
		printPlex(frec, &(all_best[i]));
#endif
		/*show current solution */
		/*reserve the log information*/
		memcpy(&(all_log[i]), &cur_log, sizeof(Log));
	}
	/*show all the information to the record file*/
	fprintf(frec, "\nGraph |V| = %d; |E| = %d; Density = %.2f; s=%d; Prundeg = %d\n",cpl_g->v_num, cpl_g->e_num,
			(float)2* cpl_g->e_num/(cpl_g->v_num *(cpl_g->v_num -1)), param_s, prundeg);
	fprintf(frec, "filename %s; s %d; best_known_splex %d; max_iteration %d\n",
			graph_file_name, param_s, param_best, param_max_iteration);

	int sum_plex = 0;
	float sum_time = 0.f;
	long long int sum_iter = 0;
	fprintf(frec, "No.\t size\t Btime\t time\t Biter\t iter\t restart\t iter_time\n " );
	for (int i = 0; i < param_runcnt; i++){
		fprintf(frec, "%d\t %d\t %.2f\t %.2f\t %d\t %d\t \%d\t %.2f\n",
				i+1,
				all_best[i].size,
				all_log[i].best_found_time,
				all_log[i].total_time,
				all_log[i].best_found_iter,
				all_log[i].total_iters,
				all_log[i].total_restart,
				all_log[i].iave_time_us);
		sum_plex += all_best[i].size;
		sum_time += all_log[i].best_found_time;
		sum_iter += all_log[i].best_found_iter;
	}
	fprintf(frec, "average size %.3f\n",(float)sum_plex/param_runcnt);
	fprintf(frec, "average time %.3f\n", sum_time/param_runcnt);
	fprintf(frec,"average iter %lld\n", sum_iter / param_runcnt);
	fclose(frec);
}

