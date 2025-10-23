import globals

import os, sys, subprocess

CONFIG = "Debug"
ret = 0

if globals.isWindows():
	VS_BUILD_PATH = r"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
	# print(VS_BUILD_PATH)

	ret = subprocess.call(["cmd.exe", "/c", VS_BUILD_PATH, "{}.sln".format(globals.ENGINE_NAME), "/p:Configuration={}".format(CONFIG)])

if globals.isLinux():
	ret = subprocess.call(["make", "config={}".format(CONFIG)])


if globals.isMac():
	ret = subprocess.call(["make", "config={}".format(CONFIG)])

# print(ret)
sys.exit(ret)