#!/usr/bin/env python3

import os
import shutil
import subprocess
import signal
import math

# Location of sandbox
sandbox_dir = "sandbox"

# Location of original files and the files to copy
original_dir = "."
files_to_copy = ["linked_list_test.c",
                 "main.c",
                 "Makefile",
                 "scheduler.c",
                 "scheduler.h",
                 "simulator.c",
                 "simulator.h",
                 "trace.c",
                 "trace.h"]

# Handin files
handin_files = ["linked_list.c",
                "linked_list.h",
                "schedulerFCFS.c",
                "schedulerLCFS.c",
                "schedulerSJF.c",
                "schedulerMLFQ.c"]

# Location of traces
traces_dir = "traces"

test_cases = {}

linked_list_test_type = 1
trace_test_type = 2

def add_test_case_linked_list(test_name):
    test_cases[test_name] = {"TestType": linked_list_test_type, "args": ["./linked_list_test", test_name]}

def add_test_case_linked_list_valgrind(test_name):
    test_cases[f"valgrind_{test_name}"] = {"TestType": linked_list_test_type, "args": ["valgrind", "-v", "--leak-check=full", "--errors-for-leak-kinds=all", "--error-exitcode=2", "./linked_list_test", test_name]}

def add_test_cases(test_name):
    add_test_case_linked_list(test_name)
    add_test_case_linked_list_valgrind(test_name)

add_test_cases("test_list_create")
add_test_cases("test_list_insert")
add_test_cases("test_list_find")
add_test_cases("test_list_remove")

def add_test_cases_trace(test_name, policy, input_file):
    output_file = f"{input_file}.out"
    expected_outfile = f"{input_file}.expected"
    test_cases[test_name] = {"TestType": trace_test_type, "args": ["./simulator", input_file, output_file, policy], "output_file": output_file, "expected_outfile": expected_outfile}

def add_test_cases_trace_valgrind(test_name, policy, input_file):
    output_file = f"{input_file}.out"
    expected_outfile = f"{input_file}.expected"
    test_cases[f"valgrind_{test_name}"] = {"TestType": trace_test_type, "args": ["valgrind", "-v", "--leak-check=full", "--errors-for-leak-kinds=all", "--error-exitcode=2", "./simulator", input_file, output_file, policy], "output_file": output_file, "expected_outfile": expected_outfile}

for f in sorted(os.listdir(os.path.join(original_dir, traces_dir))):
    filename = os.fsdecode(f)
    if filename.endswith(".csv"):
        policy = filename.split("_", 1)[0]
        add_test_cases_trace(filename, policy, os.path.join(traces_dir, filename))
        add_test_cases_trace_valgrind(filename, policy, os.path.join(traces_dir, filename))

# Score breakdown by points (points, list of tests required to get points)
part1_point_breakdown = [
    # Linked list tests
    (1, ["test_list_create"]),
    (2, ["test_list_find"]),
    (3, ["test_list_remove"]),
    (4, ["test_list_insert"]),
    (1, ["valgrind_test_list_create"]),
    (2, ["valgrind_test_list_find"]),
    (3, ["valgrind_test_list_remove"]),
    (4, ["valgrind_test_list_insert"])
]
part1 = ["FCFS", "LCFS"]
part2_point_breakdown = []
part2 = ["SJF", "MLFQ"]

for f in sorted(os.listdir(os.path.join(original_dir, traces_dir))):
    filename = os.fsdecode(f)
    if filename.endswith(".csv"):
        policy = filename.split("_", 1)[0]
        if policy in part1:
            part1_point_breakdown.append((1, [filename]))
            part1_point_breakdown.append((1, [f"valgrind_{filename}"]))
        elif policy in part2:
            part2_point_breakdown.append((1, [filename]))
            part2_point_breakdown.append((1, [f"valgrind_{filename}"]))
        else:
            print(f"Skipping trace {filename} since filename is not in appropriate format")

def print_success(test):
    print(f"****SUCCESS: {test}****")

def print_failed(test):
    print(f"****FAILED: {test}****")
 
def make_assignment(name):
    args = ["make", "clean", "all"]
    try:
        subprocess.check_output(args, stderr=subprocess.STDOUT)
        return True
    except subprocess.CalledProcessError as e:
        print_failed(name)
        print(e.output.decode())
    except KeyboardInterrupt:
        print_failed(name)
        print("User interrupted compilation")
    except:
        print_failed(name)
        print("Unknown error occurred")
    return False

def grade(submission_dir):
    result = {}

    # Clear previous sandbox
    if os.path.exists(sandbox_dir):
        subprocess.check_output(["rm", "-rf", sandbox_dir], stderr=subprocess.STDOUT)
    os.makedirs(sandbox_dir)

    # Copy traces directory
    os.makedirs(os.path.join(sandbox_dir, traces_dir))
    for f in os.listdir(os.path.join(original_dir, traces_dir)):
        shutil.copy2(os.path.join(original_dir, traces_dir, f), os.path.join(sandbox_dir, traces_dir, f))

    # Copy handin files
    for handin_file in handin_files:
        shutil.copy2(os.path.join(submission_dir, handin_file), os.path.join(sandbox_dir, handin_file))

    # Copy original files
    for f in files_to_copy:
        if os.path.exists(os.path.join(sandbox_dir, f)):
            os.remove(os.path.join(sandbox_dir, f))
        shutil.copy2(os.path.join(original_dir, f), os.path.join(sandbox_dir, f))

    # Copy job test
    if os.path.exists(os.path.join(sandbox_dir, "job.h")):
        os.remove(os.path.join(sandbox_dir, "job.h"))
    shutil.copy2(os.path.join(original_dir, "job_test.h"), os.path.join(sandbox_dir, "job.h"))

    # Switch to sandbox
    cwd = os.getcwd()
    os.chdir(sandbox_dir)

    # Check job test compiles
    if make_assignment("make: testing job fields are NOT directly used"):
        # Switch back to original directory
        os.chdir(cwd)

        # Copy job
        os.remove(os.path.join(sandbox_dir, "job.h"))
        shutil.copy2(os.path.join(original_dir, "job.h"), os.path.join(sandbox_dir, "job.h"))

        # Switch to sandbox
        os.chdir(sandbox_dir)

        # Check includes and run make on the assignment
        if make_assignment("make"):
            # Run test cases
            for test, config in test_cases.items():
                try: 
                    if config["TestType"] == linked_list_test_type:
                        output = subprocess.check_output(config["args"], stderr=subprocess.STDOUT).decode()
                        if "ALL TESTS PASSED" in output:
                            result[test] = True
                            print_success(test)
                        else:
                            result[test] = False
                            print_failed(test)
                            error_log = f"error_{test}.log"
                            with open(error_log, "w") as fp:
                                fp.write(output)
                            print(f"See {os.path.join(sandbox_dir, error_log)} for error details")
                    elif config["TestType"] == trace_test_type:
                        output = subprocess.check_output(config["args"], stderr=subprocess.STDOUT).decode()
                        rc, out = subprocess.getstatusoutput(f"diff -b -B {config['output_file']} {config['expected_outfile']}")
                        if rc == 0:
                            result[test] = True
                            print_success(test)
                        else:
                            result[test] = False
                            print_failed(test)
                            print("Output is different from expected output")
                except subprocess.CalledProcessError as e:
                    result[test] = False
                    print_failed(test)
                    if e.returncode < 0:
                        if -e.returncode == signal.SIGSEGV:
                            print("Segmentation fault (core dumped)")
                        else:
                            print(f"Died with signal {-e.returncode}")
                    error_log = f"error_{test}.log"
                    with open(error_log, "w") as fp:
                        fp.write(e.output.decode())
                    print(f"See {os.path.join(sandbox_dir, error_log)} for error details")
                except KeyboardInterrupt:
                    result[test] = False
                    print_failed(test)
                    print("User interrupted test")
                except Exception as e:
                    result[test] = False
                    print_failed(test)
                    print(str(e))
                except:
                    result[test] = False
                    print_failed(test)
                    print("Unknown error occurred")

    # Switch back to original directory
    os.chdir(cwd)

    # Calculate score
    part1_score = 0
    part1_total_possible = 0
    for points, tests in part1_point_breakdown:
        if all(map(lambda test : test in result and result[test], tests)):
            part1_score += points
        part1_total_possible += points
    part2_score = 0
    part2_total_possible = 0
    for points, tests in part2_point_breakdown:
        if all(map(lambda test : test in result and result[test], tests)):
            part2_score += points
        part2_total_possible += points
    return (part1_score, part1_total_possible, part2_score, part2_total_possible)

if __name__ == "__main__":
    submission_dir = "."
    part1_score, part1_total_possible, part2_score, part2_total_possible = grade(submission_dir)
    print(f"Part 1 score: {part1_score}/{part1_total_possible}")
    print(f"Part 2 score: {part2_score}/{part2_total_possible}")
