Add-AppveyorCompilationMessage -Message "Making ZIP package"
cd package/win
./compile_wrapper.ps1 524 | Write-Output
./package_win32.ps1 524| Write-Output
cd ../../
if (!(Test-Path C:\project\slic3r\slic3r.par)) {
  Add-AppveyorCompilationMessage -Message "Failed to package!" -Category Error
  return 1
}

Add-AppveyorCompilationMessage -Message "Uploading to server."
& ./package/deploy/winscp.ps1 -DIR win -KEY $env:APPVEYOR_BUILD_FOLDER/package/deploy/slic3r-upload.ppk -FILE *.zip *>> ./sftplog.txt
