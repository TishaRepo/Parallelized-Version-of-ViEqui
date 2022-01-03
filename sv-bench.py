import os
import subprocess
import csv

bench_dir = 'pthread'
bench_path = 'benchmarks/sv_bench/' + bench_dir + '/'
bench_files = [f for f in os.listdir(bench_path) if f.endswith('.c')] # no .cc files in sv-benchmarks

mutex_tests = ['indexer', 'lazy01', 'queue', 'stack', 'stateful', 'sync', 'two',
            'sigma'] # not mutex but not working

failed_tests = []
tests_completed = 0

timeout = '300s'
executable_file = 'executable_file.ll'

command = [
    ['timeout', timeout, 'time', 'src/nidhugg', '--sc', '--optimal' , executable_file],     # ODPOR
    ['timeout', timeout, 'time', 'src/nidhugg', '--sc', '--observers' , executable_file],   # observer-ODPOR
    ['timeout', timeout, 'time', 'src/nidhugg', '--sc', '--view' , executable_file]]        # viewEq-SMC

file = open('sv-benchmarks-result.csv', 'w')

csv_out = ',ODPOR,,,,,Observer-ODPOR,,,,,ViewEq\n'
csv_out = csv_out + 'benchmark,#traces,time,error_code,error,warning,'
csv_out = csv_out + '#traces,time,error_code,error,warning,'
csv_out = csv_out + '#traces,time,error_code,error,warning\n'
file.write(csv_out)

def algo(n):
    if n == 0:
        return 'ODPOR'
    elif n == 1:
        return 'obs_ODPOR'
    else:
        return 'viewEq'

def is_ignore(filename):
    for prefix in mutex_tests:
        if filename.startswith(prefix):
            return True
    return False

for file in bench_files:
    if is_ignore(file):
        print('Ignoring Test ' + file[:-2])
        continue

    print ('Running Test ' + file[:-2])

    os.system('clang -c -emit-llvm -S -o ' + executable_file + ' ' + bench_path + file)
    csv_out = file[:-2] + ','

    for i in range(len(command)):
        print(i)
        process = subprocess.Popen(command[i], 
                    stdout=subprocess.PIPE, 
                    stderr=subprocess.STDOUT, 
                    universal_newlines=True)

        sout = process.stdout.readlines()

        run_fail    = True
        trace_count = ''
        time        = ''
        warning     = ''
        error       = ''
        errorcode   = ''

        if str(process.returncode) != 'None':
            error_code = str(process.returncode)

        for line in sout:
            if 'Trace count:' in line:
                print(line)
                trace_count = line[line.find('Trace count') + len('Trace count: '):-1]

            elif line.startswith('No errors'):
                run_fail = False

            elif line.startswith('Error detected') or line.startswith(' Error detected'):
                run_fail = False
                error = 'Y'
                # error = error + line

            elif line.startswith('ERROR'):
                run_fail = True
                error = 'Y'
                # error = error + line

            elif line.startswith('WARNING'):
                warning = 'Y'
                # warning = warning + line

            elif 'elapsed' in line:
                time = line[line.find('system') + len('system ') :  line.find('elapsed')]        

        if (run_fail):
            print (file[:-2] + ' Failed to complete run (' + algo(i) + ')')
            failed_tests.append((file[:-2], 'Failed to complete run'))

        tests_completed = tests_completed + 1
        csv_out = csv_out + trace_count + ',' + time + ',' + errorcode + ',' + error + ',' + warning
        if i < 2:
            csv_out = csv_out + ','
        else:
            csv_out = csv_out + '\n'

    # ####
    # print('traces:' + trace_count + ', time:' + time)
    # break
    # ####
    file.write(csv_out)

os.system('rm ' + executable_file)

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

file.close()

# print('csv:')
# rows = csv_out.split('\n')
# header = rows[0]
# rows   = rows[1:]

# header_fields = header.split(',')
# print("%30s"%header_fields[0] + " | " +
#        "%7s"%header_fields[1] + " | " +
#       "%10s"%header_fields[2] + " | " +
#        "%9s"%header_fields[3])

# for row in rows:
#     print(row)
#     col = row.split(',')
#     print("%30s"%col[0] + " | " +
#            "%7s"%col[1] + " | " +
#           "%20s"%col[2] + " | " +
#            "%9s"%col[3])