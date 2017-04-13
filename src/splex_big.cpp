//============================================================================
// Name        : splex-big.cpp
// Author      : JOEY
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <libgen.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <algorithm>
#include <sstream>
#include <list>
#include <utility>
#include "utils.hpp"
using namespace std;
#define LARGE_INT 99999999
/*Parameters*/

char param_graph_file_name[1024]="/home/zhou/benchmarks/splex/2nd_dimacs/brock400_4.clq";
int param_s = 2;
int param_best = 9999;
int param_max_seconds = 120;
int param_cycle_iter = 1000;
unsigned int param_seed;

/*original graph, the structure is kept the same,
 * but the id of vertices are renumbered, the original
 * id can be retrievaled by org_vid*/
int org_vnum;
int org_enum;
int org_fmt;
int* org_v_edge_cnt;
int** org_v_adj_vertex;
int *org_vid;	/*The real id of each original vertex*/

/*reduced graph*/
int red_vnum;
int red_enum;
int *red_orgid; /*the corresponding id in original graph*/
int* red_v_edge_cnt;
int** red_v_adj_vertex;
int red_min_deg ;
int *freq;

int *tabu_add;
int cur_iter;
int *cur_c_deg;
int *cur_c_consat; /*cur_c_consat[v] Then number of saturated neighboors of v*/
int cur_c_satu_num; //the number of saturate vertices
int *is_in_c;

RandAccessList *cur_splex;
RandAccessList *cur_cand;
//RandAccessList *cur_satured;
clock_t start_time;

/*final result*/
int best_size;
int* best_plex;
clock_t best_time;
int best_found_iter;
int total_iter;
clock_t total_time;
int total_start_pass;

/*debug*/
void print_reduced_graph(){
	cout << red_vnum << " " << red_enum << endl;
	if (red_vnum <= 50){
		for (int v = 0; v < red_vnum; v++){
			printf("v %d (%d):", v, red_v_edge_cnt[v]);
			for (int i = 0; i < red_v_edge_cnt[v]; i++){
				cout << red_v_adj_vertex[v][i] << " ";
			}
			cout << endl;
		}
	}
}

void print_org_graph(){
	cout << org_vnum << " " << org_enum <<" "<< org_fmt << endl;
	for (int v = 0; v < org_vnum; v++){
		printf("v %d (%d):", v, org_v_edge_cnt[v]);
		for (int i = 0; i < org_v_edge_cnt[v]; i++){
			cout << org_v_adj_vertex[v][i] << " ";
		}
		cout << endl;
	}
}

/**
 * load instances from  2nd Dimacs competetion
 *
 */
int load_clq_instance(char* filename){
	ifstream infile(filename);
	char line[1024];
	char tmps1[1024];
	char tmps2[1024];
	/*graph adjacent matrix*/
	int **gmat;
	if (infile == NULL){
		fprintf(stderr,"Can not find file %s\n", filename);
		return 0;
	}

	infile.getline(line,  1024);
	while (line[0] != 'p')	infile.getline(line,1024);
	sscanf(line, "%s %s %d %d", tmps1, tmps2, &org_vnum, &org_enum);

	gmat = new int*[org_vnum];
	for (int i = 0; i < org_vnum; i++){
		gmat[i] = new int[org_vnum];
		memset(gmat[i], 0, sizeof(int) * org_vnum);
	}

	int ecnt = 0;
	org_v_edge_cnt = new int[org_vnum];
	while (infile.getline(line, 1024)){
		int v1,v2;
		if (strlen(line) == 0)
			continue;
		if (line[0] != 'e')
			fprintf(stderr, "ERROR in line %d\n", ecnt+1);
		sscanf(line, "%s %d %d", tmps1, &v1, &v2);
		v1--,v2--;
		gmat[v1][v2] = 1;
		gmat[v2][v1] = 1;
		org_v_edge_cnt[v1]++;
		org_v_edge_cnt[v2]++;
		ecnt++;
	}
	assert(org_enum == ecnt);
	org_fmt = 0;
	org_vid = new int[org_vnum];
	org_v_adj_vertex = new int*[org_vnum];
	for (int i = 0; i < org_vnum; i++){
		int adj_cnt = 0;
		org_v_adj_vertex[i] = new int[org_v_edge_cnt[i]];
		org_vid[i] = i+1;
		for (int j = 0; j < org_vnum; j++){
			if (gmat[i][j] == 1)
				org_v_adj_vertex[i][adj_cnt++] = j;
		}
		assert(org_v_edge_cnt[i] == adj_cnt);
		delete[] gmat[i];
	}
	printf("load 2nd dimacs graph %s with vertices %d, edges %d \n",
				basename(param_graph_file_name), org_vnum, org_enum);
	delete[] gmat;
	return 1;
}

/*load instances from  Stanford Large Network Dataset Collection
 * URL: http://snap.stanford.edu/data/*/
int load_snap_instance(char *filename){
	ifstream infile(filename);
	char line[1024];
	vector<pair<int,int> > *pvec_edges = new vector<pair<int, int> >();
	const int CONST_MAX_VE_NUM = 9999999;
	if (infile == NULL){
		fprintf(stderr,"Can not find file %s\n", filename);
		return 0;
	}
	int max_id = 0;
	int from, to;
	//int max_id = 0;
	while (infile.getline(line, 1024)){
		char *p = line;
		while (*p ==' ' && *p !='\0') p++;
		if (*p != '#'){
			stringstream ss(line);
			ss >> from >> to;
			//printf("%d %d \n", from, to);
			pvec_edges->push_back(make_pair(from, to));
			if (from > max_id)
				max_id = from;
			else if (to > max_id)
				max_id = to;
		}
	}
	int *newid = new int[max_id+1];
	org_vid = new int[CONST_MAX_VE_NUM];
		//init the newid map
	for (int i = 0; i < max_id+1; i++){
		newid[i] = -1;
	}

	int v_num = 0;
	//count edges,resign ids from 1...v_num
	for (int i = 0; i < (int)pvec_edges->size(); i++){
		from = (pvec_edges->at(i)).first;
		if (newid[from] == -1){
			newid[from] = v_num;
			org_vid[v_num] = from;
			v_num++;
		}
		(pvec_edges->at(i)).first = newid[from];
		to = (pvec_edges->at(i)).second;
		if (newid[to] == -1) {
			newid[to] = v_num;
			org_vid[v_num] = to;
			v_num++;
		}
		(pvec_edges->at(i)).second = newid[to];
	}
//	sort(pvec_edges->begin(), pvec_edges->end());
//	for (int i = 0; i < pvec_edges->size(); i++){
//		printf("%d %d\n", pvec_edges->at(i).first, pvec_edges->at(i).second);
//	}
	org_vnum = v_num;
	int *estimate_edge_cnt = new int[org_vnum];
	memset(estimate_edge_cnt, 0, sizeof(int) * org_vnum);
	for (int i = 0; i < (int)pvec_edges->size(); i++){
		from = (pvec_edges->at(i)).first;
		to = (pvec_edges->at(i)).second;
		estimate_edge_cnt[from]++;
		estimate_edge_cnt[to]++;
	}
	org_enum = 0;
	org_v_edge_cnt = new int[org_vnum];
	memset(org_v_edge_cnt, 0, sizeof(int) * org_vnum);
	org_v_adj_vertex = new int*[org_vnum];
	for (int i = 0; i < (int)pvec_edges->size(); i++){
		from = (pvec_edges->at(i)).first;
		to = (pvec_edges->at(i)).second;
		if (from == to) continue; //self-edges are abandoned
		if (org_v_edge_cnt[from] == 0){
			org_v_adj_vertex[from] = new int[estimate_edge_cnt[from]];
		}
		int exist1 = 0;
		for (int j = 0; j < org_v_edge_cnt[from]; j++){
			if (org_v_adj_vertex[from][j] == to){
				exist1 = 1;
				break;
			}
		}
		if (!exist1){
			org_v_adj_vertex[from][org_v_edge_cnt[from]] = to;
			org_v_edge_cnt[from]++;
			org_enum++;
		}

		if (org_v_edge_cnt[to] == 0){
			org_v_adj_vertex[to] = new int[estimate_edge_cnt[to]];
		}
		int exist2 = 0;
		for (int j = 0; j < org_v_edge_cnt[to]; j++){
			if (org_v_adj_vertex[to][j] == from){
				exist2 = 1;
				break;
			}
		}
		if (!exist2){
			org_v_adj_vertex[to][org_v_edge_cnt[to]] = from;
			org_v_edge_cnt[to]++;
			org_enum++;
		}
	}
	assert(org_enum %2 == 0);
	org_enum = org_enum/2;
	printf("load SNAP graph %s with vertices %d, edges %d (directed edges %d)\n",
			basename(param_graph_file_name), org_vnum, org_enum, (int)pvec_edges->size());
	//print_org_graph();
	//build v_
	delete[] newid;
	delete[] estimate_edge_cnt;
	delete pvec_edges;
	return 1;
}

/*load instances of metis format from
 * instances are download from
 * http://www.cc.gatech.edu/dimacs10/downloads.shtml
 */
int load_metis_instance(char* filename){
	ifstream infile(filename);
	string line;

	if (infile == NULL){
		fprintf(stderr,"Can not find file %s\n", filename);
		exit(0);
	}
	org_fmt = 0;
	//ignore comment
	getline(infile, line);
	while(line.length()==0 || line[0] == '%')
		getline(infile, line);
	stringstream l1_ss(line);
	l1_ss >> org_vnum >> org_enum >> org_fmt;
	cout << line << endl;
	if (org_fmt == 100){
		cerr << "self-loops and/or multiple edges need to be considered";
		return 0;
	}
	/*allocate graph memory*/
	org_v_edge_cnt = new int[org_vnum];
	org_v_adj_vertex = new int*[org_vnum];
	org_vid = new int[org_vnum];

	int v_no = 0;
	int *vlst = new int[org_vnum];
	int n_adj = 0;
	//ignore the char
	//read the end of first line
	//infile.getline(line, LINE_LEN);
	while (getline(infile, line) && v_no < org_vnum){
		if (infile.fail()){
			fprintf(stderr, "Error in read file, vno %d\n",v_no);
			exit(-1);
		}
		stringstream ss(line);
		int ve_adj;

		n_adj = 0;
 		while (ss >> ve_adj){
			vlst[n_adj++] = ve_adj-1;
		}
		org_v_edge_cnt[v_no] = n_adj;
		if (n_adj == 0)
			org_v_adj_vertex[v_no] = NULL;
		else{
			org_v_adj_vertex[v_no] = new int[n_adj];
			memcpy(org_v_adj_vertex[v_no], vlst, sizeof(int) * n_adj);
		}
		org_vid[v_no] = v_no+1;
		v_no++;
	}
	assert(v_no == org_vnum);
	printf("load metis graph %s with vertices %d, edges %d \n",
			basename(param_graph_file_name), org_vnum, org_enum);
	//debug
	delete[] vlst;
	infile.close();
	return 1;
}
void print_current_solution(){
	printf("current solution \n");
	printf("size:%d \n",cur_splex->vnum);
	int *tmp_lst= new int[cur_splex->vnum];
	memcpy(tmp_lst, cur_splex->vlist, cur_splex->vnum * sizeof(int));
	qsort(tmp_lst, cur_splex->vnum, sizeof(int), cmpfunc);
	for(int i = 0;i < cur_splex->vnum; i++){
		printf("%d(%d) ", tmp_lst[i], cur_c_deg[tmp_lst[i]]);
	}
	printf("\n");
	delete[] tmp_lst;
}
void print_current_contex(){
	print_current_solution();
	printf("current candidate size %d :\n", cur_cand->vnum);
	for (int i = 0; i< cur_cand->vnum; i++){
		int v = cur_cand->vlist[i];
		printf("%d(%d): ", v, cur_c_deg[v]);
		for (int j = 0; j < red_v_edge_cnt[v]; j++){
			int vadj = red_v_adj_vertex[v][j];
			if (is_in_c[vadj])
				printf("%d ", vadj);
		}
		printf("\n");
	}
	printf("\n");
}

/*recursively remove all the vertices with degree less or equal than reduce_deg,
 * reconstructing the reduced graph  */
void reduce_graph(int reduce_deg){
	int rm_cnt = 0;
	int *rmflag = new int[red_vnum];
	int *remain_edge_count = new int[red_vnum];
	/*Avoid unnecessary reduction*/
	if (red_min_deg > reduce_deg){
		return;
	}
	memcpy(remain_edge_count, red_v_edge_cnt, sizeof(int) * red_vnum);
	memset(rmflag, 0, sizeof(int) * red_vnum);
	queue<int> rm_que;
	for (int idx = 0; idx < red_vnum; idx++){
		if (remain_edge_count[idx] <= reduce_deg){
			rmflag[idx] = 1;
			rm_que.push(idx);
			rm_cnt++;
		}
	}
	while(!rm_que.empty()){
		int idx = rm_que.front();
		rm_que.pop();
		for (int i = 0; i < red_v_edge_cnt[idx]; i++){
			int adjv = red_v_adj_vertex[idx][i];
			remain_edge_count[adjv]--;
			if (!rmflag[adjv] && remain_edge_count[adjv] <= reduce_deg){
				rmflag[adjv] = 1;
				rm_que.push(adjv);
				rm_cnt++;
			}
		}
	}

	/*rebuild the reduced graph*/
	int rest_vnum = red_vnum - rm_cnt;
	int *new_id = new int[red_vnum];
	/*Mapp the vertex id to the original id(in the initial graph) */
	int *org_id = new int[rest_vnum];
	int count = 0; //count the rest vertices
	int *last_freq = new int[rest_vnum];
	//resign new id to the rest of the vertices
	for (int idx = 0; idx < red_vnum; idx++){
		if (!rmflag[idx]){
			new_id[idx] = count;
			org_id[count] = red_orgid[idx];
			last_freq[count] = freq[idx];
			count++;
		}
	}
	/**/
	int min_deg = LARGE_INT;
	int n_edges = 0;
	int **new_adj_tbl = new int*[rest_vnum];
	int *new_edge_count = new int[rest_vnum];
	for (int idx_prev = 0; idx_prev < red_vnum; idx_prev++){
		if (rmflag[idx_prev]){
			delete[] red_v_adj_vertex[idx_prev];
			continue;
		}
		int idx_new = new_id[idx_prev];
		new_adj_tbl[idx_new] = new int[remain_edge_count[idx_prev]];
		new_edge_count[idx_new] = remain_edge_count[idx_prev];
		int cnt = 0;
		for (int i = 0; i < red_v_edge_cnt[idx_prev]; i++){
			int vi_adj = red_v_adj_vertex[idx_prev][i];
			if (!rmflag[vi_adj]){
				new_adj_tbl[idx_new][cnt++] = new_id[vi_adj];
				n_edges++;
			}
		}
		if (new_edge_count[idx_new] < min_deg)
			min_deg = new_edge_count[idx_new];
		assert(cnt == remain_edge_count[idx_prev]);
		delete[] red_v_adj_vertex[idx_prev];
	}
	assert(n_edges % 2 == 0);
	printf("(V%d E%d) reduce to (V%d, E%d)\n", red_vnum, red_enum, rest_vnum, n_edges/2);
	/*assign to the new graph*/
	delete[] red_v_adj_vertex;
	red_v_adj_vertex = new_adj_tbl;
	delete[] red_v_edge_cnt;
	red_v_edge_cnt = new_edge_count;

	red_vnum = rest_vnum;
	red_enum = n_edges/2;

	/*reset the minimum deg*/
	red_min_deg = min_deg;

	/*reset the original id map*/
	delete[] red_orgid;
	red_orgid = org_id;

	/*reset the last join record*/
	delete[] freq;
	freq = last_freq;

	delete[] new_id;
	delete[] rmflag;
	delete[] remain_edge_count;

}

/*reinitial the data for a new start of the algorithm*/
void init_search(){
	/*copy the graph to reduce graph*/
	red_vnum = org_vnum;
	red_enum = org_enum;
	red_orgid = new int[red_vnum];
	red_v_edge_cnt = new int[red_vnum];
	red_v_adj_vertex = new int*[red_vnum];
	red_min_deg = LARGE_INT;
	memcpy(red_v_edge_cnt, org_v_edge_cnt, sizeof(int) * org_vnum);
	for (int v = 0; v < red_vnum; v++){
		red_orgid[v] = v;
		red_v_adj_vertex[v] = new int[red_v_edge_cnt[v]];
		memcpy(red_v_adj_vertex[v], org_v_adj_vertex[v], sizeof(int) * org_v_edge_cnt[v]);
		if (red_v_edge_cnt[v] < red_min_deg)
			red_min_deg = red_v_edge_cnt[v];
	}

	/*init search data*/
	cur_iter = 0;
	cur_c_deg = new int[red_vnum];
	is_in_c = new int[red_vnum];
	memset(cur_c_deg, 0, sizeof(int) * red_vnum);
	memset(is_in_c, 0, sizeof(int) * red_vnum);
	freq = new int[red_vnum];

	cur_splex = ral_init(red_vnum);
	cur_cand = ral_init(red_vnum);

	tabu_add = new int[red_vnum];

	/*init best found data*/
	best_size = 0;
	/*We could reduce this size*/
	best_plex = new int[red_vnum];
	best_time = 0;
	best_found_iter = 0;
	total_start_pass = 0;

	start_time = clock();
    srand((unsigned int)param_seed);
}

void restart_search(){
	//cur_iter = 0;
	memset(cur_c_deg, 0, sizeof(int) * red_vnum);
	memset(is_in_c, 0, sizeof(int) * red_vnum);
	ral_clear(cur_splex);
	ral_clear(cur_cand);
	memset(tabu_add, 0, sizeof(int) * red_vnum);
}
void add_cur_vertex(int v){
	assert(!is_in_c[v]);

	is_in_c[v] = 1;
	ral_add(cur_splex, v);
	if (cur_c_deg[v] > 0){
		ral_delete(cur_cand, v);
	}

	for (int i = 0; i < red_v_edge_cnt[v]; i++){
		int adjv = red_v_adj_vertex[v][i];
		cur_c_deg[adjv]++;
		if (!is_in_c[adjv] && cur_c_deg[adjv] == 1){
			ral_add(cur_cand, adjv);
		}
	}
}

void remove_cur_vertex(int v){
	assert(is_in_c[v]);

	is_in_c[v] = 0;
	ral_delete(cur_splex, v);
	if (cur_c_deg[v] > 0){
		ral_add(cur_cand, v);
	}

	for (int i = 0; i < red_v_edge_cnt[v]; i++){
		int adjv = red_v_adj_vertex[v][i];
		cur_c_deg[adjv]--;
		if (!is_in_c[adjv] && cur_c_deg[adjv] == 0){
			ral_delete(cur_cand, adjv);
		}
	}
}

#define Is_Saturated(v) (cur_c_deg[v] == cur_splex->vnum - param_s)
#define Is_Overflow(v) (cur_c_deg[v] < cur_splex->vnum - param_s)

int get_saturate_size(){
	int size = 0;
	for (int i = 0; i < cur_splex->vnum; i++){
		int vin = cur_splex->vlist[i];
		if (Is_Saturated(vin)){
			size++;
		}
	}
	return size;
}

void record_best(){
	best_size = cur_splex->vnum;
	for (int i = 0; i < cur_splex->vnum; i++){
		int v = cur_splex->vlist[i];
		best_plex[i] = red_orgid[v];
	}
	//debug
	best_time = clock();
	best_found_iter = cur_iter;
}


void fast_init_solution(){
	vector<int> vec_M1;
	int leastfreq = LARGE_INT;
	for (int i = 0; i < 100; i++){
		int randv = rand() % red_vnum;
		if (freq[randv] < leastfreq){
			vec_M1.clear();
			vec_M1.push_back(randv);
			leastfreq = randv;
		}else if (freq[randv] == leastfreq){
			vec_M1.push_back(freq[randv]);
		}
	}
	int vstart = vec_M1[rand()%vec_M1.size()];
	add_cur_vertex(vstart);
	while (1){
		int sat_size = get_saturate_size();
		vec_M1.clear();
		leastfreq = LARGE_INT;
		//print_current_contex();
		for (int i = 0; i < cur_cand->vnum; i++){
			int vi = cur_cand->vlist[i];
			int satcon = 0;
			if (cur_c_deg[vi] >= cur_splex->vnum - param_s + 1){
				//verify the connection to staturation vertices
				for (int idx = 0; idx < red_v_edge_cnt[vi]; idx++){
					int vcur = red_v_adj_vertex[vi][idx];
					if (is_in_c[vcur] && Is_Saturated(vcur))
						satcon++;
				}
				if (satcon == sat_size){
					if (freq[vi] < leastfreq){
						vec_M1.clear();
						vec_M1.push_back(vi);
						leastfreq = freq[vi];
					}else if(freq[vi] < leastfreq){
						vec_M1.push_back(vi);
					}
				}
			}
		}
		if (vec_M1.empty()){
			break;
		}
		int vadd = vec_M1[rand()%vec_M1.size()];
		add_cur_vertex(vadd);
		//print_current_contex();
	}
	while( cur_splex->vnum < param_s){
		int vrand = rand() % red_vnum;
		while (is_in_c[vrand]) vrand = rand() % red_vnum;
		add_cur_vertex(vrand);
	}
	if (cur_splex->vnum > best_size){
		record_best();
	}
}

/*find the only unadjacent saturated vertex of v*/
int find_unadj_satu(int v){
	assert(!is_in_c[v]);
	int *mark = new int[cur_splex->vnum];
	int v_rt = -1;
	memset(mark, 0, sizeof(int) * cur_splex->vnum);
	for (int i = 0; i < red_v_edge_cnt[v]; i++){
		int vcur = red_v_adj_vertex[v][i];
		if (is_in_c[vcur]){
			mark[cur_splex->vpos[vcur]] = 1;
		}
	}
	for (int i = 0; i < cur_splex->vnum; i++){
		int vin = cur_splex->vlist[i];
		if (Is_Saturated(vin) && mark[i] == 0){
			v_rt = vin;
			break;
		}
	}
	delete[] mark;
	return v_rt;
}

/*get a random vertex of C\N_C(v)*/
int random_unadj_with_exception(int v, int vexception){
	assert(!is_in_c[v]);
	int *mark = new int[cur_splex->vnum];
	vector<int> vec_unadj;
	memset(mark, 0, sizeof(int) * cur_splex->vnum);
	for (int i = 0; i < red_v_edge_cnt[v]; i++){
		int vcur = red_v_adj_vertex[v][i];
		if (is_in_c[vcur]){
			mark[cur_splex->vpos[vcur]] = 1;
		}
	}
	for (int i = 0; i < cur_splex->vnum; i++){
		if (!mark[i] && cur_splex->vlist[i] != vexception)
			vec_unadj.push_back(cur_splex->vlist[i]);
	}
	delete[] mark;
	if (vec_unadj.size() == 0)
		return -1;
	else
		return vec_unadj[rand() % vec_unadj.size()];
}

void push_vertex_tabu(int vpush){
	add_cur_vertex(vpush);
	/*repair*/
	int idx = 0;
	while (idx < cur_splex->vnum){
		int vin = cur_splex->vlist[idx];
		if (Is_Overflow(vin)){
			remove_cur_vertex(vin);
			tabu_add[vin] = cur_iter + 7;
			//ATTENTION: idx need to be hold for another round
			freq[vin]++;
		}else
			idx++;
	}
//	printf("\n");
}

void tabu_based_search(){
	vector<int> vec_M1;
	vector<pair<int,int> > vec_M2_out;
	vector<int> vec_M2_in;
	vector<int> vec_M3;
	int cycle_iter = 0;
	int cycle_best = cur_splex->vnum;
	int fixed = -1;
	int max_freq;

	while (1){
		int end = 0;
		int non_improve_iter = 0;
		while (!end){
			int satu_size = get_saturate_size();
			vec_M1.clear();
			vec_M2_out.clear();
			vec_M3.clear();
			max_freq = 0;
			//vec_M4.clear();
			/*Update M1 M2 sets*/
			for (int i = 0; i < cur_cand->vnum; i++){
				int vcand = cur_cand->vlist[i];
				int satcon = 0;

				if (cur_c_deg[vcand] >= cur_splex->vnum - param_s){
					//check all the adjacent vertices of vcan in C
					for (int idx = 0; idx < red_v_edge_cnt[vcand]; idx++){
						int vcur = red_v_adj_vertex[vcand][idx];
						if (is_in_c[vcur] && Is_Saturated(vcur)){
							satcon++;
						}
					}
					if (cur_c_deg[vcand] >= cur_splex->vnum - param_s + 1
							&& satcon == satu_size
							&& (tabu_add[vcand] <= cur_iter || cur_splex->vnum + 1 > cycle_best)){
						vec_M1.push_back(vcand);
					}else if (cur_c_deg[vcand] >= cur_splex->vnum - param_s
							&& satcon == satu_size - 1
							&& tabu_add[vcand] <= cur_iter){
						vec_M2_out.push_back(make_pair(vcand, 0));
					}else if (cur_c_deg[vcand] == cur_splex->vnum - param_s
							&& satcon == satu_size
							&& tabu_add[vcand] <= cur_iter){
						vec_M2_out.push_back(make_pair(vcand, 1));
					}else if (tabu_add[vcand] <= cur_iter ){
						if (freq[vcand] > max_freq){
							vec_M3.clear();
							vec_M3.push_back(vcand);
							max_freq = vcand;
						}else if (freq[vcand] == max_freq){
							vec_M3.push_back(vcand);
						}
					}
				}else if (tabu_add[vcand] <= cur_iter){
					if (freq[vcand] > max_freq){
						vec_M3.clear();
						vec_M3.push_back(vcand);
						max_freq = vcand;
					}else if (freq[vcand] == max_freq){
						vec_M3.push_back(vcand);
					}
				}
			}
			/*Add or Swith move*/
			if (!vec_M1.empty()){
				int vadd = vec_M1[rand() % vec_M1.size()];
				add_cur_vertex(vadd);
				if (cur_splex->vnum > cycle_best){
					cycle_best = cur_splex->vnum;
				}
				freq[vadd]++;
			}else if (!vec_M2_out.empty()){
				pair<int,int> pvswp = vec_M2_out[rand() % vec_M2_out.size()];
				int vswp_in;
				if (pvswp.second == 0){ //type 1
					vswp_in = find_unadj_satu(pvswp.first);
				}else{ //type 2
					vswp_in = random_unadj_with_exception(pvswp.first, fixed);
				}
				if (vswp_in == fixed || vswp_in == -1)
					end = 1;
				else{
					remove_cur_vertex(vswp_in);
					add_cur_vertex(pvswp.first);
					tabu_add[vswp_in] = cur_iter + 10 + vec_M2_out.size();
					freq[vswp_in]++;
					freq[pvswp.first]++;
				}
			}else{
				end = 1;
			}
			if (cur_splex->vnum > best_size){
				record_best();
				if (best_size == param_best) //reach optimum
					goto ts_stop;
				non_improve_iter = 0;
			}else{
				non_improve_iter++;
			}
			if (non_improve_iter > param_s * best_size){
				end = 1;
			}
			cur_iter++;
			cycle_iter++;

			if(cycle_iter > param_cycle_iter ||
					(float)(clock() - start_time) / CLOCKS_PER_SEC > param_max_seconds){
				goto ts_stop;
			}
			if (cur_iter % 10000 == 0){
				printf("ITER: %d best %d\n", cur_iter, best_size);
			}
		}
		//perturb by push
		if (vec_M3.empty())
			break;
		int vpush = vec_M3[rand() % vec_M3.size()];
		freq[vpush] = 0;
		fixed = vpush;
		push_vertex_tabu(vpush);
	}
ts_stop:
	return;
}


/*TODO:Entrance of the whole search*/
void search_main(){
	int restart_pass = 0;
	/*Initial data structure*/
	init_search();
	/*start from a new solution*/
	fast_init_solution();
	reduce_graph(best_size - param_s);
	if (red_vnum == 0 ){
		goto end;
	}
	while (1){
		restart_search();
		fast_init_solution();
		tabu_based_search();
		if (best_size == param_best)
			goto end;
		if (best_size - param_s >= red_min_deg){
			reduce_graph(best_size - param_s);
		}
		if (red_vnum <= best_size )
			goto end;
		restart_pass++;
		if ((float)(clock() - start_time) / CLOCKS_PER_SEC > param_max_seconds ){
			break;
		}
	}
end:
	total_start_pass = restart_pass;
	total_time = clock();
	total_iter = cur_iter;
}
void report_result(){
	printf("\n");
	printf("$seed=%d\n", param_seed);
	printf("@solver=splex\n");
	printf("@para_file=%s\n",param_graph_file_name);
	printf("@para_s=%d\n",param_s);
	printf("@para_kbv=%d\n",param_best);
	printf("@para_seconds=%d\n",param_max_seconds);
	printf("#orgvnum=%d\n",org_vnum);
	printf("#orgenum=%d\n",org_enum);
	printf("#redvnum=%d\n", red_vnum);
	printf("#redenum=%d\n",red_enum);
	printf("#objv=%d\n",best_size);
	printf("#bestiter=%d\n",best_found_iter);
	printf("#besttime=%.3f\n",(float)(best_time - start_time) / CLOCKS_PER_SEC);
	printf("#totaliter=%d\n",total_iter);
	printf("#totalpass=%d\n",total_start_pass);
	printf("#totaltime=%.3f\n",(float)(total_time - start_time) / CLOCKS_PER_SEC);
	printf("Solution:");
	for (int i = 0; i < best_size; i++){
		int vorg = org_vid[best_plex[i]];
		printf("%d ", vorg);
	}
	printf("\n");
}

void showUsage(){
	fprintf(stderr, "splex -f <filename> -s <paramete s> -t <max seconds>  [-o optimum object] [-c seed]");
}

void read_params(int argc, char **argv){
	int hasFile = 0;
	int hasTimeLimit = 0;
	int hasS = 0;
	for (int i = 1; i < argc; i+=2){
		if (argv[i][0] != '-' || argv[i][2] != 0){
			showUsage();
			exit(0);
		}else if (argv[i][1] == 'f'){
			strncpy(param_graph_file_name, argv[i+1],1000);
			hasFile = 1;
		}else if(argv[i][1] == 's'){
			param_s = atoi(argv[i+1]);
			if (param_s >= 1)	hasS = 1;
		}else if (argv[i][1] == 'o'){
			param_best = atoi(argv[i+1]);
		}else if (argv[i][1] == 'c'){
			param_seed = atoi(argv[i+1]);
		}else if(argv[i][1] == 't'){
			param_max_seconds = atoi(argv[i+1]);
			hasTimeLimit = 1;
		}
	}
	/*check parameters*/
	if (!hasFile){
		fprintf(stderr,"No file name\n");
		showUsage();
		exit(1);
	}
	if (!hasTimeLimit){
		fprintf(stderr,"No time limit \n");
		showUsage();
		exit(1);
	}
	if (!hasS){
		fprintf(stderr,"No paramete s\n");
		showUsage();
		exit(1);
	}
}

int check_solution(){
	int *mark = new int[org_vnum];
	memset(mark, 0, sizeof(int) * org_vnum);
	for (int i = 0; i < best_size; i++){
		mark[best_plex[i]] = 1;
	}
	for (int i = 0; i < best_size; i++){
		int v = best_plex[i];
		int indeg = 0;
		for (int j = 0; j < org_v_edge_cnt[v]; j++){
			int vadj = org_v_adj_vertex[v][j];
			if (mark[vadj])
				indeg++;
		}
		if (indeg < best_size - param_s)
			return 0;
	}
	delete[] mark;
	return 1;
}

const char* file_suffix(char* filename){
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}
int main(int argc, char** argv) {
	int load = 0;

	read_params(argc, argv);
	const char* fileext = file_suffix(param_graph_file_name);
	//printf("%s\n",fileext);
	if (0 == strcmp(fileext, "graph")){
		load = load_metis_instance(param_graph_file_name);
	}else if(0 == strcmp(fileext, "txt")){
		load = load_snap_instance(param_graph_file_name);
	}else if(0 == strcmp(fileext, "clq")){
		load = load_clq_instance(param_graph_file_name);
	}
	if (load != 1){
		fprintf(stderr, "failed in loading graph %s\n",param_graph_file_name);
		exit(-1);
	}

	search_main();
	int chk = check_solution();
	if (chk == 0){
		fprintf(stderr,"ERROR! Final solution is infeasible\n");
		exit(-1);
	}
	report_result();
}
