echo (copy configs)
if exist "%TOP_GEAR_SERVER_PATH%\Binaries\RunEnv\config\*.xml" xcopy /D /F /I /R /Y "%TOP_GEAR_SERVER_PATH%\Binaries\RunEnv\config\*.xml" "%TOP_GEAR_PATH%\Binaries\RunEnv\Configs"
echo (generate functions)
%TOP_GEAR_SERVER_PATH%\Tools\FuncTool -i%1 -c%TOP_GEAR_PATH%\Binaries\RunEnv\Lua\script -s%TOP_GEAR_SERVER_PATH%\Binaries\RunEnv\script
