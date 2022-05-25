import os
import subprocess
import csv

bench_dir   = 'temp'
work_dir    = bench_dir + '_temp'
bench_path  = 'benchmarks/sv-repo-benchmarks/'
work_path   = bench_path + work_dir + '/'
bench_files = [f for f in os.listdir(work_path) if f.endswith('.c')] # no .cc files in sv-benchmarks

remove_keys  = ['extern', '__VERIFIER_atomic_begin', '__VERIFIER_atomic_end', 'void __VERIFIER_assert']
replace_keys = {'__VERIFIER_assert':'assert', 'true':'1', 'false':'0'}

os.system('rm -r ' + work_path)
os.system('mkdir ' + work_path)
os.system('cp ' + bench_path + bench_dir + '/* ' + work_path)

def init_in_main(lines, dec_start, dec_end, dest_start):
    inits = []
    for i in range(dec_start, dec_end+1):
        line = lines[i].lstrip() # remove leading spaces
        if len(line) == 0:
            continue
        vars = line[line.find(' ')+1:]
        vars = vars[:vars.rfind(';')]
        vars = vars.split(',')
        for var in vars:
            if '=' in var: # var=val
                inits.append(var + ';\n')
            else:
                inits.append(var + '=0;\n')

    return lines[:dest_start] + inits + ['\n'] + lines[dest_start:]


failed_tests = []
tests_completed = 0

for file in bench_files:
    print ('-------------------- Formatting ' + file[:-2] + ' --------------------')

    original_file = open(work_path + file, 'r')
    original_program = original_file.readlines()
    original_file.close()

    original_program = init_in_main(original_program, 47, 140, 253)
    transformed_program = ''

    push_down_lines = ''
    count_push_down_lines = 0
    last_include = 0
    line_no = 0

    uninitialized_global_vars = []
    in_global_var_space = True
    awaiting_main_start = False

    for line in original_program:
        line_no += 1
        remove_line = False
        for key in remove_keys:
            if key in line:
                remove_line = True
                # print('removing ' + line)
                break
        if remove_line:
            continue

        for key in replace_keys:
            if key in line:
                key_start = line.find(key)
                key_end   = key_start + len(key)
                line = line[:key_start] + replace_keys[key] + line[key_end:]      

        if '#include' in line:
            last_include = line_no     

        if last_include == 0:
            push_down_lines += line
            count_push_down_lines += 1
        else:
            transformed_program += line

    k = last_include - count_push_down_lines - 1
    count_lines = 0
    final_program = ''
    l = 0
    for c in transformed_program:
        l += 1
        final_program += c
        if c == '\n':
            count_lines += 1
            if count_lines >= k:
                break
    final_program += push_down_lines + transformed_program[l:]


    # print(transformed_program)
    transformed_file = open(work_path + file, 'w')
    transformed_file.write(final_program)
    transformed_file.close()

    # os.system('gcc -pthread ' + work_path + file)

    # process = subprocess.Popen(['gcc', '-pthread', work_path + file], 
    #             stdout=subprocess.PIPE, 
    #             stderr=subprocess.STDOUT, 
    #             universal_newlines=True)

    # sout = process.stdout.readlines()
    # if len(sout) > 0:
    #     print(str(sout))

    # break