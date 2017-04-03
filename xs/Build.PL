#!/usr/bin/perl -w

use strict;
use warnings;

use Config;
use Devel::CheckLib;
use ExtUtils::CppGuess;
use Module::Build::WithXSpp;

my $cpp_guess = ExtUtils::CppGuess->new;
my $mswin = $^O eq 'MSWin32';

# _GLIBCXX_USE_C99 : to get the long long type for g++
# HAS_BOOL         : stops Perl/lib/CORE/handy.h from doing "#  define bool char" for MSVC
# NOGDI            : prevents inclusion of wingdi.h which defines functions Polygon() and Polyline() in global namespace
# BOOST_ASIO_DISABLE_KQUEUE : prevents a Boost ASIO bug on OS X: https://svn.boost.org/trac/boost/ticket/5339
my @cflags = qw(-D_GLIBCXX_USE_C99 -DHAS_BOOL -DNOGDI -DSLIC3RXS -DBOOST_ASIO_DISABLE_KQUEUE);
if (defined($ENV{'CFLAGS'})) {
    # push environment cflags
    push @cflags, split / /, $ENV{'CFLAGS'};
}
if (defined($ENV{'CXXFLAGS'})) {
    # push environment cxxflags
    push @cflags, split / /, $ENV{'CXXFLAGS'};
}
if ($cpp_guess->is_gcc)	{
	# GCC is pedantic with c++11 std, so undefine strict ansi to get M_PI back
	push @cflags, qw(-U__STRICT_ANSI__);
}
if (`$Config{cc} -v` =~ /gcc version 4\.6\./) {
    # Compatibility with GCC 4.6 is not a requirement, but as long as code compiles on it we try to support it.
    push @cflags, qw(-std=c++0x);
} else {
    # std=c++11 Enforce usage of C++11 (required now). Minimum compiler supported: gcc 4.9, clang 3.3, MSVC 14.0
    push @cflags, qw(-std=c++11);
}
my @ldflags = ();
if (defined($ENV{'LDFLAGS'})) {
    push @ldflags, split / /, $ENV{'LDFLAGS'};
}

if ($^O eq 'darwin') {
    push @cflags, qw(-stdlib=libc++);
    push @ldflags, qw(-framework IOKit -framework CoreFoundation -lc++);
    
    # Due to a bug/misconfiguration/stupidity, boost 1.52 and libc++ don't like each
    # other much: a compilation error "Constexpr function never produces a constant
    # expression" pops up when trying to compile anything that uses
    # boost/chrono/duration.hpp (namely boost/thread for us). This is a workaround
    # that prevents this from happening, not needed with newer Boost versions.
    # See here for more details: https://svn.boost.org/trac/boost/ticket/7671
    push @cflags, qw(-DBOOST_THREAD_DONT_USE_CHRONO -DBOOST_NO_CXX11_RVALUE_REFERENCES -DBOOST_THREAD_USES_MOVE);
    
    # ExtUtils::CppGuess has a hard-coded -lstdc++, so we filter it out
    {
        no strict 'refs';
        no warnings 'redefine';
        my $func = "ExtUtils::CppGuess::_get_lflags";
        my $orig = *$func{CODE};
        *{$func} = sub {
            my $lflags = $orig->(@_);
            $lflags =~ s/\s*-lstdc\+\+//;
            return $lflags;
        };
    }
}

my @early_includes = ();

if ($mswin) {
    # In case windows.h is included, we don't want the min / max macros to be active.
    # If <math.h> is included, we want the #defines to be active (M_PI etc.)
    push @cflags, qw(-DNOMINMAX -D_USE_MATH_DEFINES);
}

my @INC  = qw(-Isrc/libslic3r);
my @LIBS = $cpp_guess->is_msvc ? qw(-LIBPATH:src/libslic3r) : qw(-Lsrc/libslic3r);

if ($ENV{SLIC3R_GUI}) {
    print "Slic3r will be built with GUI support\n";
    require Alien::wxWidgets;
    Alien::wxWidgets->load;
    push @INC, Alien::wxWidgets->include_path;
    push @cflags, qw(-DSLIC3R_GUI -DUNICODE), Alien::wxWidgets->defines, Alien::wxWidgets->c_flags;
    push @ldflags, Alien::wxWidgets->link_flags, Alien::wxWidgets->libraries(qw(gl html));
    push @early_includes, qw(slic3r/GUI/wxinit.h);
}

# search for Boost in a number of places
my @boost_include = ();
if (defined $ENV{BOOST_INCLUDEDIR}) {
    push @boost_include, $ENV{BOOST_INCLUDEDIR}
} elsif (defined $ENV{BOOST_DIR}) {
    my $subdir = $ENV{BOOST_DIR} . (($mswin == 1) ? '\include' : '/include');
    if (-d $subdir) {
        push @boost_include, $subdir;
    } elsif (-d "../$subdir") {
        # User might have provided a path relative to the Build.PL in the main directory
        push @boost_include, "../$subdir";
    } else {
        push @boost_include, $ENV{BOOST_DIR};
    }
} else {
    # Boost library was not defined by the environment.
    # Try to guess at some default paths.
    if ($mswin) {
        for my $path (glob('C:\dev\boost*\include'), glob ('C:\boost*\include')) {
            push @boost_include, $path;
        }
        if (! @boost_include) {
            # No boost\include. Try to include the boost root.
            for my $path (glob('C:\dev\boost*'), glob ('C:\boost*')) {
                push @boost_include, $path;
            }
        }
    } else {
        push @boost_include, grep { -d $_ }
            qw(/opt/local/include /usr/local/include /opt/include /usr/include);
    }
}

my @boost_libs = ();
if (defined $ENV{BOOST_LIBRARYPATH}) {
    push @boost_libs, $ENV{BOOST_LIBRARYPATH}
} elsif (defined $ENV{BOOST_DIR}) {
    my $subdir = $ENV{BOOST_DIR} . ($mswin ? '\stage\lib' : '/stage/lib');
    if (-d $subdir) {
        push @boost_libs, $subdir;
    } elsif (-d "../$subdir") {
        # User might have provided a path relative to the Build.PL in the main directory
        push @boost_libs, "../$subdir";
    } else {
        push @boost_libs, $ENV{BOOST_DIR};
    }
} else {
    # Boost library was not defined by the environment.
    # Try to guess at some default paths.
    if ($mswin) {
        for my $path (
            glob('C:\dev\boost*\lib'),       glob ('C:\boost*\lib'), 
            glob('C:\dev\boost*\stage\lib'), glob ('C:\boost*\stage\lib')) {
            push @boost_libs, $path;
        }
    } else {
        push @boost_libs, grep { -d $_ }
            qw(/opt/local/lib /usr/local/lib /opt/lib /usr/lib /lib);
    }
}
# In order to generate the -l switches we need to know how Boost libraries are named
my $have_boost = 0;
my @boost_libraries = qw(system thread filesystem);  # we need these
# check without explicit lib path (works on Linux)
if (! $mswin) {
    $have_boost = 1
        if check_lib(
            lib     => [ map "boost_${_}", @boost_libraries ],
        );
}

if (!$ENV{SLIC3R_STATIC} && $have_boost) {
    # The boost library was detected by check_lib on Linux.
    push @LIBS, map "-lboost_${_}", @boost_libraries;
} else {
    # Either static linking, or check_lib could not be used to find the boost libraries.
    my $lib_prefix = 'libboost_';
    my $lib_ext = $Config{lib_ext};
    PATH: foreach my $path (@boost_libs) {
        # Try to find the boost system library.
        my @files = glob "$path/${lib_prefix}system*$lib_ext";
        next if !@files;
  
        if ($files[0] =~ /${lib_prefix}system([^.]*)$lib_ext$/) {
            # Suffix contains the version number, the build type etc.
            my $suffix = $1;
            # Verify existence of all required boost libraries at $path.
            for my $lib (map "${lib_prefix}${_}${suffix}${lib_ext}", @boost_libraries) {
                # If the library file does not exist, try next library path.
                -f "$path/$lib" or next PATH;
            }
            if (! $cpp_guess->is_msvc) {
                # Test the correctness of boost libraries by linking them to a minimal C program.
                check_lib(
                    lib     => [ map "boost_${_}${suffix}", @boost_libraries ],
                    INC     => join(' ', map "-I$_", @INC,  @boost_include),
                    LIBS    => "-L$path",
                ) or next;
            }
            push @INC, (map " -I$_", @boost_include);  # TODO: only use the one related to the chosen lib path
            if ($ENV{SLIC3R_STATIC} || $cpp_guess->is_msvc) {
                push @LIBS, map "${path}/${lib_prefix}$_${suffix}${lib_ext}", @boost_libraries;
            } else {
                push @LIBS, " -L$path", (map " -lboost_$_$suffix", @boost_libraries);
            }
            $have_boost = 1;
            last;
        }
    }
}
push @cflags, '-DBOOST_LIBS' if $have_boost;
die <<'EOF' if !$have_boost;
Slic3r requires the Boost libraries. Please make sure they are installed.

If they are installed, this script should be able to locate them in several
standard locations. If this is not the case, you might want to supply their 
path through the BOOST_INCLUDEPATH and BOOST_LIBRARYPATH environment variables:

    BOOST_INCLUDEPATH=/usr/local/include BOOST_LIBRARYPATH=/usr/lib perl Build.PL

If you just compiled Boost in its source directory without installing it in the
system you can just provide the BOOST_DIR variable pointing to that directory.

EOF

if ($ENV{SLIC3R_DEBUG}) {
    # only on newer GCCs: -ftemplate-backtrace-limit=0
    push @cflags, '-DSLIC3R_DEBUG';
    push @cflags, $cpp_guess->is_msvc ? '-Gd' : '-g';
} else {
    # Disable asserts in the release builds.
    push @cflags, '-DNDEBUG';
}
if ($cpp_guess->is_gcc) {
    # check whether we're dealing with a buggy GCC version
    # see https://github.com/alexrj/Slic3r/issues/1965
    if (`cc --version` =~ / 4\.7\.[012]/) {
        # Workaround suggested by Boost devs:
        # https://svn.boost.org/trac/boost/ticket/8695
        push @cflags, qw(-fno-inline-small-functions);
    }
}

if ($mswin) {
    # like boost, Windows lives to make things difficult for us.
    # Use an environment option for the necesary location and pray.
    push @cflags, qw(-I$ENV{WXWIN}\\include);
    push @cflags, qw(-D__WXMSW__ -D_UNICODE);
    # add libraries
    if ($cpp_guess->is_msvc) {
        push @ldflags, qw(-L$ENV{WXWIN}\\lib -L$ENV{WXWIN}\\lib\\msvc_lib);
    } else {
        push @ldflags, qw(-L$ENV{WXWIN}\\lib -L$ENV{WXWIN}\\lib\\gcc_lib);
    }
    push @ldflags, qw(-pthread -lwx_gtk2u_xrc-3.0 -lwx_gtk2u_html-3.0 -lwx_gtk2u_qa-3.0 -lwx_gtk2u_adv-3.0 -lwx_gtk2u_core-3.0 -lwx_baseu_xml-3.0 -lwx_baseu_net-3.0 -lwx_baseu-3.0);
} else {
    # on linux/osx, wx-config is hopefully in the path and can be used.
    # if wxdir is defined (which is the prefix that wxwidgets was installed to
    # use that instead.
    my $wx_config_libs;
    my $wx_config_flags;
    if (defined($ENV{WXDIR})) {
        chomp($wx_config_libs     = `sh $ENV{WXDIR}/bin/wx-config --libs`);
        chomp($wx_config_flags     = `sh $ENV{WXDIR}/bin/wx-config --cxxflags`);
    } else {
        chomp($wx_config_libs     = `wx-config --libs`);
        chomp($wx_config_flags     = `wx-config --cxxflags`);
    }
    push @cflags, $wx_config_flags;
    push @ldflags, $wx_config_libs;
}

my $build = Module::Build::WithXSpp->new(
    module_name     => 'Slic3r::XS',
    dist_abstract   => 'XS code for Slic3r',
    build_requires => {qw(
        ExtUtils::ParseXS           3.18
        ExtUtils::Typemaps          1.00
        ExtUtils::Typemaps::Default 1.05
        ExtUtils::XSpp              0.17
        Module::Build               0.3601
        Test::More                  0
    )},
    configure_requires => {qw(
        ExtUtils::CppGuess          0.07
        Module::Build               0.38
        Module::Build::WithXSpp     0.13
    )},
    extra_compiler_flags => [ @INC, @cflags ],
    extra_linker_flags => [ @LIBS, @ldflags ],
    
    # Provides extra C typemaps that are auto-merged
    extra_typemap_modules => {
        'ExtUtils::Typemaps::Basic' => '1.05',
    },
    
    # for MSVC builds
    early_includes => [qw(
        cstring
        cstdlib
        ostream
        sstream
        libslic3r/GCodeSender.hpp
    ), @early_includes]
);

$build->create_build_script;

__END__
