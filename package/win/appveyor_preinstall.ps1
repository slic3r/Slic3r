mkdir C:\projects\slic3r\FreeGLUT
if (!(Test-Path "C:\users\appveyor\freeglut.7z")) 
{
    wget "http://www.siusgs.com/slic3r/buildserver/win/freeglut-mingw-3.0.0.win64.7z" -o C:\users\appveyor\freeglut.7z
}
cmd /c "7z x C:\Users\appveyor\freeglut.7z -oC:\projects\slic3r\FreeGLUT"

if (!(Test-Path "C:\users\appveyor\strawberry.msi"))  {
    wget "https://bintray.com/lordofhyphens/Slic3r/download_file?file_path=slic3r-perl-5.24.1.4-64bit.msi" -o "C:\users\appveyor\strawberry.msi" | Write-Output
}
if (!(Test-Path "C:\users\appveyor\extra_perl.7z"))  {
    wget "https://bintray.com/lordofhyphens/Slic3r/download_file?file_path=Strawberry-6.3.0-seg-archive.7z" -o "C:\users\appveyor\extra_perl.7z" | Write-Output
}

msiexec.exe /i "C:\users\appveyor\strawberry.msi" /quiet
cmd /c "7z x -aoa C:\Users\appveyor\extra_perl.7z -oC:\"

if (!(Test-Path "C:\users\appveyor\winscp.zip")) {
    wget "https://bintray.com/lordofhyphens/Slic3r/download_file?file_path=WinSCP-5.9.4-Portable.zip" -o "C:\users\appveyor\winscp.zip" | Write-Output
}
cmd /c "7z x C:\Users\appveyor\winscp.zip -oC:\Strawberry\c\bin"

rm -r C:\min* -Force
rm -r C:\msys64\mingw* -Force
rm -r C:\cygwin* -Force
rm -r C:\Perl -Force

$PERLDIR = 'C:\Strawberry'
$env:Path = "C:\Strawberry\c\bin;C:\Strawberry\perl\bin;C:\Strawberry\perl\vendor\bin;" + $env:Path

if(Test-Path -Path 'C:\Strawberry' ) {
    copy C:\Strawberry\c\bin\gcc.exe C:\Strawberry\c\bin\cc.exe
        cmd /c mklink /D C:\Perl C:\Strawberry\perl
        mkdir C:\dev
        if (!(Test-Path "C:\users\appveyor\boost.1.63.0.7z") -Or $env:FORCE_BOOST_REINSTALL -eq 1) {
            wget "https://bintray.com/lordofhyphens/Slic3r/download_file?file_path=boost_1_63_0-x64-gcc-6.3.0-seh.7z" -O "C:\users\appveyor\boost.1.63.0.7z" | Write-Output
        }
    Add-AppveyorCompilationMessage -Message "Extracting cached archive."
        cmd /c "7z x C:\Users\appveyor\boost.1.63.0.7z -oC:\dev"

        mkdir C:\dev\CitrusPerl
        cmd /C mklink /D C:\dev\CitrusPerl\mingw32 C:\Strawberry\c
        cd C:\projects\slic3r
        cpanm ExtUtils::Typemaps::Basic
        cpanm ExtUtils::Typemaps::Default
        cpanm local::lib
        Add-AppveyorCompilationMessage -Message "Finished install script."
        rm -r 'C:\Program Files\Git\usr\bin' -Force
} else {
    Add-AppveyorCompilationMessage -Message "No strawberry perl!"
}


Add-AppveyorCompilationMessage -Message "Installing wxWidgets (xsgui dependency))"
if ($env:FORCE_WX_BUILD -eq 1) {
    rm "C:\Users\appveyor\wxwidgets.7z" -Force
}
if (!(Test-Path "C:\Users\appveyor\wxwidgets.7z")) {
    Add-AppveyorCompilationMessage -Message "Compiling wxWidgets"
        git clone https://github.com/wxWidgets/wxWidgets -b "v3.1.0" -q C:\dev\wxWidgets
        cd C:\dev\wxwidgets
        cp .\include\wx\msw\setup0.h include/wx/msw/setup.h
        cd build\msw
        mingw32-make -f makefile.gcc CXXFLAGS="-std=gnu++11" BUILD=release VENDOR=Slic3r 
        cd C:\dev
        7z a C:\Users\appveyor\wxwidgets.7z wxwidgets
        cd C:\projects\slic3r
} else {
    Add-AppveyorCompilationMessage -Message "Extracting prebuilt wxWidgets."
        7z x "C:\Users\appveyor\wxwidgets.7z" -oC:\dev
}
