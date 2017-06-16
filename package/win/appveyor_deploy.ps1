
Add-AppveyorCompilationMessage -Message "Uploading to server."
& ./package/deploy/winscp.ps1 -DIR win -KEY $env:APPVEYOR_BUILD_FOLDER/package/deploy/slic3r-upload.ppk -FILE *.zip *>> ./sftplog.txt
