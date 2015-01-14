/*
 * graph.cpp
 *
 *  Created on: Nov 12, 2014
 *      Author: zhou
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "graph.h"
using namespace std;

#define MEM_MAX_NODES 100000

void addArc(GraphType *g, Arc *arc, int v){
	arc->next_arc = g->adj_list[v].first_arc;
	g->adj_list[v].first_arc = arc;
	(g->adj_list[v].degree)++;
}

void showGraph(FILE *fout, GraphType *g){
	fprintf(fout, "|Vertics| %d, |Edge| %d\n, Dens %f",g->v_num, g->e_num,
			2*g->e_num / float(g->v_num *(g->v_num-1)));
	for (int i = 0; i < g->v_num; i++)
	{
		fprintf(fout, "vertex %d id %d, degree %d: ", i, g->adj_list[i].vid, g->adj_list[i].degree);
		Arc *p = g->adj_list[i].first_arc;
		int deg = 0;
		while (p!=NULL){
			fprintf(fout, "%d ", p->adjvex);
			deg++;
			p = p->next_arc;
		}
		fprintf(fout, "\n");
		assert(deg == g->adj_list[i].degree);
	}
}
void convertComplementGraphMatrix(GraphType *org, GraphType *cg){
	assert(org->v_num <= MEM_MAX_NODES);
	cg->v_num = org->v_num;
	cg->matrix = new int*[cg->v_num];
	cg->adj_list = new AdjNode[cg->v_num];
	for (int i = 0; i < cg->v_num; i++){
		cg->adj_list[i].first_arc = NULL;
		cg->adj_list[i].degree = 0;
		cg->adj_list[i].info = NULL; //reserve
		cg->adj_list[i].vid = org->adj_list[i].vid;
	}
	int e_num = 0;
	//printf("completed graph\n");
	for (int i = 0; i < org->v_num; i++){
		cg->matrix[i] = new int[cg->v_num];
		for (int j = 0; j < cg->v_num; j++){
			cg->matrix[i][j] = 1;
		}
		Arc *parc = (org->adj_list[i]).first_arc;
		while (parc != NULL){
			cg->matrix[i][parc->adjvex] = 0;
			parc = parc->next_arc;
		}
		for (int j = 0; j < i; j++){
			if (cg->matrix[i][j] == 1){
				//printf("%d %d\n",i,j);
				Arc *parc1 = new Arc;
				parc1->weight = 0;
				parc1->adjvex = j;
				addArc(cg, parc1, i);

				Arc *parc2 = new Arc;
				parc2->weight = 0;
				parc2->adjvex = i;
				addArc(cg, parc2, j);
				e_num++;
			}
		}
	}
	assert(e_num == cg->v_num * (cg->v_num-1) / 2 - org->e_num);
	cg->e_num = e_num;
}


static void loadGraphDimacs(GraphType *g, char *filename){
	ifstream fin(filename);
	if (fin.fail()){
		cerr << "Can not open file " << filename << endl;
		exit(0);
	}
	char line[1000];
	g->v_num = 0;
	int ecnt = 0;
	while (fin >> line){
		if (fin.eof()){
			if (g->v_num == 0){
				cerr << "Empty grah";
			}
			return ;
		}
		if (strcmp(line, "p") == 0){
			char word[20]; //the word "edge"
			fin >> word >> g->v_num >> g->e_num;
			g->adj_list = new AdjNode[g->v_num];
			for (int i = 0; i < g->v_num; i++){
				g->adj_list[i].first_arc = NULL;
				g->adj_list[i].degree = 0;
				g->adj_list[i].info = NULL; //reserve
				g->adj_list[i].vid = i;
			}
		}
		if (strcmp(line, "e") == 0){
			int v1, v2;
			fin >> v1 >> v2;
			v1--,v2--; //in order to start from 0

			assert(v1 >= 0 && v1 < g->v_num && v2 >= 0 && v2 <= g->v_num);
			Arc *parc1 = new Arc;
			parc1->weight = 0;
			parc1->adjvex = v2;
			addArc(g, parc1, v1);

			Arc *parc2 = new Arc;
			parc2->weight = 0;
			parc2->adjvex = v1;
			addArc(g, parc2, v2);
			ecnt++;

		}
	}

	assert (ecnt == g->e_num);
	//INFO: density
	//cout << "VNum = " << g->v_num << "  ENum = " << g->e_num << endl;
	//cout << "Density = " << (float) 2*ecnt/(g->v_num*(g->v_num-1)) << endl;
}

static void loadGraphMetis(GraphType *g, char* filename){
	ifstream fin(filename);
	string line;
	int type = 0;

	if (fin.fail()){
		cerr << "Can not open file " << filename << endl;
		exit(0);
	}

	getline(fin, line);
	stringstream ss(line.c_str());
	ss >> g->v_num >> g->e_num >> type;
	assert (type == 0);		//目前还只支持了无向无权图
	g->adj_list = new AdjNode[g->v_num];
	for (int i = 0; i < g->v_num; i++){
			g->adj_list[i].first_arc = NULL;
			g->adj_list[i].degree = 0;
			g->adj_list[i].info = NULL; //reserve
			g->adj_list[i].vid = i;
	}

	int v_no = 0;
	int ecnt= 0; //count the edge
	while (getline(fin, line)){
		int adjv = 0;
		stringstream ssl(line);
		while (ssl >> adjv){
			Arc *parc = new Arc;
			parc->adjvex = adjv - 1; /*实际编号小1*/
			parc->weight = 0;
			addArc(g, parc, v_no);
			ecnt ++;
		}
		v_no++;
	}
	assert(v_no == g->v_num);
	assert(ecnt == 2 * g->e_num);
	//cout << "VNum = " << g->v_num << "  ENum = " << g->e_num << endl;
	//cout << "Density = " << (float)2 * g->e_num/(g->v_num*(g->v_num-1)) << endl;
}

/** detecting whether base is ends with str
 */
static bool endsWith (char* base, char* str) {
    int blen = strlen(base);
    int slen = strlen(str);
    return (blen >= slen) && (0 == strcmp(base + blen - slen, str));
}

void loadGraph(GraphType *g, char* filename){
	if (filename == NULL || g == NULL){
		cerr << "Empty graphs pointer or file name" << endl;
		return;
	}
	if (endsWith(filename,"clq")){
		loadGraphDimacs(g, filename);
	}else if (endsWith(filename, "graph")){
		loadGraphMetis(g, filename);
	}
	//TODO: show graph in stdout
	//showGraph(stdout, g);
}

void disposeGraph(GraphType *g, int size){
	for (int vi = 0; vi < size; vi++){
		Arc *parc =  g->adj_list[vi].first_arc;
		while (parc != NULL){
			g->adj_list[vi].first_arc = parc->next_arc;
			delete parc;
			parc = g->adj_list[vi].first_arc;
		}
		g->v_num--;
	}
}
/**
 * copy unmarked vertices to new graph
 */
static void shrinkGraph(GraphType *org, GraphType *newg, int *deleted, int delete_num){
	int e_num = 0;
	int *order = new int[org->v_num];
	int cnt = 0;
	for (int i = 0; i < org->v_num; i++)
		if (!deleted[i]) order[i] = cnt++;

	newg->v_num = org->v_num - delete_num;
	assert(cnt == newg->v_num); /*Ensure consistency*/

	newg->adj_list = new AdjNode[newg->v_num];
	for (int i = 0; i < newg->v_num; i++){
		newg->adj_list[i].first_arc = NULL;
		newg->adj_list[i].degree = 0;
		newg->adj_list[i].info = NULL; //reserve
	}
	for (int id = 0; id < org->v_num; id++){
		if (!deleted[id]){
			newg->adj_list[order[id]].vid = id;
			Arc *parc = org->adj_list[id].first_arc;
			while(parc != NULL){
				int vcur = parc->adjvex;
				if (!deleted[vcur]){
					Arc *newarc = new Arc;
					newarc->adjvex = order[vcur];
					newarc->weight = parc->weight;
					addArc(newg, newarc, order[id]);
					e_num++;
				}
				parc = parc->next_arc;
			}
		}
	}
	newg->e_num = e_num/2;
	assert(cnt == newg->v_num);
	delete[] order;
}
int prunningGraph(GraphType *org, GraphType *prunnedg, int gsize){
	int prun_deg = 0;
	int *removed= new int[org->v_num];
	int *deg = new int[org->v_num];
	memset(removed, 0, sizeof(int) * org->v_num);
	int *queue = new int[org->v_num * 4];
	int qend = 0;
	int qstart = 0;
	int prun_num = 0;
	for (int i = 0; i < org->v_num; i++)
		deg[i] = org->adj_list[i].degree;
	while (org->v_num- prun_num > gsize){
		prun_deg += 1;
		for (int i = 0;i < org->v_num; i++){
			if (!removed[i] && deg[i] <= prun_deg){
					removed[i] = 1;
					prun_num++;
					queue[qend++] = i;	/*put the removed vertex in the queue*/
			}
		}
		while (qend > qstart){
			//remove the vertex from queue
			int curv = queue[qstart++];
			//update the degree
			Arc *parc = org->adj_list[curv].first_arc;
			while(parc != NULL){
				int adjv = parc->adjvex;
				deg[adjv]--;
				if (!removed[adjv] && deg[adjv] <= prun_deg){
						removed[adjv] = 1;
						prun_num++;
						queue[qend++] = adjv;
				}
				parc = parc->next_arc;
			}
		}
	}
	/*Ensure safty*/
	assert(qend < org->v_num * 4);
	if (prunnedg == NULL)
		prunnedg = new GraphType;
	shrinkGraph(org, prunnedg, removed, prun_num);
	delete[] removed;
	delete[] deg;
	delete[] queue;
	return prun_deg;
}

int testConvertComplete(){
	GraphType *newg = new GraphType;
	GraphType *cg = new GraphType;
	loadGraph(newg, "graph_test.clq");
	convertComplementGraphMatrix(newg, cg);
	showGraph(stdout, cg);
	return 0;
}

int testPrunning(){
	GraphType *g = new GraphType;
//	GraphType *cg = new GraphType;
	GraphType *pg = new GraphType;
//	loadGraph(g, "/home/zhou/workspace/splex/graph_test.clq");
	loadGraph(g, "adjnoun.graph");
//	convertComplementGraphMatrix(g, cg);
	prunningGraph(g, pg, 110);
	showGraph(stdout, pg);
	return 0;
}

int __main__(){
	return testPrunning();
}
