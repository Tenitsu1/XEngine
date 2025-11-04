workspace "XEngine"
	startproject "XEngineditor"
	architecture "x64"

	configurations
	{
		"Debug",
		"Release"
	}

-- External Dependencies

externals = {}
externals["sdl3"] = "external/SDL3"
externals["spdlog"] = "external/spdlog"
externals["glad"] = "external/glad"
-- externals["tinygltf"] = "external/tinygltf"
-- externals["imgui"] = "external/imgui"

-- Run premake5.lua in Glad first
include "external/glad"


project "XEngine"
	location "XEngine"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir("bin/%{cfg.buildcfg}/%{prj.name}")
	objdir("bin-obj/%{cfg.buildcfg}/%{prj.name}")

	files
	{
		-- "external/imgui/**.h", "external/imgui/**.cpp",
		"%{prj.name}/include/**.h",
		"%{prj.name}/include/**.hpp",
		"%{prj.name}/include/**.cpp",
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/include/**.natvis",
	}

	externalincludedirs
	{
		"%{prj.name}/include/",
		"%{prj.name}/include/XEngine",
		"%{externals.sdl3}/include",
		"%{externals.spdlog}/include",
		"%{externals.glad}/include",
		-- "%{externals.tinygltf}/include"
		-- "%{externals.imgui}"
	}

	fatalwarnings { "ALL" }

	defines
	{
		"GLFW_INCLUDE_NONE"  -- Ensure glad doesn't include glfw
	}

	filter { "system:windows", "configurations:*" }
		systemversion "latest"
		buildoptions { "/utf-8" }
		defines
		{
			"XENGINE_PLATFORM_WINDOWS"
		}

	filter { "system:macosx", "configurations:*" }
		xcodebuildsettings
		{
			["MACOSC_DEPLOYMENT_TARGET"] = "10.5",
			["UseModernBuildSystem"] = "NO"
		}

		defines
		{
			"XENGINE_PLATFORM_MAC"
		}

	filter { "system:linux", "configurations:*" }
		defines
		{
			"XENGINE_PLATFORM_LINUX"
		}

	filter {"configurations:Debug"}
		defines
		{
			"XENGINE_CONFIG_DEBUG"
		}
		runtime "Debug"
		symbols "on"

	filter {"configurations:Release"}
		defines
		{
			"XENGINE_CONFIG_RELEASE"
		}
		runtime "Release"
		symbols "off"
		optimize "on"

project "XEngineditor"
	location "XEngineditor"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"
	links { "XEngine" }

	targetdir("bin/%{cfg.buildcfg}/%{prj.name}")
	objdir("bin-obj/%{cfg.buildcfg}/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/shaders/**",
		"%{prj.name}/models/**.bin",
		"%{prj.name}/models/**.glb",
		"%{prj.name}/models/**.gltf",
		"%{prj.name}/image/**"
	}

	externalincludedirs
	{
		"XEngine/include",
		"%{externals.spdlog}/include",
		-- "%{externals.imgui}",
		-- "%{externals.sdl3}/include"
	}

	fatalwarnings { "ALL" }

	filter { "system:windows", "configurations:*" }
		systemversion "latest"	
		buildoptions { "/utf-8" }
		defines
		{
			"XENGINE_PLATFORM_WINDOWS"
		}

		libdirs
		{
			"%{externals.sdl3}/lib"
		}

		links{ "SDL3", "glad" }

	filter { "system:macosx", "configurations:*" }
		xcodebuildsettings
		{
			["MACOSC_DEPLOYMENT_TARGET"] = "10.5",
			["UseModernBuildSystem"] = "NO"
		}

		defines
		{
			"XENGINE_PLATFORM_MAC"
		}

	filter { "system:linux", "configurations:*" }
		defines
		{
			"XENGINE_PLATFORM_LINUX"
		}

	filter {"configurations:Debug"
		}defines
		{
			"XENGINE_CONFIG_DEBUG"
		}
		runtime "Debug"
		symbols "on"

	filter {"configurations:Release"}
		defines
		{
			"XENGINE_CONFIG_RELEASE"
		}
		runtime "Release"
		symbols "off"
		optimize "on"

