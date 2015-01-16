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
    except getopt.GetoptError:
        print 'option unrecognized'
        usage()
        sys.exit(2)
    inputfile = None #defialt input file is commands 
    max_core_num = 500      #default number of jobes is 500
    for o, a in opts:
        if o in ('-f','--cmdfile'):
            inputfile = a
        elif o in ("-c", "--core"):
            try:
                max_core_num = int(a)
            except ValueError:
                print 'core must be a specified by a number'
                exit(2)
        else:
            assert False, "unhandled option"
    
    if inputfile is None:
        print 'No input file'
        exit(0)
    cmds_list = file(inputfile,'r').read().splitlines()
    ch = raw_input('Plan to submit %d jobs, continue[Y\N]?'%len(cmds_list))
    
    now = datetime.datetime.now()
    cnt = 1
    record = file('%d%d-%d:%d:%d.log'%(now.month,now.day,now.hour,now.minute,now.second),'w')
    for cmd in cmds_list:
        jobname = 'T%d%d%d_%d'%(now.day,now.hour,now.minute,cnt)
        subtask(jobname, [cmd])    
        record.write('job %s: %s\n'%(jobname,cmd))
        cnt += 1
    record.close()