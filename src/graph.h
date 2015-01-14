/*
 * graph.h
 *
 *  Created on: Nov 12, 2014
 *      Author: zhou
 */

#ifndef GRAPH_H_
#define GRAPH_H_
#include <stdio.h>

typedef struct ArcStruct{
	int adjvex;
	struct ArcStruct *next_arc;
	int weight;
}Arc; /*The arc to each vertex*/

typedef struct {
	int degree;
	Arc *first_arc;
	char *info; // reserved
	int vid;
}AdjNode;

typedef struct{
	AdjNode *adj_list;
	int **matrix;
	int v_num;
	int e_num;
}GraphType;

extern void showGraph(FILE *fout, GraphType *g);
extern void convertComplementGraphMatrix(GraphType *org, GraphType *cg);
extern void loadGraph(GraphType *g, char* filename);
extern void disposeGraph(GraphType *g, int size);
extern int prunningGraph(GraphType *org, GraphType *prunnedg, int gsize);

#endif /* GRAPH_H_ */
