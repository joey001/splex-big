'''
Created on Jan 12, 2015
This scipt generate running files
@author: zhou
'''
max_value = 99999
org_table = '''MANN_a9.clq 45 918 92.73 16 26 36 36 44
MANN_a27.clq 378 70551 99.01 126 *235 351 *350 *350
brock200_1.clq 200 14834 74.54 21 *24 *23 *23 *23
brock200_2.clq 200 9876 49.63 12 13 16 *17 *16
brock200_3.clq 200 12048 60.54 15 17 *19 *19 *19
brock200_4.clq 200 13089 65.77 17 20 *20 *20 *19
brock400_1.clq 400 59723 74.84 27 *23 *22 *22 *21
brock400_2.clq 400 59786 74.92 29 *23 *22 *21 *21
c-fat200-1.clq 200 1534 7.71 12 12 12 12 14
c-fat200-2.clq 200 3235 16.26 24 24 24 24 24
c-fat200-5.clq 200 8473 42.58 58 58 58 58 58
c-fat500-1.clq 500 4459 3.57 14 14 14 14 15
c-fat500-10.clq 500 46627 37.38 126 126 126 126 126
c-fat500-2.clq 500 9139 7.33 26 26 26 26 26
c-fat500-5.clq 500 23191 18.59 64 64 64 64 64
hamming6-2.clq 64 1824 90.48 32 32 32 40 48
hamming6-4.clq 64 704 34.92 4 6 8 10 12
hamming8-2.clq 256 31616 96.86 128 128 128 *45 *60
hamming8-4.clq 256 20864 63.92 16 16 *20 *18 *18
johnson16-2-4.clq 120 5460 76.47 8 10 *16 *19 *21
johnson8-2-4.clq 28 210 55.56 4 5 8 9 12
johnson8-4-4.clq 70 1855 76.81 14 14 18 22 *24
keller4.clq 171 9435 64.91 11 15 21 *22 *20
p_hat300-1.clq 300 10933 24.38 8 10 12 14 *13
p_hat300-2.clq 300 21928 48.89 25 30 *30 *19 *18
p_hat500-1.clq 500 31569 25.31 9 12 14 *13 *14
adjnoun.graph 112 425 6.8371 5 6 8 8 10
as-22july06.graph 22963 48436 0.0183 17 19 21 22 24
astro-ph.graph 16706 121251 0.0868 57 57 57 57 57
caidaRouterLevel.graph 192244 609066 0.0032 17 20 *22 *23 *22
celegans_metabolic.graph 453 2025 1.9779 9 10 11 13 14
Cit-HepPh.graph 34546 420877 0.0705 19 24 *25 *18 *15
Cit-HepTh.graph 27770 352285 0.0913 23 28 *31 *29 *19
cnr-2000.graph 325557 2738969 0.0051 84 85 86 86 86
coAuthorsCiteseer.graph 227320 814134 0.0031 87 87 87 87 87
coAuthorsDBLP.graph 299067 977676 0.0021 115 115 115 115 115
cond-mat-2005.graph 40421 175691 0.0215 30 30 30 30 30
dolphin.graph 62 159 8.4082 5 6 7 7 9
email.graph 1133 5451 0.85 12 12 12 12 13
Email-EuAll.graph 265214 364481 0.001 16 19 *22 *22 *27
football.graph 115 613 9.3516 9 10 11 12 12
jazz.graph 198 2742 14.0593 30 30 30 30 30
karate.graph 34 78 13.9037 5 6 6 8 11
memplus.graph 17758 54196 0.0343 97 97 97 97 97
netscience.graph 1589 2742 0.2173 20 20 20 20 *19
p2p-Gnutella04.graph 10876 39994 0.0676 4 5 *5 *6 *7
p2p-Gnutella24.graph 26518 65369 0.0185 4 5 *5 *5 *5
p2p-Gnutella25.graph 22687 54705 0.0212 4 5 *5 *5 *5
PGPgiantcompo.graph 10680 24316 0.0426 25 29 31 33 35
polblogs.graph 1490 16715 1.5067 20 23 27 29 *32
polbooks.graph 105 441 8.0769 6 7 9 10 11
power.graph 4941 6594 0.054 6 6 6 *6 *8
rgg_n_2_17_s0.graph 131072 728753 0.0084 15 16 17 18 *17
rgg_n_2_19_s0.graph 524288 3269766 0.0023 18 19 19 20 *15
rgg_n_2_20_s0.graph 1048576 6891620 0.0012 17 18 19 *15 *14
Slashdot0811.graph 77360 469180 0.0156 26 31 *33 *36 *35
Slashdot0902.graph 82168 504230 0.0149 27 32 *35 *36 *35
soc-Epinions1.graph 75879 405740 0.014 23 28 *25 *27 *22
web-BerkStan.graph 685230 6649470 0.0028 201 202 202 202 202
web-Google.graph 875713 4322051 0.0011 44 46 47 48 48
web-NotreDame.graph 325729 1090108 0.002 155 155 155 155 155
web-Stanford.graph 281903 1992636 0.005 61 64 64 *64 *64
Wiki-Vote.graph 7115 100762 0.3981 17 21 *24 *26 *27
'''
config={}
strans = lambda s: max_value if s.startswith('*') else int(s)
for line in org_table.splitlines():
    filename, nodenum, edgenum, density, s1, s2, s3, s4, s5 = line.split(' ')
    nodenum = int(nodenum)
    edgenum = int(edgenum)
    density = float(density)
    s1 = strans(s1)
    s2 = strans(s2)
    s3 = strans(s3)
    s4 = strans(s4)
    s5 = strans(s5)
    fileinfo = {'node':nodenum, 'edge':edgenum, 'desity':density, 's1':s1, \
                's2':s2,'s3':s3,'s4':s4,'s5':s5}
    config[filename] = fileinfo
Baulas_list = ['MANN_a27.clq','MANN_a9.clq','brock200_1.clq',
               'c-fat200-1.clq','c-fat200-2.clq','c-fat500-1.clq',
               'c-fat500-10.clq','c-fat500-2.clq','c-fat500-5.clq',
               'hamming6-2.clq','hamming6-4.clq','hamming8-2.clq',
               'hamming8-4.clq','johnson16-2-4.clq','johnson8-2-4.clq',
               'johnson8-4-4.clq','keller4.clq']
#-----------------------------------config----------------------------------

import os.path
#load config
# srange=[1,2,3,4,5]
srange=[1,2,3,4,5]
ins_dir = "/home/zhou/benchmarks/"
runcnt = 10
command_file = 'commands.txt'


insfilter = lambda x: x.endswith('clq') or x.endswith('graph')
ins_files = filter(insfilter, os.listdir(ins_dir))
outf = open(command_file,'w+')
#outf.write('#format:[H\E] ./splex [-f filename] [-s plex] [-r runcount] [-b bes_known]\n')
# for f in ins_files:
for f in Baulas_list:
    if f not in config:
        print '%s not exist,ignore'%f
    else:
        fpath = os.path.join(ins_dir, f)
        for s in srange:
            #tasks whose best value are max_value are believed to be hard tasks,(or time consuming tasks)
            pre_time = 'E'
            if config[f]['s%d'%s] == max_value:
                pre_time = 'H'
            command =  '%s ./splex -f %s -s %d -r %d -b %d \n'%\
            (pre_time, fpath, s, runcnt, config[f]['s%d'%s])
            outf.write(command)
outf.close()
                                                        



