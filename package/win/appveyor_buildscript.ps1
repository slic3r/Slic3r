if (!(Test-Path "C:\users\appveyor\local-lib.7z")) {
    wget "http://www.siusgs.com/slic3r/buildserver/win/slic3r-perl-dependencies-5.24.0-win-seh-gcc6.3.0-x64.7z" -o "C:\users\appveyor\local-lib.7z" | Write-Output
}

if (Test-Path "C:\users\appveyor\local-lib.7z") {
    cmd /c "7z x C:\Users\appveyor\local-lib.7z -oC:\projects\slic3r" -y | Write-Output
    rm -r 'C:\projects\slic3r\local-lib\Slic3r*'
}

$env:Path = "C:\Strawberry\c\bin;C:\Strawberry\perl\bin;" +     $env:Path
cd C:\projects\slic3r
rm -r 'C:\Program Files (x86)\Microsoft Vis*\bin' -Force
Add-AppveyorCompilationMessage -Message "Building Slic3r XS"
perl Build.pl
if (-NOT ($LASTEXITCODE -eq 0)) {
    Add-AppveyorCompilationMessage -Message "XS Failed to Build" -Category Error
}
Add-AppveyorCompilationMessage -Message "Making ZIP package"
cd package/win
./compile_wrapper.ps1 524| Write-Output
./package_win32.ps1 524| Write-Output
