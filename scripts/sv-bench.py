import os
import subprocess
import argparse

from time import time
from datetime import datetime
from output_colors import output_colors as oc

# default -----------------------------------------
bench_path = 'benchmarks/sv_bench/'
executable_file = 'sv_bench_executable_file.ll'
result_file = 'results/sv-benchmarks-result.csv'

# arguments ---------------------------------------
parser = argparse.ArgumentParser()
parser.add_argument("--dir-path", "-d", type=str, required=False, dest="bench_path",
					help="Path to test directory.")
parser.add_argument("--test-configurations", "-c", required=False, action='store_true', dest="has_configurations",
					help="Tests have configurations.")

args = parser.parse_args()
bench_path  = args.bench_path
has_configs = args.has_configurations

# define constants --------------------------------
TO = 1800 # 30 mins
#--------------------------------------------------

# initial setup -----------------------------------
os.system('make')

if bench_path[-1] != '/':
    bench_path += '/'
bench_dir   = bench_path.split('/')[-2]

executable_file = executable_file[:-3] + '-' + bench_dir + '.ll'
result_file = result_file[:-4] + '-' + bench_dir + '.csv'

benchdirs = [bench_path]

ignore_tests = ['sigma'] # not working

failed_tests = []
total_tests_completed = 0

command = [
    ['timeout', str(TO)+'s', './src/nidhugg', '--sc', '--optimal',   executable_file], # ODPOR
    ['timeout', str(TO)+'s', './src/nidhugg', '--sc', '--observers', executable_file], # observer-ODPOR
    ['timeout', str(TO)+'s', './src/nidhugg', '--sc', '--view',      executable_file]  # viewEq-SMC
]      

csvfile = open(result_file, 'w')

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
    config = ( file[:-2].split('_conf_')[-1] ).split('-') # file name format: bench_dir_conf_config
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
    if not has_configs:
        return file[:-2]

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
    test_timedout  = [False] * len(command)

    print(oc.PURPLE, '[' + str(datetime.now()) + ']', oc.ENDC, 'Running Test ' + file[:-2])

    os.system('clang -c -emit-llvm -S -o ' + executable_file + ' ' + bench_path + file)
    csv_out = dir + ',' + print_file(file) + ','

    for i in range(len(command)):
        if i in ignore_command:
            print(oc.RED, 'Assumed Timeout (' + algo(i) + ')', oc.ENDC)
            test_timedout[i] = True
            csv_out = csv_out + make_csv_row('TIMEOUT (assumed)', '', '', i)
            continue

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
            test_completed[i] = True
        elif returncode == 42: # assert_fail
            error = 'assert fail'
            test_completed[i] = True
        elif returncode == 124: # timeout
            assert(float(test_time) >= TO)
            print(oc.RED, 'Timeout (' + algo(i) + ')', oc.ENDC)
            csv_out = csv_out + make_csv_row('TIMEOUT', test_time, error, i)
            test_timedout[i] = True
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

    total_tests_completed += test_completed.count(True)
    return test_completed, test_timedout, csv_out

# main --------------------------------------------

while len(benchdirs) > 0:
    tests_completed = 0
    tests_failed    = 0
    tests_timedout  = 0

    if has_configs:
        last_config = {}
        [last_config.setdefault(x,[]) for x in range(len(command))] # {0:[], 1:[], 2:[]}
        last_status_TO = [False] * len(command)

    bench_path = benchdirs.pop()
    if bench_path[-1] != '/':
        bench_path += '/'
    bench_files = [ f for f in os.listdir(bench_path) if f.endswith('.c') ] # no .cc files in sv-benchmarks
    benchdirs  += [ f.path for f in os.scandir(bench_path) if f.is_dir() ]
    bench_dir   = bench_path.split('/')[-2]

    if is_ignore(bench_dir):
        print('Ignoring Test Set ' + bench_dir)
        continue

    print(oc.BLUE, oc.BOLD, 'Entering', bench_path, oc.ENDC)
    
    if has_configs:
        configs = sort_by_config(bench_files)
        bench_files = [ bench_dir + '_conf_' + '-'.join(config) + '.c' for config in configs ]
    for file in bench_files:
        assumed_TO_configs = []
        if has_configs:
            assumed_TO_configs = generate_assumed_TO_configs(get_config(file), last_config, last_status_TO)
    
        test_completed, test_timedout, test_result = run_test(bench_path, file, assumed_TO_configs)
        csvfile.write(test_result)

        tests_completed += test_completed.count(True)
        tests_timedout  += test_timedout.count(True)
        tests_failed    += len(command) - test_completed.count(True) - test_timedout.count(True)

        if has_configs:
            record_TOs(test_timedout, get_config(file), last_status_TO, last_config, assumed_TO_configs)
    
    print(oc.BLUE, oc.BOLD, 'Leaving', bench_path, oc.ENDC)
    print_dir_summary(bench_dir, tests_completed, tests_failed, tests_timedout)

csvfile.close()
os.system('rm ' + executable_file)
print_set_summary()