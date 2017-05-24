# Written by Joseph Lenox 
# Licensed under the same license as the rest of Slic3r.
# ------------------------
# You need to have Strawberry Perl 5.24.0.1 (or slic3r-perl) installed for this to work, 
param (
	[switch]$exe = $false
)
echo "Make this is run from the perl command window." 
echo "Requires PAR."

New-Variable -Name "current_branch" -Value ""
New-Variable -Name "current_date" -Value "$(Get-Date -UFormat '%Y.%m.%d')"
New-Variable -Name "output_file" -Value ""

if ($args[0]) {
	$perlversion = $args[0]
} else {
	$perlversion = "524"
}

$perldll = "perl$perlversion"

git branch | foreach {
   if ($env:APPVEYOR) {
	   if ($_ -match "`  (.*)") {
		   $current_branch += $matches[1]
	   }
   } else {
	   if ($_ -match "\*` (.*)"){
		   $current_branch += $matches[1]
	   }
   }
}
if ($exe) {
	$output_file = "slic3r.exe"
} else {
	$output_file = "slic3r.par"
}

# Change this to where you have Strawberry Perl installed.
New-Variable -Name "STRAWBERRY_PATH" -Value "C:\Strawberry"

cpanm "PAR::Packer"
if ($env:ARCH -eq "32bit") { 
	$perlarch = "sjlj"
	$glut = "libglut-0_.dll"
	$pthread= "pthreadGC2-w32.dll"
} else {
	$perlarch = "seh"
	$glut = "libglut-0__.dll"
	$pthread= "pthreadGC2-w64.dll"
}


pp `
-a "slic3r.exe;slic3r.exe"  `
-a "slic3r-console.exe;slic3r-console.exe"  `
-a "slic3r-debug-console.exe;slic3r-debug-console.exe"  `
-a "../../lib;libexec\lib" `
-a "../../local-lib;libexec\local-lib" `
-a "../../slic3r.pl;libexec\slic3r.pl" `
-a "../../utils;utils"  `
-a "../../var;libexec\var"  `
-a "../../FreeGLUT/freeglut.dll;libexec/freeglut.dll" `
-a "${STRAWBERRY_PATH}\perl\bin\perl${perlversion}.dll;libexec\perl${perlversion}.dll"  `
-a "${STRAWBERRY_PATH}\perl\bin\libstdc++-6.dll;libexec\libstdc++-6.dll"  `
-a "${STRAWBERRY_PATH}\perl\bin\libgcc_s_${perlarch}-1.dll;\libexec\libgcc_s_${perlarch}-1.dll"  `
-a "${STRAWBERRY_PATH}\perl\bin\libwinpthread-1.dll;libexec\libwinpthread-1.dll"  `
-a "${STRAWBERRY_PATH}\c\bin\${pthread};libexec\${pthread}"  `
-a "${STRAWBERRY_PATH}\c\bin\${glut};libexec\${glut}"  `
-M AutoLoader `
-M B `
-M Carp `
-M Class::Accessor `
-M Config `
-M Crypt::CBC `
-M Cwd `
-M Devel::GlobalDestruction `
-M Digest `
-M Digest::MD5 `
-M Digest::SHA `
-M Digest::base `
-M DynaLoader `
-M Errno `
-M Exporter `
-M Exporter::Heavy `
-M Fcntl `
-M File::Basename `
-M File::Glob `
-M File::Spec `
-M File::Spec::Unix `
-M File::Spec::Win32 `
-M FindBin `
-M HTTP::Config `
-M HTTP::Date `
-M HTTP::Headers `
-M HTTP::Headers::Util `
-M HTTP::Message `
-M HTTP::Request `
-M HTTP::Request::Common `
-M HTTP::Response `
-M HTTP::Status `
-M IO `
-M IO::Handle `
-M IO::Select `
-M IO::Socket `
-M LWP `
-M LWP::MediaTypes `
-M LWP::MemberMixin `
-M LWP::Protocol `
-M LWP::Protocol::http `
-M LWP::UserAgent `
-M List::Util `
-M Math::Trig `
-M Method::Generate::Accessor `
-M Method::Generate::BuildAll `
-M Method::Generate::Constructor `
-M Module::Runtime `
-M POSIX `
-M Pod::Escapes `
-M Pod::Text `
-M Pod::Usage `
-M SelectSaver `
-M Socket `
-M Socket6 `
-M Storable `
-M Sub::Exporter `
-M Sub::Exporter::Progressive `
-M Symbol `
-M Term::Cap `
-M Text::ParseWords `
-M Thread `
-M Thread::Queue `
-M Thread::Semaphore `
-M Tie::Handle `
-M Tie::Hash `
-M Tie::StdHandle `
-M Time::Local `
-M URI `
-M URI::Escape `
-M URI::http `
-M Unicode::Normalize `
-M Win32 `
-M Win32::API `
-M Win32::TieRegistry `
-M Win32::WinError `
-M Win32API::Registry `
-M XSLoader `
-B `
-M lib `
-p ..\..\slic3r.pl -o ..\..\${output_file}

# switch renaming based on whether or not using packaged exe or zip 
if ($exe) {
	if ($env:APPVEYOR) {
		copy ..\..\slic3r.exe "..\..\slic3r-${current_branch}.${current_date}.${env:APPVEYOR_BUILD_NUMBER}.$(git rev-parse --short HEAD).exe"
		del ..\slic3r.exe
	} else {
		copy ..\..\slic3r.exe "..\..\slic3r-${current_branch}.${current_date}.$(git rev-parse --short HEAD).exe"
		del ..\..\slic3r.exe
	}
} else {
# make this more useful for not being on the appveyor server
	if ($env:APPVEYOR) {
		copy ..\..\slic3r.par "..\..\slic3r-${current_branch}.${current_date}.${env:APPVEYOR_BUILD_NUMBER}.$(git rev-parse --short HEAD).$env:ARCH.zip"
	} else {
		copy ..\..\slic3r.par "..\..\slic3r-${current_branch}.${current_date}.$(git rev-parse --short HEAD).zip"
			del ..\..\slic3r.par
	}
}
