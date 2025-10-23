import globals
import os, sys, subprocess

config = "debug"
exePath = "{}/bin/{}/{}/".format(os.getcwd(), config, globals.PROJECT_NAME)
ret = 0

if globals.isWindows():
	ret = subprocess.call(["cmd.exe", "/c", "{}\\run.bat".format(globals.TOOLS_DIR), config, globals.PROJECT_NAME], cwd=os.getcwd())
else:
	ret = subprocess.call(["{}{}".format(exePath, globals.PROJECT_NAME)], cwd=exePath)

# print(ret)
sys.exit(ret)