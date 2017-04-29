Add-AppveyorCompilationMessage -Message "Making ZIP package"
cd package/win
./compile_wrapper.ps1 524 | Write-Output
./package_win32.ps1 524| Write-Output
cd ../../

Add-AppveyorCompilationMessage -Message "Uploading to server."
& ./package/deploy/winscp.ps1 -DIR win -KEY $env:APPVEYOR_BUILD_FOLDER/package/deploy/slic3r-upload.ppk -FILE *.zip *>> ./sftplog.txt
