import os
import sys
import subprocess
from time import time
import shlex

# define constants --------------------------------
TO = 1800 # 30 mins
executable_file = 'sv_bench_executable_file.ll'
#--------------------------------------------------

if len(sys.argv) == 1:
    bench_dir  = 'pthread'
    bench_path = 'benchmarks/sv_bench/' + bench_dir + '/'
else:
    bench_path = sys.argv[1]
    if bench_path[-1] != '/':
        bench_path += '/'
    bench_dir  = bench_path.split('/')[-2]

bench_files = [f for f in os.listdir(bench_path) if f.endswith('.c')] # no .cc files in sv-benchmarks

ignore_tests = ['sigma'] # not working

failed_tests = []
tests_completed = 0

command = [
    './src/nidhugg --sc --optimal ' + executable_file, # ODPOR
    # ['./src/nidhugg', '--sc', '--observers', executable_file], # observer-ODPOR
    # ['./src/nidhugg', '--sc', '--view',      executable_file]  # viewEq-SMC
]      

csvfile = open('sv-benchmarks-result.csv', 'w')

csv_out = ',ODPOR,,,Observer-ODPOR,,,ViewEq\n'
csv_out = csv_out + 'benchmark,#traces,time,assert-fail,error_code,'
csv_out = csv_out + '#traces,time,assert-fail,error_code,'
csv_out = csv_out + '#traces,time,assert-fail,error_code\n'
csvfile.write(csv_out)

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

def run_test(file):
    global tests_completed
    global failed_tests

    print ('Running Test ' + file[:-2])

    os.system('clang -c -emit-llvm -S -o ' + executable_file + ' ' + bench_path + file)
    csv_out = file[:-2] + ','

    for i in range(len(command)):
        start_time = time()
        try:
            (returncode, sout) = subprocess.getstatusoutput(command[i])
            print('rc:', returncode)
        except subprocess.TimeoutExpired:
            test_time = str( time() - start_time )
            csv_out = csv_out + make_csv_row('TIMEOUT', test_time, error, i)
            continue
        except Exception as e:
            print('subprocess error:', e)

        test_time = str( round( time() - start_time , 2 ))
        sout = sout.splitlines()

        trace_count = ''
        error       = ''
        
        if returncode == 0: # OK
            tests_completed += 1
        elif returncode == 42: # assert_fail
            error = 'assert fail'
            tests_completed += 1
        else:
            error_code = str(returncode)
            print (file[:-2] + ' Failed to complete run (' + algo(i) + ')')
            failed_tests.append((file[:-2], 'Failed to complete run'))
            error = 'RUN FAIL(' + error_code + ')'
            csv_out = csv_out + make_csv_row(trace_count, test_time, error, i)
            continue

        for line in sout:
            if 'Trace count:' in line:
                trace_count = line[ line.find('Trace count') + len('Trace count: '): ]

        csv_out = csv_out + make_csv_row(trace_count, test_time, error, i)

    return csv_out

# ---------- main ---------------
for file in bench_files:
    if is_ignore(file):
        print('Ignoring Test ' + file[:-2])
        continue

    test_result = run_test(file)
    # csvfile.write(csv_out)
    print(test_result)

print ('----------------------------------------------')
print ('Test Set sv-benchmarks::' + bench_dir)
print ('----------------------------------------------')
print ('No. of tests run = ' + str(tests_completed))
print ('No. of tests failed = ' + str(len(failed_tests)))
print ('----------------------------------------------\n')

for (idx, msg) in failed_tests:
    print ('Test ' + idx + ': FAIL')
    print ('     ' + msg)    
if len(failed_tests) > 0:
    print ('----------------------------------------------\n\n')

csvfile.close()
os.system('rm ' + executable_file)
