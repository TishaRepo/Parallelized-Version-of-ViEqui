import os
import subprocess
import csv

bench_dir = 'pthread_temp'
bench_path = 'benchmarks/sv-benchmarks/c/' + bench_dir + '/'
bench_files = [f for f in os.listdir(bench_path) if f.endswith('.c')] # no .cc files in sv-benchmarks

failed_tests = []
tests_completed = 0

csv_out = 'benchmark,#traces,time,error_code,error,warning\n'

for file in bench_files:
    print ('Running Test ' + file[:-2])

    os.system('clang -c -emit-llvm -S -o executable_file.ll ' + bench_path + file)
    process = subprocess.Popen(['timeout', '30s', 'time', 'src/nidhugg', '--sc' , 'executable_file.ll'], 
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

        elif line.startswith('Error detected'):
            run_fail = False
            error = error + line

        elif line.startswith('ERROR'):
            run_fail = True
            error = error + line

        elif line.startswith('WARNING'):
            warning = warning + line

        elif 'elapsed' in line:
            time = line[line.find('system') + len('system ') :  line.find('elapsed')]        

    if (run_fail):
        print file[:-2] + ' Failed to complete run'
        failed_tests.append((file[:-2], 'Failed to complete run'))

    tests_completed = tests_completed + 1
    csv_out = csv_out + file[:-2] + ',' + trace_count + ',' + time + ',' + errorcode + ',' + error + ',' + warning + '\n'

    # ####
    # print('traces:' + trace_count + ', time:' + time)
    # break
    # ####

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

file = open('sv-benchmarks-result.csv', 'w')
file.write(csv_out)
file.close

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