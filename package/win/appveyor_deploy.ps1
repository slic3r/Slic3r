Add-AppveyorCompilationMessage -Message "Making ZIP package"
cd package/win
./compile_wrapper.ps1 524 | Write-Output
./package_win32.ps1 524| Write-Output
cd ../../
$myPath = $(pwd) -replace "\\", "/"
$myPath = $myPath -replace "C:", "/c" 
& 'C:\msys64\usr\bin\bash.exe' -c "PATH=/c/msys64/usr/bin:$PATH  BUILD_DIR=$env:APPVEYOR_BUILD_FOLDER    UPLOAD_USER=$env:UPLOAD_USER $mypath/package/deploy/sftp.sh win $BUILD_FOLDER/slic3r-upload.rsa.appveyor *.zip"

if (!(Test-Path C:\project\slic3r\slic3r.par)) {
  Add-AppveyorCompilationMessage -Message "Failed to package!" -Category Error
  return 1
}
