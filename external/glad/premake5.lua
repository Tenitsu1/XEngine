project "glad"
	kind "StaticLib"
	language "C"
	staticruntime "on"

	targetdir("bin/%{cfg.buildcfg}/%{prj.name}")
	objdir("bin-obj/%{cfg.buildcfg}/%{prj.name}")

	files
	{
		"include/**.h",
		"src/**.c",
	}

	externalincludedirs
	{
		"include",
	}

	filter { "system:windows" }
		systemversion "latest"
		buildoptions { "/utf-8" }

	filter { "system:macosx" }
		xcodebuildsettings
		{
			["MACOSC_DEPLOYMENT_TARGET"] = "10.5",
			["UseModernBuildSystem"] = "NO"
		}


	filter { "system:linux", }


	filter {"configurations:Debug"}
		runtime "Debug"
		symbols "on"

	filter {"configurations:Release"}
		runtime "Release"
		symbols "off"
		optimize "on"
