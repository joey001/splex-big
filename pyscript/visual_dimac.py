'''
Created on Nov 26, 2014
visulaze the graph/completed in dimacs format 
@author: zhou
'''
import sys
import networkx as nx
import matplotlib.pyplot as plt
import random
import os

def loadGraph(file):
    f = open(file, 'r')
    G = nx.Graph()
    nnodes = 0;
    nedge = 0;
    for line in f.readlines():
        if line.startswith('c'):
            pass
        elif line.startswith('p'):
            parts = line.split()
            nnodes = int(parts[2])
            nedges = int(parts[3])
        elif line.startswith('e'):
            parts = line.split()
            G.add_edge(int(parts[1])-1, int(parts[2])-1)
        else:
            pass
    return G

# g = loadGraph('/home/zhou/workspace/splex-unfix/graph_test.clq')
g = loadGraph('/home/zhou/splex/benchmarks/dimacs/MANN_a9.clq')

# nx.draw_spectral(g)
# plt.show()
# nx.draw_circular(cg)
# nx.draw_spectral(cg)
# nx.draw_graphviz(cg)
# nx.draw_spring(cg)

if 1:
    cg = nx.complement(g, 'complement')
    mis = nx.maximal_independent_set(cg)
    
    print "size ",len(mis)
    print "out num", len(cg) -len(mis)  
    for v in mis:
        print "%d: degree(%d) "%(v, cg.degree(v))
    
    #design the layout
    pos = []
    gap = 0.2
    for v in g.nodes():
        if v in mis:
            pos.append((0.2, gap *1.2))
        else:
            pos.append((random.random() * 0.6+0.6, (gap+0.1)*1.2))
        gap += 0.2
        
    #create labels for each node
    labels={i:str(i) for i in cg.nodes()}
    nx.draw_networkx_labels(cg, pos, labels,font_color='w')
    nx.draw_networkx_nodes(cg, pos, node_color = 'b')
    nx.draw_networkx_nodes(cg, pos, nodelist = mis,
                           node_color = 'r')
    nx.draw_networkx_edges(cg,pos,width=1.0,alpha=0.5)
plt.show()
