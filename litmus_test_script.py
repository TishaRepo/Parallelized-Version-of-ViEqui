import os
import subprocess
import time

current_path = os.getcwd() + '/'
litmus_path = current_path + 'tests/litmus/'
test_path = litmus_path + 'C-tests-neg/'
output = os.getcwd() + '/view_litmus_neg.csv'
fileslist = litmus_path + 'listfile.txt'

def getFilesList( dirName ):
    listOfFile = os.listdir(dirName)
    files = list()
    for entry in listOfFile:
        fullpath = os.path.join(dirName, entry)
        if os.path.isdir(fullpath):
            files = files + getFilesList(fullpath)
        elif fullpath.endswith('.c'):
            files.append(fullpath)
    return files

def run_one_test( logfile, algo ):
    t0 = time.time()
    try:
      if (algo == 1): 
          #process = subprocess.run([current_path + 'src/nidhugg', '--sc', current_path+ 'executable_file.ll'], stdout = subprocess.PIPE, stderr = subprocess.STDOUT, bufsize=1, universal_newlines=True, timeout = 60.0)
          (status, sout) = subprocess.getstatusoutput('timeout 60s '+ current_path + 'src/nidhugg --sc '+ current_path+ 'executable_file.ll')
      elif(algo == 2):
          #process = subprocess.run([current_path + 'src/nidhugg', '--sc','--observers', current_path+ 'executable_file.ll'], stdout = subprocess.PIPE, stderr = subprocess.STDOUT, bufsize=1, universal_newlines=True, timeout = 60.0)
          (status, sout) = subprocess.getstatusoutput('timeout 60s ' + current_path + 'src/nidhugg --sc --observers '+ current_path+ 'executable_file.ll')
          #process = subprocess.Popen([current_path + 'src/nidhugg', '--sc','--view', current_path + 'executable_file.ll'], stdout=subprocess.PIPE, bufsize=1, universal_newlines=True)
      elif(algo == 3):
          #process = subprocess.run([current_path + 'src/nidhugg', '--sc', '--view',current_path+ 'executable_file.ll'], bufsize=1, universal_newlines=True, timeout = 60.0)
          (status, sout) = subprocess.getstatusoutput('timeout 60s '+ current_path + 'src/nidhugg --sc --view --check-optimality '+ current_path+ 'executable_file.ll')
    except subprocess.TimeoutExpired:
      return  '' + ',' + '' + ',' + 'Time out' + ',' + '124'
    exn_time = time.time() - t0
    test_run = False
    trace_count = ''
    errors = ''
    cmnt = ''
    optimal = ' '
    #sout = process.stdout
    #print(sout)
    
    lines = sout.splitlines()
    for line in lines:
        if 'Trace count' in line :
            test_run = True
            trace_count = line[line.find('Trace count') + len('Trace count: '):]
            trace_count = trace_count.strip()
        if algo == 3 and 'snj' in line :
            cmnt = cmnt + line.strip() + " "
        if 'Error' in line:
            errors += line
        if algo == 3 and 'Optimal Exploration' in line:
            optimal = '1'
            #print(optimal)
        elif algo == 3 and 'Redundant explorations' in line:
            optimal = '0'
            #print(optimal)
    
    if(status != 0 and len(lines) != 0 and errors == ''): errors = lines[-1]
    errorList = errors.split(',')
    errors = ' '.join(errorList)
    if algo == 3 : return trace_count + ',' + str(exn_time) + ',' + errors.strip() + ',' + str(status) + ',' + optimal + ',' + cmnt
    return trace_count + ',' + str(exn_time) + ',' + errors.strip() + ',' + str(status)

def run_tests(start = 0, end = 8065):
    testfiles = open(fileslist, 'r')
    logfile = open(output, 'a')
    if(start == 0):
        logfile.write('File name,Default Trace Count,Default time,Default Error,Default exit status,')
        logfile.write('Observers Trace Count,Observers time, Observers Error,Observers exit status,View Trace Count,View time,View Error,View exit status,View Optimal,Comments\n')
    counter = 0
    
    while counter < end :
        if(counter < start):
            ln = testfiles.readline()
            counter+=1
            continue

        counter+=1
        ln = testfiles.readline()
        ln = ln.strip()
        logfile.write(ln+',')

        os.system('clang -c -emit-llvm -S -o executable_file.ll ' + test_path + ln)

        default_stats = run_one_test(logfile, 1)
        #print(default_stats)
        logfile.write(default_stats+',')

        observers_stats = run_one_test(logfile, 2)
        #print(observers_stats)

        logfile.write(observers_stats+',')

        view_stats = run_one_test(logfile, 3)
        #print(view_stats)
        logfile.write(view_stats+'\n')

        os.system('rm ' + current_path + 'executable_file.ll')
        os.system('echo Test number ' + str(counter) + '\n')
    logfile.close()


def run_litmus():
    testfiles = open(fileslist, 'r')
    logfile = open(output, 'a')
    counter = 0
    
    while counter < 1 :
        ln = testfiles.readline()
        
        ln = ln.strip()
        logfile.write(ln+',')

        os.system('clang -c -emit-llvm -S -o executable_file.ll ' + litmus_path + ln)

        default_stats = run_one_test(logfile, 1)
        logfile.write(default_stats+',')

        observers_stats = run_one_test(logfile, 2)
        logfile.write(observers_stats+',')

        view_stats = run_one_test(logfile, 3)
        logfile.write(view_stats+'\n')

        counter+=1
        os.system('rm ' + current_path + 'executable_file.ll')
        os.system('echo Test number ' + str(counter) + '\n')
    logfile.close()

###################################################################################

# files = getFilesList('tests/litmus/C-tests/')
# filelist = open(current_path + 'filelist.txt', 'w')
# files.sort()
# for entry in files:
#    print(entry)
#    filelist.write(entry[entry.find('Litmus') + 7:] + '\n')

os.system('make')
run_tests(0) 
