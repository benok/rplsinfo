rem nkf con be installed with chocolatey
rem (https://github.com/kai2nenobu/my-chocolatey-packages)
rem
rem choco install nkf --source https://www.myget.org/F/kai2nenobu

set OPT=-w16L --overwrite
nkf %OPT% characterSets.cpp
nkf %OPT% characterSets.h
nkf %OPT% convToUnicode.cpp
nkf %OPT% convToUnicode.h
nkf %OPT% rplsinfo.cpp
nkf %OPT% rplsinfo.h
nkf %OPT% rplsproginfo.cpp
nkf %OPT% rplsproginfo.h
nkf %OPT% stdafx.cpp
nkf %OPT% stdafx.h
nkf %OPT% targetver.h
nkf %OPT% tsprocess.cpp
nkf %OPT% tsprocess.h
nkf %OPT% tsproginfo.cpp
nkf %OPT% tsproginfo.h

set ACTION=vs2022
premake5 %ACTION% --file=premake5-utf16.lua

start rplsinfo.sln
