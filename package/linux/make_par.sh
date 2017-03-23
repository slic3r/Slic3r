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
if [ -z "$SLIC3R_USE_SYSTEM_LIB" ]; then
    echo "Using ${SLIC3R_DIR}/local-lib"
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
       -M Devel::GlobalDestruction \
       -M Devel::InnerPackage \
       -M Digest::HMAC \
       -M Digest::HMAC_MD5 \
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
       -M File::Listing \
       -M File::Basename \
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
       -M Socket \
       -M Spiffy \
       -M Spiffy::mixin \
       -M Sub::Defer \
       -M Sub::Exporter::Progressive \
       -M Sub::Quote \
       -M Sub::Util \
       -M Thread::Queue \
       -M Thread::Semaphore \
       -M Tie::Handle \
       -M Time::HiRes \
       -M Time::Local \
       -M Try::Tiny \
       -M URI \
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
       -a "var;var" \
       -vv \
       --reusable \
       -B ${SLIC3R_DIR}/slic3r.pl -o ${WD}/${parfile}

tar -czvf ${WD}/${parfile}.tar.gz ${WD}/${parfile}
mv -v ${WD}/${parfile} ${SLIC3R_DIR}
