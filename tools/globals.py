V_MAJOR = 0
V_MINOR = 1

ENGINE_NAME = "XEngine"
PROJECT_NAME = "XEngineditor"

TOOLS_DIR = "tools"

import sys, platform

PLATFORM = sys.platform

for x in platform.uname():
	if "microsoft" in x.lower():
		PLATFORM = "windows"
		break

def isWindows():
	return PLATFORM == "windows"

def isLinux():
	return PLATFORM == "Linux"

def isMac():
	return PLATFORM == "darwin"