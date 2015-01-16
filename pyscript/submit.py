'''
Created on Jan 12, 2015
This script submit the jobs to our cluster
submit [-f filename] 
-f filename: the file stores all commands  
@author: zhou
'''
import os.path
import sys
import subprocess
import datetime

# submit a task with content inside
def subtask(jobname, content_list):
    shfilename = '%s.sh'%jobname
    shf = open(shfilename,'w')
    shf.writelines('#!/bin/bash\n')
    shf.writelines('chmod u+x splex\n')
    shf.write('\n'.join(content_list))
    shf.close()
    print '----create a job file %s----'%jobname
     
    #Given the user executive authority
    cmd1 = ['chmod','u+x',os.path.join(os.getcwd(), shfilename)]
    rt = subprocess.call(cmd1)
    if rt is not 0:
        print 'no execution authority is granted'
        return rt
    
    #submit the shell file to specified core  
    
    #absolute path must be set
    cmd2 = ['qsub','-q','*@@intel-E5440','-cwd',os.path.join(os.getcwd(), shfilename)]
    rt = 1
    try:
        print '-----submit %s------'%(' '.join(cmd2))
        rt = subprocess.call(cmd2)
    except: 
        print 'fail submit %s'%jobname
    else:
        print "successful submit %s"%jobname
    os.remove(shfilename)
    return rt

#Calcute the number of cores    
import getopt
def usage():
    print 'submit [-f filename]'
if __name__ == '__main__':
    #read parameters
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'f:c:', ['cmdfile=','co'])
    except getopt.GetoptError as err:
        print str(err) # will print something like "option -a not recognized"
        usage()
        sys.exit(2)
    inputfile = 'commands.txt' #defialt input file is commands 
    max_core_num = 500      #default number of jobes is 500
    for o, a in opts:
        if o in ('-f','--cmdfile'):
            inputfime = a
        elif o in ("-c", "--core"):
            try:
                max_core_num = int(a)
            except ValueError:
                print 'core must be a specified by a number'
                exit(2)
        else:
            assert False, "unhandled option"
    
    
    cmds_list = file(inputfile,'r').read().splitlines()
    ch = raw_input('Plan to submit %d jobs, continue[Y\N]?'%len(cmds_list))

    # deploy tasks
    hard_cmds = [line[2:] for line in cmds_list if line.startswith('H')]
    easy_cmds = [line[2:] for line in cmds_list if line.startswith('E')]
    #default core_number is 20,to balance the load of each core, one core only contain one hard tasks
    #if no more than 20 tasks in total, 
    if len(hard_cmds) > max_core_num:
        core_num = len(hard_cmds)
    elif len(cmds_list) < max_core_num:
        core_num = len(cmds_list)
        
    #prompt to get continue
    
    if ch !='' and (ch[0] == 'y' or ch[0] == 'Y'):
        pass
    else:
        print 'submit abort'
        sys.exit(0)
    
    #deploy, try to balance the   
    now = datetime.datetime.now()
    record = file('z%d%d-%d:%d:%d.record'%(now.month,now.day,now.hour,now.minute,now.second),'w')
    
    hard_index = 0
    easy_index = 0
    basic_num = len(cmds_list) / core_num
    rest = len(cmds_list) % core_num
    for no_core in range(core_num): 
        if no_core < rest:
            task_num = basic_num+1
        else:
            task_num = basic_num
        #ensure at most one hard task is deployed on each core
        #ensure the hard task is deployed at the end of the core
        content_list = []
        while easy_index < len(easy_cmds) and len(content_list) < task_num -1:
            content_list.append(easy_cmds[easy_index])
            easy_index+=1
        if hard_index < len(hard_cmds):
            content_list.append(hard_cmds[hard_index])
            hard_index+=1
        else:
            content_list.append(easy_cmds[easy_index])
            easy_index+=1
        jobname = 'j%d%d%d_%d'%(now.day,now.hour,now.minute,no_core+1)
        rt = subtask(jobname, content_list)
        
        record.write('job %s\n'%jobname)
        for line in content_list:
            record.write(line+'\n')
    record.close()
        
