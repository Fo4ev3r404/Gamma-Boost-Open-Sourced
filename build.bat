@echo off
taskkill /F /IM GammaBoost.exe >nul 2>&1
windres resource.rc -O coff -o resource.res 2>nul || "C:\Users\mbari\scoop\apps\mingw\current\bin\windres.exe" resource.rc -O coff -o resource.res
g++ -O2 -mwindows -municode main.cpp resource.res -o GammaBoost.exe -lgdi32 -lgdiplus -lole32 -ldwmapi -luser32 -lcomctl32 -lcomdlg32 -lshell32 -lwinmm -lpsapi -ladvapi32 -static -static-libgcc -static-libstdc++ 2>nul || "C:\Users\mbari\scoop\apps\mingw\current\bin\g++.exe" -O2 -mwindows -municode main.cpp resource.res -o GammaBoost.exe -lgdi32 -lgdiplus -lole32 -ldwmapi -luser32 -lcomctl32 -lcomdlg32 -lshell32 -lwinmm -lpsapi -ladvapi32 -static -static-libgcc -static-libstdc++
if %ERRORLEVEL% EQU 0 (
    echo Build succeeded!
) else (
    echo Build failed!
)
