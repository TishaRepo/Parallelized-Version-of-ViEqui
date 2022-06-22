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
from output_colors import output_colors as oc

test_results = [(2,'N'), # Test1
                (3,'N'), # Test2
                (4,'N'), # Test3
                (5,'N'), # Test4
                (7,'N'), # Test5
                (2,'N'), # Test6
                (3,'N'), # Test7
                (16,'N'), # Test8
                (16,'N'), # Test9
                (5,'N'), # Test10
                (2,'N'), # Test11 
                (2,'N'), # Test12
                (3,'N'), # Test13
                (4,'N'), # Test14
                (9,'N'), # Test15
                (6,'N'), # Test16
                (6,'N'), # Test17
                (7,'N'), # Test18
                (4,'N'), # Test19
                (4,'N'), # Test20
                (7,'N'), # Test21
                (4,'N'), # Test22
                (2,'N'), # Test23
                (4,'N'), # Test24
                (6,'N'), # Test25
                (8,'N'), # Test26
                (4,'Y'), # Test27
                (4,'N'), # Test28
                (2,'N'), # Test29
                (1,'N'), # Test30
                (8,'N'), # Test31
                (4,'N')] # Test32

ignore_tests = [29]
failed_tests = []

current_path = os.getcwd() + '/'
test_path = current_path + 'tests/ViewEq_RegressionSuite/'

test_files = [f for f in os.listdir(test_path) if f.endswith('.c')]

# os.system('make')

tests_completed = 0

for test_index in range(1, len(test_results)+1):
    test_name = [test_file for test_file in test_files if ('Test' + str(test_index) + '_') in test_file][0]

    if test_index in ignore_tests:
        print (oc.YELLOW, 'Ignoring Test', test_index, oc.ENDC)
        continue

    print (oc.BLUE, 'Running Test', test_index, oc.ENDC)

    (test_traces, test_isviolation) = test_results[test_index-1]

    os.system('clang -c -emit-llvm -S -o executable_file.ll ' + test_path + test_name)
    process = subprocess.Popen([current_path + 'src/nidhugg', '--sc', '--view', '--check-optimality' , current_path + 'executable_file.ll'], stdout=subprocess.PIPE, bufsize=1, universal_newlines=True)
    sout = process.stdout.readlines() # process.communicate()[0]
    os.system('rm ' + current_path + 'executable_file.ll')

    run_fail = True
    count_fail = False
    violation_status_fail = False
    optimality_fail = False

    for line in sout:
        if 'Trace count:' in line:
            out_traces = int(line[line.find('Trace count') + len('Trace count: '):])
            if (out_traces != test_traces):
                count_fail =True

        elif line.startswith('No errors'):
            run_fail = False
            if (test_isviolation != 'N'):
                violation_status_fail = True

        elif line.startswith(' Error detected'):
            run_fail = False
            if (test_isviolation != 'Y'):
                violation_status_fail = True

        elif line.startswith('Redundant explorations'):
            optimality_fail = True

    if (run_fail):
        print (oc.RED, test_index, 'Failed to complete run', oc.ENDC)
        failed_tests.append((test_name, 'Failed to complete run'))

    fail_msg = ''
    if count_fail:
        fail_msg += '     Traces count mismatch (expected=' + str(test_traces) + ' found=' + str(out_traces) + ')\n'
    if violation_status_fail:
        fail_msg += '     Violation status mismatch (expected=' + test_isviolation + ')\n'
    if optimality_fail:
        fail_msg += '     Optimality failed\n'

    if count_fail or violation_status_fail or optimality_fail:
        failed_tests.append( (test_name, fail_msg) )
    else:
        print(oc.GREEN, 'OK', oc.ENDC)

    tests_completed += 1

assert(tests_completed == len(test_results) - len(ignore_tests))

print(oc.YELLOW)
print('----------------------------------------------')
print ('No. of tests run = ' + str(tests_completed))
print ('No. of tests failed = ' + str(len(failed_tests)))
print('----------------------------------------------')
print(oc.ENDC)
for (idx, msg) in failed_tests:
    print ('Test ' + idx + ': FAIL')
    print (msg)