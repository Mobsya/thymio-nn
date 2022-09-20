# run a simulation with vmshell and check that output matches expectations

# Usage:
# python3 testsim.py program.aseba expected-output.txt
# Result:
# ok (exit status: 0)
# error (exit status: 1)

import sys
import os
import re

# get test input
program = sys.argv[1]
expected_output_file = sys.argv[2]
with open(expected_output_file) as f:
    expected_output = f.read()

# run simulation and get output
with os.popen(f"./vmshell --src {program}") as p:
    output = p.read()

# normalize output and expected_output
output = re.sub(r"\s+", " ", output).strip()
expected_output = re.sub(r"\s+", " ", expected_output).strip()

# compare
if output == expected_output:
	print("ok")
else:
	print("error")
	sys.exit(1)
