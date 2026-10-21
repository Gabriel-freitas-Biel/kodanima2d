compilation resource commands  kodenima_geany_novo.cpp

compile    C:\mingw64\bin\g++.exe -std=c++11 -c "%f" -o "%e.o"

bulid      cmd.exe /C "C:\mingw64\bin\windres.exe kodaicone.rc -O coff -o kodaicone_res.o && C:\mingw64\bin\g++.exe -std=c++17 -mwindows "%f" kodaicone_res.o -o "%e.exe" -lcomctl32 -lcomdlg32 -lshell32 -lgdi32 -luser32 -lmfplat -lmfreadwrite -lmfuuid -lole32 -loleaut32 -lmsimg32 -ld3d9 -luuid"

lint       "%e.exe"

These are the commands needed to compile the program.

You need to place the program inside a folder at this path.

C:\Users\pc user\Downloads\projeto

With everything ready, you can now compile.
