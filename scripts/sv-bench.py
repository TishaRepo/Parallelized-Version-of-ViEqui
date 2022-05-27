from locale import T_FMT_AMPM
import os
import sys
import subprocess
from threading import Timer
from time import time
from datetime import datetime
from output_colors import output_colors as oc

# define constants --------------------------------
TO = 1800 # 30 mins
executable_file = 'sv_bench_executable_file.ll'
#--------------------------------------------------

# initial setup -----------------------------------
if len(sys.argv) == 1:
    bench_dir  = 'pthread'
    bench_path = 'benchmarks/sv_bench/' + bench_dir + '/'
else:
    bench_path = sys.argv[1]
    if bench_path[-1] != '/':
        bench_path += '/'

benchdirs = [bench_path]

ignore_tests = ['sigma'] # not working

failed_tests = []
total_tests_completed = 0

command = [
    ['timeout', str(TO)+'s', './src/nidhugg', '--sc', '--optimal',   executable_file], # ODPOR
    ['timeout', str(TO)+'s', './src/nidhugg', '--sc', '--observers', executable_file], # observer-ODPOR
    ['timeout', str(TO)+'s', './src/nidhugg', '--sc', '--view',      executable_file]  # viewEq-SMC
]      

csvfile = open('results/sv-benchmarks-result.csv', 'w')

csv_out = ',,ODPOR,,,Observer-ODPOR,,,ViewEq\n'
csv_out = csv_out + 'directory,benchmark,#traces,time,error_code,'
csv_out = csv_out + '#traces,time,error_code,'
csv_out = csv_out + '#traces,time,error_code\n'
csvfile.write(csv_out)

# for printing result summary
max_header_length = 45 
min_border_length = 40
#--------------------------------------------------

def algo(n):
    if n == 0:
        return 'ODPOR'
    elif n == 1:
        return 'obs_ODPOR'
    else:
        return 'viewEq'

def is_ignore(filename):
    for prefix in ignore_tests:
        if filename.startswith(prefix):
            return True
    return False

def make_csv_row(trace_count, time, error, i):
    row = trace_count + ',' + time + ',' + error
    if i < 2:
        row = row + ','
    else:
        row = row + '\n'
    
    return row

def print_dir_summary(bench_dir, tests_completed, tests_failed, tests_timedout):
    header = 'Test Set sv-benchmarks::' + bench_dir
    header_length = len(header)
    border_line = '-' * max(header_length, min_border_length)

    print (oc.BLUE, oc.BOLD, border_line, oc.ENDC)
    print (oc.BLUE, oc.BOLD, header, oc.ENDC)
    print (oc.BLUE, oc.BOLD, border_line, oc.ENDC)
    print ('\tNo. of tests completed =', tests_completed)
    print ('\tNo. of tests failed    =', tests_failed)
    print ('\tNo. of tests timedout  =', tests_timedout)
    print (oc.BLUE, oc.BOLD, border_line, '\n', oc.ENDC)

def print_set_summary():
    header = '\t\tTest Set Summary'
    border_line = '=' * max_header_length
    thin_border_line = '-' * max_header_length

    print (oc.YELLOW, oc.BOLD, border_line, oc.ENDC)
    print (oc.YELLOW, oc.BOLD, header, oc.ENDC)
    print (oc.YELLOW, oc.BOLD, border_line, oc.ENDC)
    print ('\tNo. of tests completed =', total_tests_completed)
    print ('\tNo. of tests failed =', len(failed_tests))
    print (oc.YELLOW, oc.BOLD, border_line, oc.ENDC)
    
    for (idx, msg) in failed_tests:
        print ('Test ' + idx + ': FAIL')
        print ('     ' + msg)    
    if len(failed_tests) > 0:
        print (thin_border_line + '\n\n')
    
def run_test(dir, file):
    global total_tests_completed
    global failed_tests

    tests_completed = 0
    tests_timedout  = 0

    print(oc.PURPLE, '[' + str(datetime.now()) + ']', oc.ENDC, 'Running Test ' + file[:-2])

    os.system('clang -c -emit-llvm -S -o ' + executable_file + ' ' + bench_path + file)
    csv_out = dir + ',' + file[:-2] + ','

    for i in range(len(command)):
        start_time = time()
        try:
            p = subprocess.Popen(command[i],
                stdout=subprocess.PIPE,
                stderr=subprocess.DEVNULL,
                bufsize=1, 
                universal_newlines=True)
        except subprocess.TimeoutExpired:
            test_time = str( time() - start_time )
            csv_out = csv_out + make_csv_row('TIMEOUT', test_time, error, i)
            continue
        except Exception as e:
            print(oc.RED, 'subprocess error:', e, oc.ENDC)
            continue

        sout = p.communicate()[0].split('\n')
        returncode = p.wait()
        test_time = str( round( time() - start_time, 3 ) )

        trace_count = ''
        error       = ''
        
        if returncode == 0: # OK
            tests_completed += 1
        elif returncode == 42: # assert_fail
            error = 'assert fail'
            tests_completed += 1
        elif returncode == 124: # timeout
            print(oc.RED, 'Timeout (' + algo(i) + ')', oc.ENDC)
            csv_out = csv_out + make_csv_row('TIMEOUT', test_time, error, i)
            tests_timedout += 1
            continue
        else:
            error_code = str(returncode)
            print (oc.RED, file[:-2] + ' Failed to complete run (' + algo(i) + ')', oc.ENDC)
            failed_tests.append((file[:-2], 'Failed to complete run'))
            error = 'RUN FAIL(' + error_code + ')'
            csv_out = csv_out + make_csv_row(trace_count, test_time, error, i)
            continue

        for line in sout:
            if 'Trace count:' in line:
                trace_count = line[ line.find('Trace count') + len('Trace count: '): ]

        csv_out = csv_out + make_csv_row(trace_count, test_time, error, i)

    total_tests_completed += tests_completed
    return tests_completed, tests_timedout, csv_out

# main --------------------------------------------

while len(benchdirs) > 0:
    tests_completed = 0
    tests_failed    = 0
    tests_timedout  = 0

    bench_path = benchdirs.pop()
    if bench_path[-1] != '/':
        bench_path += '/'
    bench_files = [ f for f in os.listdir(bench_path) if f.endswith('.c') ] # no .cc files in sv-benchmarks
    benchdirs  += [ f.path for f in os.scandir(bench_path) if f.is_dir() ]
    bench_dir   = bench_path.split('/')[-2]

    print(oc.BLUE, oc.BOLD, 'Entering', bench_path, oc.ENDC)
    
    for file in bench_files:
        if is_ignore(file):
            print('Ignoring Test ' + file[:-2])
            continue

        completed_tests, to_tests, test_result = run_test(bench_path, file)
        csvfile.write(test_result)

        tests_completed += completed_tests
        tests_timedout  += to_tests
        tests_failed    += len(command) - completed_tests - to_tests
    
    print(oc.BLUE, oc.BOLD, 'Leaving', bench_path, oc.ENDC)
    print_dir_summary(bench_dir, tests_completed, tests_failed, tests_timedout)

csvfile.close()
os.system('rm ' + executable_file)
print_set_summary()