# A script for downloading the OTA partition from the device

pushd

cd ..\esp-idf

echo Importing IDF
echo
call export.bat

echo
echo DOWNLOADING (no progress indication)
echo

python components\app_update\otatool.py --port COM3 --baud 956000 read_ota_partition --name app0 --output app.bin

move /Y app.bin ..\MuscleMemory\Server\static\app.bin
cd ..\MuscleMemory
copy Firmware\src\Version.h Server\static

popd