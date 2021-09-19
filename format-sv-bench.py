import os
import subprocess
import csv

bench_dir   = 'pthread'
work_dir    = bench_dir + '_temp'
bench_path  = 'benchmarks/sv-benchmarks/c/'
work_path   = bench_path + work_dir + '/'
bench_files = [f for f in os.listdir(work_path) if f.endswith('.c')] # no .cc files in sv-benchmarks
# bench_files = ['stack_longer-1.c']

os.system('rm -r ' + work_path)
os.system('mkdir ' + work_path)
os.system('cp ' + bench_path + bench_dir + '/* ' + work_path)

failed_tests = []
tests_completed = 0

for file in bench_files:
    print ('-------------------- Formatting ' + file[:-2] + ' --------------------')

    original_file = open(work_path + file, 'r')
    original_program = original_file.readlines()
    original_file.close()

    transformed_program = ''

    uninitialized_global_vars = []
    in_global_var_space = True
    awaiting_main_start = False

    for line in original_program:
        # print (line)
        if 'extern void' in line:
            # print('removing extern void')
            continue

        if '__VERIFIER' in line:
            # print('removing __VERIFIER')
            continue

        if 'void reach_error()' in line:
            # print('removing reach_error')
            continue

        if 'ERROR:' in line:
            # print('chengin ERROR:... to assert(0)')
            transformed_program = transformed_program + line[:line.find('ERROR:')] + 'assert(0);\n' # indentation then assert(0)
            continue

        if 'goto ERROR' in line:
            transformed_program = transformed_program + 'assert(0);\n'
            continue

        if in_global_var_space and line.startswith('int') and not('(' in line): # remove global var initialization (to be shifted to main)
            line = line[4:-2] # skip int and ;
            line = line.replace(" ", "") # remove white spaces
            declarations = line.split(',') # separate different declarations

            transformed_program = transformed_program + 'int '

            for decl in declarations:
                if '=' in decl:
                    decl = decl.split('=')
                    uninitialized_global_vars.append((decl[0], decl[1])) # var, val pairs
                    transformed_program = transformed_program + decl[0] + ', '
                else:
                    transformed_program = transformed_program + decl + ', '

            transformed_program = transformed_program[:-2] + ';\n'
            # print('global vars:' + decl)
            continue

        if line.startswith('{') and awaiting_main_start:
            awaiting_main_start = False
            transformed_program = transformed_program + line
            for (var, val) in uninitialized_global_vars:
                transformed_program = transformed_program + '  ' + var + ' = ' + val + ';\n'
            uninitialized_global_vars = []
            # print('reached main')
            continue

        if line.startswith('int main'): # put initialization of global vars in main
            if '{' in line:
                transformed_program = transformed_program + line
                for (var, val) in uninitialized_global_vars:
                    transformed_program = transformed_program + '  ' + var + ' = ' + val + ';\n'
                uninitialized_global_vars = []
                # print('reached main')
                continue
            else:
                awaiting_main_start = True

        if line.startswith('void *t1') or line.startswith('{') or line.startswith('#define'):
            # print('void* or { or #define')
            in_global_var_space = False

        # if 'pthread_create' in line: TODO
            

        # print('added line')
        transformed_program =transformed_program + line

    # print(transformed_program)
    transformed_file = open(work_path + file, 'w')
    transformed_file.write(transformed_program)
    transformed_file.close()

    os.system('gcc -pthread ' + work_path + file)

    # process = subprocess.Popen(['gcc', '-pthread', work_path + file], 
    #             stdout=subprocess.PIPE, 
    #             stderr=subprocess.STDOUT, 
    #             universal_newlines=True)

    # sout = process.stdout.readlines()
    # if len(sout) > 0:
    #     print(str(sout))

    # break