mkdir C:\projects\slic3r\FreeGLUT
if (!(Test-Path "C:\users\appveyor\freeglut.$env:ARCH.7z")) 
{
    wget "https://bintray.com/lordofhyphens/Slic3r/download_file?file_path=freeglut-mingw-3.0.0.$env:ARCH.7z" -o C:\users\appveyor\freeglut.$env:ARCH.7z
}
cmd /c "7z x C:\Users\appveyor\freeglut.$env:ARCH.7z -oC:\projects\slic3r\FreeGLUT"

if (!(Test-Path "C:\users\appveyor\strawberry.$env:ARCH.msi"))  {
	if ($env:ARCH -eq "64bit") {
		wget "https://bintray.com/lordofhyphens/Slic3r/download_file?file_path=slic3r-perl-5.24.1.4-64bit.msi" -o "C:\users\appveyor\strawberry.$env:ARCH.msi" | Write-Output
	} else {
		wget "http://strawberryperl.com/download/5.24.1.1/strawberry-perl-5.24.1.1-32bit.msi" -o "C:\users\appveyor\strawberry.$env:ARCH.msi" | Write-Output
	}
}

if (!($env:ARCH -eq "32bit")) {
	if (!(Test-Path "C:\users\appveyor\extra_perl.7z"))  {
		wget "https://bintray.com/lordofhyphens/Slic3r/download_file?file_path=Strawberry-6.3.0-seg-archive.7z" -o "C:\users\appveyor\extra_perl.7z" | Write-Output
	}
}

msiexec.exe /i "C:\users\appveyor\strawberry.$env:ARCH.msi" /quiet
if (!($env:ARCH -eq "32bit")) {
	cmd /c "7z x -aoa C:\Users\appveyor\extra_perl.7z -oC:\"
}

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
        if (!(Test-Path "C:\users\appveyor\boost.1.63.0.$env:ARCH.7z") -Or $env:FORCE_BOOST_REINSTALL -eq 1) {
			if ($env:ARCH -eq "64bit") {
				wget "http://www.siusgs.com/slic3r/buildserver/win/boost_1_63_0-x64-gcc-6.3.0-seh.7z" -O "C:\users\appveyor\boost.1.63.0.$env:ARCH.7z" | Write-Output
			} else {
				wget "https://bintray.com/lordofhyphens/Slic3r/download_file?file_path=boost_1_63_0-x32-gcc-4.9.2-sjlj.7z" -O "C:\users\appveyor\boost.1.63.0.$env:ARCH.7z" | Write-Output
			}
        }
    Add-AppveyorCompilationMessage -Message "Extracting cached archive."
        cmd /c "7z x C:\Users\appveyor\boost.1.63.0.$env:ARCH.7z -oC:\dev"

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
    rm "C:\Users\appveyor\wxwidgets-$env:ARCH.7z" -Force
}

if (!(Test-Path "C:\Users\appveyor\wxwidgets-$env:ARCH.7z")) {
	Add-AppveyorCompilationMessage -Message "Extracting wxWidgets for $env:ARCH"
	wget "https://bintray.com/lordofhyphens/Slic3r/download_file?file_path=wxwidgets-$env:ARCH.7z" -o C:\users\appveyor\wxwidgets-$env:ARCH.7z
	7z x C:\users\appveyor\wxwidgets-$env:ARCH.7z -oC:\dev
} else {
    Add-AppveyorCompilationMessage -Message "Extracting prebuilt wxWidgets."
        7z x "C:\Users\appveyor\wxwidgets-$env:ARCH.7z" -oC:\dev
}
