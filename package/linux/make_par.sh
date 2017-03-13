#!/bin/bash 
# PAR Packaging script for Slic3r
# Written by Joseph Lenox
# Set $SLIC3R_USE_SYSTEM_LIB if you don't want to use the compiled libraries from Slic3r's local-lib directory.

if [ "$#" -ne 1 ]; then
    echo "Usage: $(basename $0) par_name"
    exit 1;
fi

WD=$(dirname $0)
PERL_BIN=$(which perl)
PP_BIN=$(which pp)
SLIC3R_VERSION=$(grep "VERSION" xs/src/libslic3r/libslic3r.h | awk -F\" '{print $2}')

if [ $(git describe &>/dev/null) ]; then
    TAGGED=true
    SLIC3R_BUILD_ID=$(git describe)
else
    TAGGED=false
    SLIC3R_BUILD_ID=${SLIC3R_VERSION}
fi
if [ "$TRAVIS_PULL_REQUEST_BRANCH" != "" ]; then
    current_branch=$TRAVIS_PULL_REQUEST_BRANCH
else
    current_branch="unknown"
    if [ ! -z ${TRAVIS_BRANCH+x} ]; then
        echo "Setting to TRAVIS_BRANCH"
        current_branch=$(echo $TRAVIS_BRANCH | cut -d / -f 2)
    fi
    if [ ! -z ${APPVEYOR_REPO_BRANCH+x} ]; then
        echo "Setting to APPVEYOR_REPO_BRANCH"
        current_branch=$APPVEYOR_REPO_BRANCH
    fi
fi

# If we're on a branch, add the branch name to the app name.
if [ "$current_branch" == "master" ]; then
    appname=Slic3r
    parfile=slic3r-${SLIC3R_BUILD_ID}-${1}
else
    appname=Slic3r-$(git symbolic-ref HEAD | sed 's!refs\/heads\/!!')
    parfile=slic3r-${SLIC3R_BUILD_ID}-${1}-$(git symbolic-ref HEAD | sed 's!refs\/heads\/!!')
fi

SLIC3R_DIR=$(perl -MCwd=realpath -e "print realpath '${WD}/../../'")

# use local-lib unless otherwise indicated
if [ -z $SLIC3R_USE_SYSTEM_LIB ]; then
    eval $(perl -Mlocal::lib=${SLIC3R_DIR}/local-lib)
fi

# Ensure that PAR is installed.

if [ $(perl -MPAR::Packer -e 1) ]; then
    cpanm PAR::Packer 
fi
# Run PAR to get the correct 
pp \
       -M Algorithm::Diff \
       -M Algorithm::DiffOld \
       -M Alien::wxWidgets \
       -M App::cpanminus \
       -M App::cpanminus::fatscript \
       -M B \
       -M Capture::Tiny \
       -M Class::Accessor \
       -M Class::Accessor::Fast \
       -M Class::Accessor::Faster \
       -M Class::Method::Modifiers \
       -M Class::XSAccessor \
       -M Class::XSAccessor::Array \
       -M Class::XSAccessor::Heavy \
       -M Crypt::CBC \
       -M Data::UUID \
       -M Devel::CheckLib \
       -M Devel::GlobalDestruction \
       -M Devel::InnerPackage \
       -M Digest::HMAC \
       -M Digest::HMAC_MD5 \
       -M Digest::HMAC_SHA1 \
       -M Encode \
       -M Encode::Alias \
       -M Encode::Byte \
       -M Encode::CJKConstants \
       -M Encode::CN \
       -M Encode::CN::HZ \
       -M Encode::Config \
       -M Encode::EBCDIC \
       -M Encode::Encoder \
       -M Encode::Encoding \
       -M Encode::GSM0338 \
       -M Encode::Guess \
       -M Encode::JP \
       -M Encode::JP::H2Z \
       -M Encode::JP::JIS7 \
       -M Encode::KR \
       -M Encode::KR::2022_KR \
       -M Encode::Locale \
       -M Encode::MIME::Header \
       -M Encode::MIME::Header::ISO_2022_JP \
       -M Encode::MIME::Name \
       -M Encode::Symbol \
       -M Encode::TW \
       -M Encode::Unicode \
       -M Encode::Unicode::UTF7 \
       -M ExtUtils::Command \
       -M ExtUtils::Command::MM \
       -M ExtUtils::Config \
       -M ExtUtils::CppGuess \
       -M ExtUtils::Helpers \
       -M ExtUtils::Helpers::Unix \
       -M ExtUtils::Helpers::VMS \
       -M ExtUtils::Helpers::Windows \
       -M ExtUtils::InstallPaths \
       -M ExtUtils::Liblist \
       -M ExtUtils::Liblist::Kid \
       -M ExtUtils::MM \
       -M ExtUtils::MM_AIX \
       -M ExtUtils::MM_Any \
       -M ExtUtils::MM_BeOS \
       -M ExtUtils::MM_Cygwin \
       -M ExtUtils::MM_DOS \
       -M ExtUtils::MM_Darwin \
       -M ExtUtils::MM_MacOS \
       -M ExtUtils::MM_NW5 \
       -M ExtUtils::MM_OS2 \
       -M ExtUtils::MM_QNX \
       -M ExtUtils::MM_UWIN \
       -M ExtUtils::MM_Unix \
       -M ExtUtils::MM_VMS \
       -M ExtUtils::MM_VOS \
       -M ExtUtils::MM_Win32 \
       -M ExtUtils::MM_Win95 \
       -M ExtUtils::MY \
       -M ExtUtils::MakeMaker \
       -M ExtUtils::MakeMaker::Config \
       -M ExtUtils::MakeMaker::Locale \
       -M ExtUtils::MakeMaker::version \
       -M ExtUtils::MakeMaker::version::regex \
       -M ExtUtils::MakeMaker::version::vpp \
       -M ExtUtils::Mkbootstrap \
       -M ExtUtils::Mksymlists \
       -M ExtUtils::ParseXS \
       -M ExtUtils::ParseXS::Constants \
       -M ExtUtils::ParseXS::CountLines \
       -M ExtUtils::ParseXS::Eval \
       -M ExtUtils::ParseXS::Utilities \
       -M ExtUtils::Typemap::Basic \
       -M ExtUtils::Typemap::Default \
       -M ExtUtils::Typemap::ObjectMap \
       -M ExtUtils::Typemap::STL \
       -M ExtUtils::Typemap::STL::String \
       -M ExtUtils::Typemap::STL::Vector \
       -M ExtUtils::Typemaps \
       -M ExtUtils::Typemaps::Basic \
       -M ExtUtils::Typemaps::Cmd \
       -M ExtUtils::Typemaps::Default \
       -M ExtUtils::Typemaps::InputMap \
       -M ExtUtils::Typemaps::ObjectMap \
       -M ExtUtils::Typemaps::OutputMap \
       -M ExtUtils::Typemaps::STL \
       -M ExtUtils::Typemaps::STL::List \
       -M ExtUtils::Typemaps::STL::String \
       -M ExtUtils::Typemaps::STL::Vector \
       -M ExtUtils::Typemaps::Type \
       -M ExtUtils::XSpp \
       -M ExtUtils::XSpp::Cmd \
       -M ExtUtils::XSpp::Driver \
       -M ExtUtils::XSpp::Exception \
       -M ExtUtils::XSpp::Exception::code \
       -M ExtUtils::XSpp::Exception::object \
       -M ExtUtils::XSpp::Exception::perlcode \
       -M ExtUtils::XSpp::Exception::simple \
       -M ExtUtils::XSpp::Exception::stdmessage \
       -M ExtUtils::XSpp::Exception::unknown \
       -M ExtUtils::XSpp::Grammar \
       -M ExtUtils::XSpp::Lexer \
       -M ExtUtils::XSpp::Node \
       -M ExtUtils::XSpp::Node::Access \
       -M ExtUtils::XSpp::Node::Argument \
       -M ExtUtils::XSpp::Node::Class \
       -M ExtUtils::XSpp::Node::Comment \
       -M ExtUtils::XSpp::Node::Constructor \
       -M ExtUtils::XSpp::Node::Destructor \
       -M ExtUtils::XSpp::Node::Enum \
       -M ExtUtils::XSpp::Node::EnumValue \
       -M ExtUtils::XSpp::Node::File \
       -M ExtUtils::XSpp::Node::Function \
       -M ExtUtils::XSpp::Node::Member \
       -M ExtUtils::XSpp::Node::Method \
       -M ExtUtils::XSpp::Node::Module \
       -M ExtUtils::XSpp::Node::Package \
       -M ExtUtils::XSpp::Node::PercAny \
       -M ExtUtils::XSpp::Node::Preprocessor \
       -M ExtUtils::XSpp::Node::Raw \
       -M ExtUtils::XSpp::Node::Type \
       -M ExtUtils::XSpp::Parser \
       -M ExtUtils::XSpp::Plugin::feature::default_xs_typemap \
       -M ExtUtils::XSpp::Typemap \
       -M ExtUtils::XSpp::Typemap::parsed \
       -M ExtUtils::XSpp::Typemap::reference \
       -M ExtUtils::XSpp::Typemap::simple \
       -M ExtUtils::XSpp::Typemap::wrapper \
       -M ExtUtils::testlib \
       -M File::Listing \
       -M FindBin \
       -M Getopt::Long \
       -M Growl::GNTP \
       -M HTML::Entities \
       -M HTML::Filter \
       -M HTML::HeadParser \
       -M HTML::LinkExtor \
       -M HTML::Parser \
       -M HTML::PullParser \
       -M HTML::Tagset \
       -M HTML::TokeParser \
       -M HTTP::Config \
       -M HTTP::Cookies \
       -M HTTP::Cookies::Microsoft \
       -M HTTP::Cookies::Netscape \
       -M HTTP::Daemon \
       -M HTTP::Date \
       -M HTTP::Headers \
       -M HTTP::Headers::Auth \
       -M HTTP::Headers::ETag \
       -M HTTP::Headers::Util \
       -M HTTP::Message \
       -M HTTP::Negotiate \
       -M HTTP::Request \
       -M HTTP::Request::Common \
       -M HTTP::Response \
       -M HTTP::Status \
       -M IO::AtomicFile \
       -M IO::CaptureOutput \
       -M IO::HTML \
       -M IO::InnerFile \
       -M IO::Lines \
       -M IO::Scalar \
       -M IO::ScalarArray \
       -M IO::Stringy \
       -M IO::Wrap \
       -M IO::WrapTie \
       -M LWP \
       -M LWP::Authen::Basic \
       -M LWP::Authen::Digest \
       -M LWP::Authen::Ntlm \
       -M LWP::ConnCache \
       -M LWP::Debug \
       -M LWP::Debug::TraceHTTP \
       -M LWP::DebugFile \
       -M LWP::MediaTypes \
       -M LWP::MemberMixin \
       -M LWP::Protocol \
       -M LWP::Protocol::cpan \
       -M LWP::Protocol::data \
       -M LWP::Protocol::file \
       -M LWP::Protocol::ftp \
       -M LWP::Protocol::gopher \
       -M LWP::Protocol::http \
       -M LWP::Protocol::loopback \
       -M LWP::Protocol::mailto \
       -M LWP::Protocol::nntp \
       -M LWP::Protocol::nogo \
       -M LWP::RobotUA \
       -M LWP::Simple \
       -M LWP::UserAgent \
       -M List::Util \
       -M List::Util::XS \
       -M Math::Trig \
       -M Method::Generate::Accessor \
       -M Method::Generate::BuildAll \
       -M Method::Generate::Constructor \
       -M Method::Generate::DemolishAll \
       -M Module::Build \
       -M Module::Build::Base \
       -M Module::Build::Compat \
       -M Module::Build::Config \
       -M Module::Build::ConfigData \
       -M Module::Build::Cookbook \
       -M Module::Build::Dumper \
       -M Module::Build::Notes \
       -M Module::Build::PPMMaker \
       -M Module::Build::Platform::Default \
       -M Module::Build::Platform::MacOS \
       -M Module::Build::Platform::Unix \
       -M Module::Build::Platform::VMS \
       -M Module::Build::Platform::VOS \
       -M Module::Build::Platform::Windows \
       -M Module::Build::Platform::aix \
       -M Module::Build::Platform::cygwin \
       -M Module::Build::Platform::darwin \
       -M Module::Build::Platform::os2 \
       -M Module::Build::PodParser \
       -M Module::Build::Tiny \
       -M Module::Build::WithXSpp \
       -M Module::Pluggable \
       -M Module::Pluggable::Object \
       -M Module::Runtime \
       -M Moo \
       -M Moo::HandleMoose \
       -M Moo::HandleMoose::FakeMetaClass \
       -M Moo::HandleMoose::_TypeMap \
       -M Moo::Object \
       -M Moo::Role \
       -M Moo::_Utils \
       -M Moo::_mro \
       -M Moo::_strictures \
       -M Moo::sification \
       -M Net::Bonjour \
       -M Net::Bonjour::Entry \
       -M Net::DNS \
       -M Net::DNS::Domain \
       -M Net::DNS::DomainName \
       -M Net::DNS::Header \
       -M Net::DNS::Mailbox \
       -M Net::DNS::Nameserver \
       -M Net::DNS::Packet \
       -M Net::DNS::Parameters \
       -M Net::DNS::Question \
       -M Net::DNS::RR \
       -M Net::DNS::RR::A \
       -M Net::DNS::RR::AAAA \
       -M Net::DNS::RR::AFSDB \
       -M Net::DNS::RR::APL \
       -M Net::DNS::RR::CAA \
       -M Net::DNS::RR::CDNSKEY \
       -M Net::DNS::RR::CDS \
       -M Net::DNS::RR::CERT \
       -M Net::DNS::RR::CNAME \
       -M Net::DNS::RR::CSYNC \
       -M Net::DNS::RR::DHCID \
       -M Net::DNS::RR::DLV \
       -M Net::DNS::RR::DNAME \
       -M Net::DNS::RR::DNSKEY \
       -M Net::DNS::RR::DS \
       -M Net::DNS::RR::EUI48 \
       -M Net::DNS::RR::EUI64 \
       -M Net::DNS::RR::GPOS \
       -M Net::DNS::RR::HINFO \
       -M Net::DNS::RR::HIP \
       -M Net::DNS::RR::IPSECKEY \
       -M Net::DNS::RR::ISDN \
       -M Net::DNS::RR::KEY \
       -M Net::DNS::RR::KX \
       -M Net::DNS::RR::L32 \
       -M Net::DNS::RR::L64 \
       -M Net::DNS::RR::LOC \
       -M Net::DNS::RR::LP \
       -M Net::DNS::RR::MB \
       -M Net::DNS::RR::MG \
       -M Net::DNS::RR::MINFO \
       -M Net::DNS::RR::MR \
       -M Net::DNS::RR::MX \
       -M Net::DNS::RR::NAPTR \
       -M Net::DNS::RR::NID \
       -M Net::DNS::RR::NS \
       -M Net::DNS::RR::NSEC \
       -M Net::DNS::RR::NSEC3 \
       -M Net::DNS::RR::NSEC3PARAM \
       -M Net::DNS::RR::NULL \
       -M Net::DNS::RR::OPENPGPKEY \
       -M Net::DNS::RR::OPT \
       -M Net::DNS::RR::PTR \
       -M Net::DNS::RR::PX \
       -M Net::DNS::RR::RP \
       -M Net::DNS::RR::RRSIG \
       -M Net::DNS::RR::RT \
       -M Net::DNS::RR::SIG \
       -M Net::DNS::RR::SMIMEA \
       -M Net::DNS::RR::SOA \
       -M Net::DNS::RR::SPF \
       -M Net::DNS::RR::SRV \
       -M Net::DNS::RR::SSHFP \
       -M Net::DNS::RR::TKEY \
       -M Net::DNS::RR::TLSA \
       -M Net::DNS::RR::TSIG \
       -M Net::DNS::RR::TXT \
       -M Net::DNS::RR::URI \
       -M Net::DNS::RR::X25 \
       -M Net::DNS::Resolver \
       -M Net::DNS::Resolver::Base \
       -M Net::DNS::Resolver::MSWin32 \
       -M Net::DNS::Resolver::Recurse \
       -M Net::DNS::Resolver::UNIX \
       -M Net::DNS::Resolver::android \
       -M Net::DNS::Resolver::cygwin \
       -M Net::DNS::Resolver::os2 \
       -M Net::DNS::Text \
       -M Net::DNS::Update \
       -M Net::DNS::ZoneFile \
       -M Net::HTTP \
       -M Net::HTTP::Methods \
       -M Net::HTTP::NB \
       -M Net::HTTPS \
       -M Net::Rendezvous \
       -M Net::Rendezvous::Entry \
       -M OpenGL \
       -M OpenGL::Config \
       -M POSIX \
       -M Role::Tiny \
       -M Role::Tiny::With \
       -M Scalar::Util \
       -M Slic3r::XS \
       -M Socket \
       -M Spiffy \
       -M Spiffy::mixin \
       -M Sub::Defer \
       -M Sub::Exporter::Progressive \
       -M Sub::Quote \
       -M Sub::Util \
       -M Test2 \
       -M Test2::API \
       -M Test2::API::Breakage \
       -M Test2::API::Context \
       -M Test2::API::Instance \
       -M Test2::API::Stack \
       -M Test2::Event \
       -M Test2::Event::Bail \
       -M Test2::Event::Diag \
       -M Test2::Event::Encoding \
       -M Test2::Event::Exception \
       -M Test2::Event::Generic \
       -M Test2::Event::Info \
       -M Test2::Event::Note \
       -M Test2::Event::Ok \
       -M Test2::Event::Plan \
       -M Test2::Event::Skip \
       -M Test2::Event::Subtest \
       -M Test2::Event::TAP::Version \
       -M Test2::Event::Waiting \
       -M Test2::Formatter \
       -M Test2::Formatter::TAP \
       -M Test2::Hub \
       -M Test2::Hub::Interceptor \
       -M Test2::Hub::Interceptor::Terminator \
       -M Test2::Hub::Subtest \
       -M Test2::IPC \
       -M Test2::IPC::Driver \
       -M Test2::IPC::Driver::Files \
       -M Test2::Tools::Tiny \
       -M Test2::Util \
       -M Test2::Util::ExternalMeta \
       -M Test2::Util::HashBase \
       -M Test2::Util::Trace \
       -M Test::Base \
       -M Test::Base::Filter \
       -M Test::Builder \
       -M Test::Builder::Formatter \
       -M Test::Builder::IO::Scalar \
       -M Test::Builder::Module \
       -M Test::Builder::Tester \
       -M Test::Builder::Tester::Color \
       -M Test::Builder::TodoDiag \
       -M Test::Differences \
       -M Test::Fatal \
       -M Test::More \
       -M Test::Requires \
       -M Test::RequiresInternet \
       -M Test::Simple \
       -M Test::Tester \
       -M Test::Tester::Capture \
       -M Test::Tester::CaptureRunner \
       -M Test::Tester::Delegate \
       -M Test::use::ok \
       -M Text::Diff \
       -M Text::Diff::Config \
       -M Text::Diff::Table \
       -M Thread::Queue \
       -M Thread::Semaphore \
       -M Tie::Handle \
       -M Time::HiRes \
       -M Time::Local \
       -M Try::Tiny \
       -M URI \
       -M URI::Escape \
       -M URI::Heuristic \
       -M URI::IRI \
       -M URI::QueryParam \
       -M URI::Split \
       -M URI::URL \
       -M URI::WithBase \
       -M URI::_foreign \
       -M URI::_generic \
       -M URI::_idna \
       -M URI::_ldap \
       -M URI::_login \
       -M URI::_punycode \
       -M URI::_query \
       -M URI::_segment \
       -M URI::_server \
       -M URI::_userpass \
       -M URI::data \
       -M URI::file \
       -M URI::file::Base \
       -M URI::file::FAT \
       -M URI::file::Mac \
       -M URI::file::OS2 \
       -M URI::file::QNX \
       -M URI::file::Unix \
       -M URI::file::Win32 \
       -M URI::ftp \
       -M URI::gopher \
       -M URI::http \
       -M URI::https \
       -M URI::ldap \
       -M URI::ldapi \
       -M URI::ldaps \
       -M URI::mailto \
       -M URI::mms \
       -M URI::news \
       -M URI::nntp \
       -M URI::pop \
       -M URI::rlogin \
       -M URI::rsync \
       -M URI::rtsp \
       -M URI::rtspu \
       -M URI::sftp \
       -M URI::sip \
       -M URI::sips \
       -M URI::snews \
       -M URI::ssh \
       -M URI::telnet \
       -M URI::tn3270 \
       -M URI::urn \
       -M URI::urn::isbn \
       -M URI::urn::oid \
       -M Unicode::Normalize \
       -M WWW::RobotRules \
       -M WWW::RobotRules::AnyDBM_File \
       -M Wx \
       -M Wx::AUI \
       -M Wx::App \
       -M Wx::ArtProvider \
       -M Wx::Calendar \
       -M Wx::DND \
       -M Wx::DataView \
       -M Wx::DateTime \
       -M Wx::DocView \
       -M Wx::DropSource \
       -M Wx::Event \
       -M Wx::FS \
       -M Wx::GLCanvas \
       -M Wx::Grid \
       -M Wx::Help \
       -M Wx::Html \
       -M Wx::IPC \
       -M Wx::Locale \
       -M Wx::MDI \
       -M Wx::Media \
       -M Wx::Menu \
       -M Wx::Mini \
       -M Wx::Overload::Driver \
       -M Wx::Overload::Handle \
       -M Wx::Perl::Carp \
       -M Wx::Perl::SplashFast \
       -M Wx::Perl::TextValidator \
       -M Wx::PerlTest \
       -M Wx::Print \
       -M Wx::PropertyGrid \
       -M Wx::RadioBox \
       -M Wx::Ribbon \
       -M Wx::RichText \
       -M Wx::STC \
       -M Wx::Socket \
       -M Wx::Timer \
       -M Wx::Wx_Exp \
       -M Wx::XRC \
       -M Wx::XSP::Enum \
       -M Wx::XSP::Event \
       -M Wx::XSP::Overload \
       -M Wx::XSP::Virtual \
       -M Wx::build::MakeMaker \
       -M Wx::build::MakeMaker::Any_OS \
       -M Wx::build::MakeMaker::Any_wx_config \
       -M Wx::build::MakeMaker::Core \
       -M Wx::build::MakeMaker::Hacks \
       -M Wx::build::MakeMaker::MacOSX_GCC \
       -M Wx::build::MakeMaker::Win32 \
       -M Wx::build::MakeMaker::Win32_MSVC \
       -M Wx::build::MakeMaker::Win32_MinGW \
       -M Wx::build::Opt \
       -M Wx::build::Options \
       -M Wx::build::Utils \
       -M attributes \
       -M base \
       -M bytes \
       -M encoding \
       -M lib \
       -M ok \
       -M oo \
       -M overload \
       -M parent \
       -M strict \
       -M threads \
       -M threads::shared \
       -M utf8 \
       -M warnings \
       -M Slic3r::XS \
       -a "lib;lib" \
       -B ${SLIC3R_DIR}/slic3r.pl -o ${WD}/${parfile}

tar -czvf ${WD}/${parfile}.tar.gz ${WD}/${parfile}
mv -v ${WD}/${parfile} ${SLIC3R_DIR}
