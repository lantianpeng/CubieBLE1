@echo off

set scripts_path=..\..\scripts\support\actions
set maker_path=%scripts_path%\utils\fw_maker
set att_maker_path=%scripts_path%\utils\att_maker

echo %TIME:~0,2%:%TIME:~3,2%:%TIME:~6,2%

mkdir .\outdir\bin

echo *************************Build NVRAM*****************************

python -B %scripts_path%\build_nvram_bin.py -o .\outdir\bin\nvram.bin nvram.prop

echo *************************Build BIN*****************************
copy .\firmware.xml .\outdir\
copy .\outdir\*.bin .\outdir\bin\
cd outdir
python -B ..\%scripts_path%\build_firmware.py -c .\firmware.xml -b zs110a
del parttbl.bin firmware.xml
cd ..\

echo *************************Build fw*****************************
%maker_path%\Maker.exe -c .\fw_maker.cfg -o .\outdir\zs110a.fw -mf

echo *************************Build Atf*****************************
copy %att_maker_path%\acttest.ap .\outdir\
copy %att_maker_path%\config.xml .\outdir\
copy %att_maker_path%\config.txt .\outdir\

cd outdir
del zs110a_atf.fw
..\%att_maker_path%\att_maker.exe zs110a_atf.fw 4 acttest.ap config.xml zs110a.fw config.txt

del acttest.ap config.* zs110a.fw

cd ..\

echo.
echo.

pause
