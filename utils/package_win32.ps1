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
-a "${STRAWBERRY_PATH}\perl\site\lib\Alien\wxWidgets\msw_3_0_2_uni_gcc_3_4\lib\wxbase30u_gcc_custom.dll;wxbase30u_gcc_custom.dll"  `
-a "${STRAWBERRY_PATH}\perl\site\lib\Alien\wxWidgets\msw_3_0_2_uni_gcc_3_4\lib\wxmsw30u_adv_gcc_custom.dll;wxmsw30u_adv_gcc_custom.dll"  `
-a "${STRAWBERRY_PATH}\perl\site\lib\Alien\wxWidgets\msw_3_0_2_uni_gcc_3_4\lib\wxmsw30u_gl_gcc_custom.dll;wxmsw30u_gl_gcc_custom.dll"  `
-a "${STRAWBERRY_PATH}\perl\site\lib\Alien\wxWidgets\msw_3_0_2_uni_gcc_3_4\lib\wxmsw30u_core_gcc_custom.dll;wxmsw30u_core_gcc_custom.dll"  `
-a "${STRAWBERRY_PATH}\perl\site\lib\Alien\wxWidgets\msw_3_0_2_uni_gcc_3_4\lib\wxmsw30u_html_gcc_custom.dll;wxmsw30u_html_gcc_custom.dll"  `
-a "${STRAWBERRY_PATH}\perl\site\lib\Alien\wxWidgets\msw_3_0_2_uni_gcc_3_4\lib\wxbase30u_xml_gcc_custom.dll;wxbase30u_xml_gcc_custom.dll"  `
-a "${STRAWBERRY_PATH}\perl\site\lib\Alien\wxWidgets\msw_3_0_2_uni_gcc_3_4\lib\wxbase30u_net_gcc_custom.dll;wxbase30u_net_gcc_custom.dll"  `
-a "../lib;lib" `
-a "../slic3r.pl;slic3r.pl" `
-M AutoLoader `
-M B `
-M Carp `
-M Class::Accessor `
-M Class::XSAccessor `
-M Class::XSAccessor::Heavy `
-M Config `
-M Crypt::CBC `
-M Cwd `
-M Data::UUID `
-M Devel::GlobalDestruction `
-M Digest `
-M Digest::MD5 `
-M Digest::SHA `
-M Digest::base `
-M DynaLoader `
-M Encode `
-M Encode::Alias `
-M Encode::Byte `
-M Encode::Config `
-M Encode::Encoding `
-M Encode::Locale `
-M Encode::MIME::Name `
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
-M Getopt::Long `
-M Growl::GNTP `
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
-M IO::Socket::INET `
-M IO::Socket::INET6 `
-M IO::Socket::IP `
-M IO::Socket::UNIX `
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
-M Moo `
-M Moo::HandleMoose `
-M Moo::Object `
-M Moo::Role `
-M Moo::sification `
-M Net::Bonjour `
-M Net::Bonjour::Entry `
-M Net::DNS `
-M Net::DNS::Domain `
-M Net::DNS::DomainName `
-M Net::DNS::Header `
-M Net::DNS::Packet `
-M Net::DNS::Parameters `
-M Net::DNS::Question `
-M Net::DNS::RR `
-M Net::DNS::RR::OPT `
-M Net::DNS::RR::PTR `
-M Net::DNS::Resolver `
-M Net::DNS::Resolver `
-M Net::DNS::Resolver::Base `
-M Net::DNS::Resolver::MSWin32 `
-M Net::DNS::Update `
-M Net::HTTP `
-M Net::HTTP::Methods `
-M OpenGL `
-M POSIX `
-M Pod::Escapes `
-M Pod::Text `
-M Pod::Usage `
-M Role::Tiny `
-M Scalar::Util `
-M SelectSaver `
-M Slic3r::* `
-M Slic3r::XS `
-M Socket `
-M Socket6 `
-M Storable `
-M Sub::Defer `
-M Sub::Exporter `
-M Sub::Exporter::Progressive `
-M Sub::Name `
-M Sub::Quote `
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
-M Time::HiRes `
-M Time::Local `
-M URI `
-M URI::Escape `
-M URI::http `
-M Unicode::Normalize `
-M Win32 `
-M Win32::API `
-M Win32::API::Struct `
-M Win32::API::Type `
-M Win32::IPHelper `
-M Win32::TieRegistry `
-M Win32::WinError `
-M Win32API::Registry `
-M Wx `
-M Wx::App `
-M Wx::DND `
-M Wx::DropSource `
-M Wx::Event `
-M Wx::GLCanvas `
-M Wx::Grid `
-M Wx::Html `
-M Wx::Locale `
-M Wx::Menu `
-M Wx::Mini `
-M Wx::Print `
-M Wx::RadioBox `
-M Wx::Timer `
-M XSLoader `
-M attributes `
-M base `
-M bytes `
-M constant `
-M constant `
-M constant::defer `
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
-M threads `
-M threads::shared `
-M utf8 `
-M vars `
-M warnings `
-M warnings::register `
-e -p ..\slic3r.pl -o ..\slic3r.par

copy ..\slic3r.par "..\slic3r-${current_branch}-${APPVEYOR_BUILD_NUMBER}-$(git rev-parse --short HEAD).zip"
