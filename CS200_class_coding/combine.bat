@echo off
setlocal

set "OUT_FILE=CS200_Code_For_Gemini.txt"
set "SEARCH_DIR=source"

echo Combining all .h and .cpp files inside '%SEARCH_DIR%' folder...
if exist "%OUT_FILE%" del "%OUT_FILE%"

rem The change is here: "for /r" now starts from the SEARCH_DIR
for /r "%SEARCH_DIR%" %%f in (*.h *.hpp *.cpp) do (
    echo // ===== FILENAME: %%~pnxf ===== >> "%OUT_FILE%"
    type "%%f" >> "%OUT_FILE%"
    echo. >> "%OUT_FILE%"
)

echo.
echo Done! Output file is %OUT_FILE%
endlocal
pause