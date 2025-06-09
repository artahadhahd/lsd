if not exist ".\build\" mkdir ".\build\"
if not exist ".\bin\" mkdir ".\bin\"
cd /d build
cmake ..
cmake --build .
move .\lsd.exe ..\bin\
echo "The executable is now in the bin directory of the project"
pause