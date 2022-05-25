import os
import subprocess
import sys
import time

N = 3 # no. of runs for average of time
TO = 60 # in seconds

if len(sys.argv) < 2:
    print('no test path')
    sys.exit(0)
test_path = sys.argv[1]
if test_path[-1] == '/':
    test_path = test_path[:-1]

fileslist = ''
if len(sys.argv) < 3: # no list of files provided
    fileslist = test_path + '/listfile.txt'
    if not os.path.isfile(fileslist):
        f = open(fileslist, 'w')
        for file in os.listdir(test_path):
            f.write(file + '\n')
        f.close()
else:
    fileslist = sys.argv[2]

output = test_path + '/' + test_path.split('/')[-1] + '.csv'
current_path = os.getcwd() + '/'

# litmus_path = current_path + 'tests/litmus/'
# test_path = litmus_path + 'C-tests/'
# output = os.getcwd() + '/view_litmus.csv'
# fileslist = litmus_path + 'listfile.txt'

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

def flags(algo, check_optimality):
    if algo == 1:
        return '--sc --optimal'
    if algo == 2:
        return '--sc --observers'
    # algo = 3
    if check_optimality:
        return '--sc --view --check-optimality'
    return '--sc --view'

def read_error(lines):
    errors = ''
    for line in lines:
        if 'Error' in line:
            errors += line

    if(len(lines) != 0 and errors == ''): errors = lines[-1]
    errorList = errors.split(',')
    errors = ' '.join(errorList)

    return errors

def read_optimality_result(lines):
    for line in lines:
        if 'Optimal Exploration' in line:
            return '1'
        elif 'Redundant explorations' in line:
            return '0'
    return ''

def read_result(lines, algo):
    trace_count = ''
    cmnt = ''
    
    for line in lines:
        if 'Trace count' in line :
            trace_count = line[line.find('Trace count') + len('Trace count: '):]
            trace_count = trace_count.strip()
        if algo == 3:
            cmnt = ','
            if 'snj' in line:
                cmnt += line.strip() + ' '
        
    return trace_count, cmnt

def run_one_test( logfile, algo, check_optimality ):
    application = current_path + 'src/nidhugg'
    executable  = current_path + 'executable_file.ll'
    cmd = 'timeout ' + str(TO) + 's '+ application + ' ' + flags(algo, check_optimality) + ' ' + executable
    
    trace_count = ''
    cmnt = ''
    error = ''

    exn_times = []
    exn_time  = 0
    for i in range(N):
        end_time = 0
        start_time = time.time()
        try:
            #process = subprocess.run([current_path + 'src/nidhugg', '--sc', '--view',current_path+ 'executable_file.ll'], bufsize=1, universal_newlines=True, timeout = 60.0)
            (status, sout) = subprocess.getstatusoutput(cmd)
            end_time = time.time()
        except subprocess.TimeoutExpired:
            end_time = TO + start_time
            # return  '' + ',' + '' + ',' + 'Timeout' + ',' + '124'
        exn_times.append(end_time - start_time)

        lines = sout.splitlines()

        if check_optimality:
            return read_optimality_result(lines)

        if status != 0 and status != 42: # 0:OK 42:ASSERT FAIL
            error = read_error(lines)
            return ',' + str(exn_times[i]) + ',' + error.strip() + ',' + str(status)

        if i == 0:
            if status == 42:
                error = read_error(lines)
            trace_count, cmnt = read_result(lines, algo)
    
    exn_time = sum(exn_times)/N
    if exn_time >= N*TO:
        error = 'timeout'

    return trace_count + ',' + str(exn_time) + ',' + error + ',0' + cmnt    

def run_tests():
    f = open(fileslist, 'r')
    testfiles = f.readlines()
    f.close()

    logfile = open(output, 'w')
    logfile.write('File name,Default Trace Count,Default time,Default Error,Default exit status,')
    logfile.write('Observers Trace Count,Observers time, Observers Error,Observers exit status,')
    logfile.write('View Trace Count,View time,View Error,View exit status,View Optimal,Comments\n')
    counter = 0

    for testfile in testfiles:
        if not '.c' in testfile:
            continue

        counter+=1

        testfile = testfile.strip()
        logfile.write(testfile + ',')

        print('Test#', counter)
        print(testfile + '\n')
        os.system('clang -c -emit-llvm -S -o executable_file.ll ' + test_path + '/' + testfile)

        default_stats = run_one_test(logfile, 1, False)
        #print(default_stats)
        logfile.write(default_stats + ',')

        observers_stats = run_one_test(logfile, 2, False)
        #print(observers_stats)

        logfile.write(observers_stats + ',')

        view_stats = run_one_test(logfile, 3, False)
        #print(view_stats)
        cmnt = view_stats.split(',')[-1]
        log = view_stats[:-1*len(',' + cmnt)]

        optimality = run_one_test(logfile, 3, True)
        #print(view_stats)
        logfile.write(log + ',' + optimality + ',' + cmnt + '\n')

        os.system('rm ' + current_path + 'executable_file.ll')
    logfile.close()


# def run_litmus():
#     testfiles = open(fileslist, 'r')
#     logfile = open(output, 'a')
#     counter = 0
    
#     while counter < 1 :
#         ln = testfiles.readline()
        
#         ln = ln.strip()
#         logfile.write(ln+',')

#         os.system('clang -c -emit-llvm -S -o executable_file.ll ' + litmus_path + ln)

#         default_stats = run_one_test(logfile, 1)
#         logfile.write(default_stats+',')

#         observers_stats = run_one_test(logfile, 2)
#         logfile.write(observers_stats+',')

#         view_stats = run_one_test(logfile, 3)
#         logfile.write(view_stats+'\n')

#         counter+=1
#         os.system('rm ' + current_path + 'executable_file.ll')
#         os.system('echo Test number ' + str(counter) + '\n')
#     logfile.close()

###################################################################################

# files = getFilesList('tests/litmus/C-tests/')
# filelist = open(current_path + 'filelist.txt', 'w')
# files.sort()
# for entry in files:
#    print(entry)
#    filelist.write(entry[entry.find('Litmus') + 7:] + '\n')

os.system('make')
run_tests() 
