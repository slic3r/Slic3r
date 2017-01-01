# Written by Joseph Lenox
# Licensed under the same license as the rest of Slic3r.
# ------------------------
# You need to have Strawberry Perl 5.22 installed for this to work, 
echo "Make this is run from the perl command window." 
echo "Requires PAR."

New-Variable -Name "current_branch" -Value ""

git branch | foreach {
   if ($_ -match "`  (.*)"){
         $current_branch += $matches[1]
   }
}

# Change this to where you have Strawberry Perl installed.
New-Variable -Name "STRAWBERRY_PATH" -Value "C:\Strawberry"

cpanm "PAR::Packer"

pp `
-a "../utils;utils"  `
-a "autorun.bat;slic3r.bat"  `
-a "../var;var"  `
-a "${STRAWBERRY_PATH}\perl\bin\perl5.22.2.exe;perl5.22.2.exe"  `
-a "${STRAWBERRY_PATH}\perl\bin\perl522.dll;perl522.dll"  `
-a "${STRAWBERRY_PATH}\perl\bin\libgcc_s_sjlj-1.dll;libgcc_s_sjlj-1.dll"  `
-a "${STRAWBERRY_PATH}\perl\bin\libstdc++-6.dll;libstdc++-6.dll"  `
-a "${STRAWBERRY_PATH}\perl\bin\libwinpthread-1.dll;libwinpthread-1.dll"  `
-a "${STRAWBERRY_PATH}\perl\bin\freeglut.dll;freeglut.dll"  `
-a "${STRAWBERRY_PATH}\c\bin\libglut-0_.dll;libglut-0_.dll"  `
-a "../lib;lib" `
-a "../local-lib;local-lib" `
-a "../slic3r.pl;slic3r.pl" `
-M AutoLoader `
-M B `
-M Carp `
-M Config `
-M Crypt::CBC `
-M Class::Accessor `
-M Class::XSAccessor `
-M Class::XSAccessor::Heavy `
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
-M Sub::Defer `
-M Sub::Exporter `
-M Sub::Exporter::Progressive `
-M Sub::Name `
-M Sub::Util `
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
-M XSLoader `
-M attributes `
-M base `
-M bytes `
-M constant `
-M enum `
-M feature `
-M integer `
-M locale `
-M lib `
-M mro `
-M overload `
-M overload::numbers `
-M overloading `
-M parent `
-M re `
-M strict `
-M utf8 `
-M vars `
-M warnings `
-M warnings::register `
-M Win32::API `
-M Win32::TieRegistry `
-M Win32::WinError `
-M Win32API::Registry `
-e -p ..\slic3r.pl -o ..\slic3r.par

copy ..\slic3r.par "..\slic3r-${current_branch}-${APPVEYOR_BUILD_NUMBER}-$(git rev-parse --short HEAD).zip"
