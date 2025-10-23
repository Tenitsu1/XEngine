import sys, subprocess
import globals

ret = 0

if globals.isWindows():
	ret = subprocess.call(["cmd.exe", "/c", "premake\\premake5", "vs2022"])

if globals.isLinux():
	ret = subprocess.call(["premake/premake5.linux", "gmake2"])

if globals.isMac():
	ret = subprocess.call(["premake/premake5", "gmake2"])
	if ret == 0:
		subprocess.call(["premake/premake5", "xcode4"])

# print(ret)
sys.exit(ret)
