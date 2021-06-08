# Sample output tests/litmus/C-tests/iriwv2.c with --sc
#   Trace count: 15
#   No errors were detected.
#
# Sample output tests/litmus/C-tests/iriwv2.c with --arm
#   Trace count: 11
#
#   Error detected:                                snj: counter example
#   (<0>): Entering function main
#   (<0>,1): 
#            store 0x00000000 to [0x55c56f8da810]
#       ...
#            load 0x01000000 from [0x55c56f8dea60] source: (<0>,46)
#   Error: Assertion failure at tests/litmus/C-tests/iriwv2.c:81: 0

import os
import subprocess

test_results = [(2,'N'), (3,'N'), (3,'N'), (5,'N'), (7,'N'), (2,'N'), (3,'N'), (16,'N'), (5,'N'), (2,'N'), (2,'N'), (3,'N'), (3,'N'), (4,'N'), (9,'N'), (6,'N'), (6,'N'), (7,'N')]

failed_tests = []

current_path = os.getcwd() + '/'
test_path = current_path + 'tests/ViewEq_RegressionSuite/'

test_files = [f for f in os.listdir(test_path) if f.endswith('.c')]

# os.system('make')

tests_completed = 0

for test_file in test_files:
    test_index = int(test_file.split('_')[0][4:]) - 1
    test_name = test_file[5 + len(str(test_index)):]

    print ('Running Test' + str(test_index))

    (test_traces, test_isviolation) = test_results[test_index]

    os.system('clang -c -emit-llvm -S -o executable_file.ll ' + test_path + test_file)
    process = subprocess.Popen([current_path + 'src/nidhugg', '--sc', '--view' , current_path + 'executable_file.ll'], stdout=subprocess.PIPE, bufsize=1, universal_newlines=True)
    sout = process.stdout.readlines() # process.communicate()[0]
    os.system('rm ' + current_path + 'executable_file.ll')

    run_fail = True
    count_fail = False
    violation_status_fail = False

    # for line in sout:
    #     print line
    # print "-- stdout --"

    for line in sout:
        if line.startswith('Trace count:'):
            out_traces = int(line[len('Trace count: '):])
            if (out_traces != test_traces):
                count_fail =True

        elif line.startswith('No errors'):
            run_fail = False
            if (test_isviolation != 'N'):
                violation_status_fail = True

        elif line.startswith('Error detected'):
            run_fail = False
            if (test_isviolation != 'Y'):
                violation_status_fail = True

    if (run_fail):
        print str(test_index) + ' failed to complete run'

    if (count_fail and violation_status_fail):
        failed_tests = (test_file, 'Traces count mismatch (expected=' + test_traces + ' found=' + out_traces + ') & violation status mismatch (expected=' + test_isviolation + ')')
    elif (count_fail):
        failed_tests = (test_file, 'Traces count mismatch (expected=' + test_traces + ' found=' + out_traces + ')')
    elif (violation_status_fail):
        failed_tests = (test_file, 'Violation status mismatch (expected=' + test_isviolation + ')')

    tests_completed = tests_completed + 1

# assert(tests_completed == len(test_files))

print ('No. of tests run = ' + str(tests_completed))
print ('No. of tests failed = ' + str(len(failed_tests)))
for (idx, msg) in failed_tests:
    print ('Test ' + idx + ': FAIL')
    print ('     ' + msg)