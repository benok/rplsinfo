workspace "rplsinfo"
   configurations { "Debug", "Release" }

project "rplsinfo"
   kind "ConsoleApp"
   language "C++"
   targetdir "bin/%{cfg.buildcfg}"

   files { "*.h", "*.cpp" }
   files { "utfcpp/source/**.h" }

   filter "action:gmake"
      excludes { "stdafx.*", "targetver.h" }
      defines { "_FILE_OFFSET_BITS=64" }
      --defines { "USE_ICONV" }
      defines { "USE_UTF8_CPP" }
      linkoptions { "-static" }

   filter "action:vs*"
      buildoptions { "/utf-8" }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
