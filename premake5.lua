workspace "rplsinfo"
   configurations { "Debug", "Release" }

project "rplsinfo"
   kind "ConsoleApp"
   language "C++"
   targetdir "bin/%{cfg.buildcfg}"

   files { "**.h", "**.cpp" }

   filter "action:gmake"
      excludes { "stdafx.*", "targetver.h" }
      defines { "_FILE_OFFSET_BITS=64" }
      linkoptions { "-static" }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
