workspace "rplsinfo"
   configurations { "Debug", "Release" }

   filter "system:windows"
      platforms { "Win32", "Win64" }

   filter { "platforms:Win32" }
      system "Windows"
      architecture "x86"

   filter { "platforms:Win64" }
      system "Windows"
      architecture "x86_64"

project "rplsinfo"
   kind "ConsoleApp"
   language "C++"
   targetdir "bin/%{cfg.platform}/%{cfg.buildcfg}"
   objdir ".obj/%{cfg.platform}/%{cfg.buildcfg}"

   files { "*.h", "*.cpp" }
   files { "utfcpp/source/**.h" }

   filter "action:gmake"
      excludes { "stdafx.*", "targetver.h" }
      defines { "_FILE_OFFSET_BITS=64" }
      --defines { "USE_ICONV" }
      defines { "USE_UTF8_CPP" }
      buildoptions {
        "-Wall",
        "-Wno-sign-compare",
      }
      linkoptions { "-static" }

   filter "action:vs*"
      buildoptions { "/utf-8" }
      characterset("MBCS") -- CP65001(UTF-8)
      defines {
         "USE_UTF8_CPP",
         "_CRT_SECURE_NO_WARNINGS",
      }
      files { "rplsinfo.exe.manifest" }
      links { "Shlwapi.lib" }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
