#cil build
#cil run
#cli gen
#cli version
#cli gen build run

import os, sys
import subprocess

TOOLS_DIR = "tools"

def runCommand(cmd):
	ret = 0
	script = "{}/{}/{}.py".format(os.getcwd(), TOOLS_DIR, cmd)

	if os.path.exists(script):
		print("Executing: ", cmd)
		ret = subprocess.call(["python3", script])
	else:
		print("Invalid commad: ", cmd)
		ret = -1

	return ret

for i in range(1,len(sys.argv)):
	cmd = sys.argv[i]
	# print(cmd)
	
	print("\n-------------------------")
	if runCommand(cmd) != 0:
		break

	# runCommand(cmd)