import os, sys
import subprocess
from telnetlib import NOP

from time import time
from datetime import datetime
from output_colors import output_colors as oc

# constants -----------------------------------------
bench_path = 'benchmarks/ViewEqBenchmarks/'
executable_file = 'vieweq_bench_executable_file.ll'
result_file = 'results/vieweq-benchmarks-result.csv'
TO = 1800 # Timeout 30 mins
TR = 5    # Test-rounds run each test 5 times and take average
#--------------------------------------------------

# initial setup -----------------------------------
os.system('make')

benchdirs = [bench_path]

failed_tests = []
total_tests_completed = 0

command = [
    ['timeout', str(TO)+'s', './src/nidhugg', '--sc', '--optimal',   executable_file], # ODPOR
    ['timeout', str(TO)+'s', './src/nidhugg', '--sc', '--observers', executable_file], # observer-ODPOR
    ['timeout', str(TO)+'s', './src/nidhugg', '--sc', '--view',      executable_file]  # viewEq-SMC
]      

csvfile = open(result_file, 'w')

csv_out = ',,ODPOR,,,,Observer-ODPOR,,,,ViewEq\n'
csv_out = csv_out + 'directory,benchmark,'
csv_out = csv_out + '#traces,time,error_code,TO+F+C,'
csv_out = csv_out + '#traces,time,error_code,TO+F+C,'
csv_out = csv_out + '#traces,time,error_code,TO+F+C\n'
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

def make_csv_row(trace_count, time, error, status, i):
    row = trace_count + ',' + time + ',' + error + ',' + status
    if i < 2:
        row = row + ','
    else:
        row = row + '\n'
    
    return row

def generate_assumed_TO_configs(config, last_config, last_status_TO): 
    assumed_TO_configs = []

    for i in last_config: # last TO config of i.th command
        assert(len(last_config[i]) == 0 or len(config) == len(last_config[i][0])) # assert same config format

        if last_status_TO[i]: # an earlier config TOed
            strictly_larger_test = False # larger than atleast one TOed config
            for j in range(len(last_config[i])): # for each incomparable TO config
                strictly_larger_test_than_current = True # new config is strictly larger than current TOed config
                for k in range(len(last_config[i][j])): # for each parameter of last_config[i][j]
                    if int(last_config[i][j][k]) > int(config[k]):
                        strictly_larger_test_than_current = False
                        break
        
                if strictly_larger_test_than_current:
                    strictly_larger_test = True # new config is strictly larger a TOed config
                    break
                    
            if strictly_larger_test: # new config is strictly larger a TOed config
                assumed_TO_configs.append(i) # this config will also TO, so do not test
    
    return assumed_TO_configs

def get_config(file):
    file_ext = '.' + file.split('.')[1]
    config = ( file[:-1*len(file_ext)].split('_conf_')[-1] ).split('-') # file name format: bench_dir_conf_config
    config = [int(x) for x in config]
    return config

def sort_by_config(bench_files):
    configs = []
    for file in bench_files:
        configs.append(get_config(file))
    return [ [str(x) for x in y] for y in sorted(configs) ]

def record_TOs(tests_timedout, config, last_status_TO, last_config, assumed_TO_configs):
    for i in range(len(tests_timedout)): 
        if tests_timedout[i]: # command timedout on current config
            if not i in assumed_TO_configs: # TO after run (not assumed)
                last_status_TO[i] = True
                last_config[i].append(config)

def print_file(file):
    test_name = file.split('_conf_')[0]
    config = get_config(file)
    config_str = '(' + '.'.join([str(x) for x in config]) + ')'
    return test_name + config_str

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
    
def run_test(dir, file, ignore_command=[]):
    global total_tests_completed
    global failed_tests

    test_completed = [False] * len(command)
    test_partial   = [False] * len(command)
    test_timedout  = [False] * len(command)
    count_failed   = [0] * len(command)
    count_timeout  = [0] * len(command)

    print(oc.PURPLE, '[' + str(datetime.now()) + ']', oc.ENDC, 'Running Test ' + file.split('.')[0])

    os.system('clang -c -emit-llvm -S -o ' + executable_file + ' ' + bench_path + file)
    csv_out = dir + ',' + print_file(file) + ','

    for i in range(len(command)):
        total_time = 0
        trace_count = ''
        error = ''

        if i in ignore_command:
            print(oc.RED, 'Assumed Timeout (' + algo(i) + ')', oc.ENDC)
            test_timedout[i] = True
            csv_out = csv_out + make_csv_row('TIMEOUT (assumed)', '', '', '', i)
            continue

        for j in range(TR):
            if count_timeout[i] > 1: # if 2 runs timedout, assume rest will timeout
                count_timeout[i] += 1
                continue
            if  count_failed[i] > 1: # if 2 runs failed, assume rest will fail
                count_failed[i] += 1
                continue

            start_time = time()
            try:
                p = subprocess.Popen(command[i],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.DEVNULL,
                    bufsize=1, 
                    universal_newlines=True)
            except subprocess.TimeoutExpired:
                print(oc.RED, 'Timeout (' + algo(i) + ', round' + str(j) + ')', oc.ENDC)
                count_timeout[i] += 1
                continue
            except Exception as e:
                print(oc.RED, 'subprocess error:', e, oc.ENDC)
                continue

            sout = p.communicate()[0].split('\n')
            returncode = p.wait()
            test_time = round( time() - start_time, 3 )

            round_trace_count = ''
            round_error = ''
        
            if returncode == 0: # OK
                total_time += test_time
            elif returncode == 42: # assert_fail
                round_error = 'assert fail'
                total_time += test_time
                if j == 0:
                    error = round_error
                else:
                    assert(error == round_error)
            elif returncode == 124: # timeout
                assert(test_time >= TO)
                print(oc.RED, 'Timeout (' + algo(i) + ', round' + str(j) + ')', oc.ENDC)
                count_timeout[i] += 1
                continue
            else:
                error_code = str(returncode)
                print('error code:', error_code)
                print (oc.RED, file.split('.')[0] + ' Failed to complete run (' + algo(i) + ', round' + str(j) + ')', oc.ENDC)
                error += 'RUN FAIL(' + error_code + ')'
                count_failed[i] += 1
                continue

            for line in sout:
                if 'Trace count:' in line:
                    round_trace_count = line[ line.find('Trace count') + len('Trace count: '): ]
                if j == 0:
                    trace_count = round_trace_count
                else:
                    assert(trace_count == round_trace_count)

        if count_timeout[i] == TR:
            csv_out = csv_out + make_csv_row('TIMEOUT', str(total_time), error, '5+0+0', i)
            test_timedout[i] = True
            continue

        if count_failed[i] == TR:
            csv_out = csv_out + make_csv_row(trace_count, str(total_time), error, '0+5+0', i)
            failed_tests.append((file.split('.')[0], 'Failed to complete run'))
            continue

        count_completed = TR - count_timeout[i] - count_failed[i]
        status = str(count_timeout[i]) + '+' + str(count_failed[i]) + '+' + str(count_completed)
        total_time = total_time / count_completed
        
        if count_timeout[i] == 0 and count_failed[i] == 0:
            test_completed[i] = True
        else:
            test_partial[i] = True

        csv_out = csv_out + make_csv_row(trace_count, str(total_time), error, status, i)

    total_tests_completed += test_completed.count(True)
    return test_completed, test_partial, test_timedout, csv_out

# main --------------------------------------------

while len(benchdirs) > 0:
    tests_completed = 0
    tests_partial   = 0
    tests_failed    = 0
    tests_timedout  = 0

    last_config = {}
    [last_config.setdefault(x,[]) for x in range(len(command))] # {0:[], 1:[], 2:[]}
    last_status_TO = [False] * len(command)

    bench_path = benchdirs.pop()
    if bench_path[-1] != '/':
        bench_path += '/'
    bench_files = [ f for f in os.listdir(bench_path) if f.endswith('.c') or f.endswith('.cc') ]
    benchdirs  += [ f.path for f in os.scandir(bench_path) if f.is_dir() ]
    bench_dir   = bench_path.split('/')[-2]
    if len(bench_files) > 0:
        file_ext = '.' + bench_files[0].split('.')[1] # dir has c/cc files

    print(oc.BLUE, oc.BOLD, 'Entering', bench_path, oc.ENDC)
    
    configs = sort_by_config(bench_files)
    bench_files = [ bench_dir + '_conf_' + '-'.join(config) + file_ext for config in configs ]
    for file in bench_files:
        assumed_TO_configs = generate_assumed_TO_configs(get_config(file), last_config, last_status_TO)
    
        test_completed, test_partial, test_timedout, test_result = run_test(bench_path, file, assumed_TO_configs)
        csvfile.write(test_result)

        tests_completed += test_completed.count(True)
        tests_partial   += test_partial.count(True)
        tests_timedout  += test_timedout.count(True)
        tests_failed    += len(command) - test_completed.count(True) - test_timedout.count(True) - test_partial.count(True)

        record_TOs(test_timedout, get_config(file), last_status_TO, last_config, assumed_TO_configs)
    
    print(oc.BLUE, oc.BOLD, 'Leaving', bench_path, oc.ENDC)
    print_dir_summary(bench_dir, tests_completed, tests_failed, tests_timedout)

csvfile.close()
os.system('rm ' + executable_file)
print_set_summary()

print(oc.RED, "TODO: compute no of events per trace.", oc.ENDC)
