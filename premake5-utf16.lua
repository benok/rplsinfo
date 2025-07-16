workspace "rplsinfo"
   configurations { "Debug", "Release" }
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

   filter "action:vs*"
      defines {
         "USE_UTF16",
         "_CRT_SECURE_NO_WARNINGS",
      }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
