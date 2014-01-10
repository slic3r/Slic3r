#!/usr/bin/perl
use 5.12.0; # Requires current Citrus or Strawberry Perl 

# If you are hitting this version check
#Citrus Perl and Strawberrry perl are current Windows versions.
# Load one of them
# THEN ADD <installed perl folder>/bin folder to the $PATH environment variable.
#
#Note, if you have installed GitHub via GitHubSetup,
#GitHub may have installed an older version of Perl than is needed.
#On powershell, run the next line:
#  where.exe perl
#  
#It should say <installed folder>/perl/bin/perl.exe -- where you installed perl.
#It should NOT say "...AppData/local/Github/.../perl.exe"  
#github Prepends its folders to the $PATH variable, so you must
#Rename github's perl or otherwise handle the discrepency.


use strict;
use warnings;
use constant WIN32 => $^O eq 'MSWin32';

use Config;
use File::Spec;

my $dmake='';
my $perlp='';
if (WIN32) {
	# 2> $null suppresses printing confusing warning on console if dmake is missing
	my $redir = ' 2> \null'; #cant put this inside backquotes perl tries to sub \n!
	$dmake = `where.exe dmake.exe $redir`; #loads dmake path from windows path
	chomp $dmake;
	$perlp = `where.exe perl.exe  $redir`; #loads perl path from windows path
	if (!($perlp =~ /.*perl.exe.*/i )) {
		die "******** Windows does not have perl.exe in its path. ********\n******** Some build and test processes will fail. ********\n";
	}
} else {
	# probably should be $dmake = `which dmakeYY.exe $redir` on unix, but since i cant test that...
	$dmake = "dummy"; #assume it is there (previous linux behavior)
}

#Module::Build::WithXSpp			0		
my %prereqs = qw(
    Encode::Locale                  0
    ExtUtils::MakeMaker             6.80
    ExtUtils::ParseXS               3.22
    File::Basename                  0
    File::Spec                      0
    Getopt::Long                    0
    Math::Geometry::Voronoi         1.3
    Math::PlanePath                 53
    Module::Build::WithXSpp         0.14
    Moo                             1.003001
    Scalar::Util                    0
    Storable                        0
    Test::Harness                   0
    Test::More                      0
    IO::Scalar                      0
    Time::HiRes                     0
);
my %recommends = qw(
    Class::XSAccessor               0
    XML::SAX::ExpatXS               0
);

my $gui = defined $ARGV[0] && $ARGV[0] eq '--gui';
if ($gui) {
    %prereqs = qw(
    Wx                              0.9918
    );
    %recommends = qw(
    Growl::GNTP                     0.15
    Wx::GLCanvas                    0
    OpenGL                          0
    );
} 

if ($dmake eq '') {
die <<'dEOF'
dmake was not found. 
dmake.exe is required to run cpanm or install cpanm.

You must install mingw, which supplies dmake, gcc, etc.

There are several ways to install mingw, try one of these:
If you installed Citrus Perl, 
run  citrusutils.exe
then click "set gcc" button, and allow it to download and install mingw
THEN ADD
<mingw installed folder>/bin to $PATH env variable.

otherwise, install mingw directly to for gcc, dmake etc.    

THEN ADD
<mingw installed folder>/bin to $PATH env variable.

dEOF
}

my @missing_prereqs = ();
if ($ENV{SLIC3R_NO_AUTO}) {
    foreach my $module (sort keys %prereqs) {
        my $version = $prereqs{$module};
        next if eval "use $module $version; 1";
        push @missing_prereqs, $module if exists $prereqs{$module};
        print "Missing prerequisite $module $version\n";
    }
    foreach my $module (sort keys %recommends) {
        my $version = $recommends{$module};
        next if eval "use $module $version; 1";
        print "Missing optional $module $version\n";
    }
} else {

    my @try = (
        $ENV{CPANM} // (),
        File::Spec->catfile($Config{sitebin}, 'cpanm'),
        File::Spec->catfile($Config{installscript}, 'cpanm'),
    );
    
    my $cpanm;
    foreach my $path (@try) {
        if (-e $path) {  # don't use -x because it fails on Windows
            $cpanm = $path;
            last;
        }
    }
    if (!$cpanm) {
        if ($^O =~ /^(?:darwin|linux)$/ && system(qw(which cpanm)) == 0) {
            $cpanm = 'cpanm';
        }
    }
    die <<'EOF'
cpanm was not found. Please install it before running this script.

There are several ways to install cpanm, try one of these 
On *nix systems:

    apt-get install cpanminus
    curl -L http://cpanmin.us | perl - --sudo App::cpanminus
    cpan App::cpanminus
    
On Windows with powershell, 
    >>>Precondition: mingw must be installed, 
       and dmake.exe avilable on $PATH  <<<
       
    powershell has curl but it does not return text.
    <browser to>http://cpanmin.us then save file as cpanm.txt
    type cpanm.txt | perl - App::cpanminus
    

If it is installed in a non-standard location you can do:
    
    CPANM=/path/to/cpanm perl Build.PL

EOF
        if !$cpanm;
    
    # make sure our cpanm is updated (old ones don't support the ~ syntax)
    system $cpanm, 'App::cpanminus';
    
    # install the Windows-compatible Math::Libm
    if (WIN32 && !eval "use Math::Libm; 1") {
        system $cpanm, 'https://github.com/alexrj/Math-Libm/tarball/master';
    }
    
    my %modules = (%prereqs, %recommends);
    foreach my $module (sort keys %modules) {
        my $version = $modules{$module};
        my @cmd = ($cpanm, "$module~$version");
        if (WIN32) {
			# Windows installs are failing due to test failures, in turn apparently unmaintained windows tests
			# Uncear if the tests are not valid on windows, or if ther are bugs to be fixed on windows.
			# put this back in to force install
			# push( @cmd, '--force');
			if ($module eq 'XML::SAX::ExpatXS' ) {
				my $mingw = $dmake;
				$mingw =~ s/[\/\\]bin[\/\\]dmake.exe[ \n]*//i;
				#for debugging print "Ming is <$mingw>\n";
	                 
				if (!-d $mingw) {
					print "Could not find the MinGW directory at $mingw; skipping XML::SAX::ExpatXS (only needed for faster parsing of AMF files)\n";
				} else {
					push @cmd, sprintf('--configure-args="EXPATLIBPATH=%s\lib EXPATINCPATH=%s\include"', $mingw, $mingw);
				}
			}
        }
        # for debugging print ('Running... '.join(' ', @cmd)."\n");
        my $res = system @cmd;
        if ($res != 0) {
            if (exists $prereqs{$module}) {
                push @missing_prereqs, $module;
            } else {
                printf "Don't worry, this module is optional.\n";
            }
        }
    }
    
    if (!$gui) {
        # clean xs directory before reinstalling, to make sure Build is called
        # with current perl binary
        if (-e './xs/Build') {
			# I believe this statement works for both platforms
			# system 'perl ./xs/Build';
            if (WIN32) {
				# windows calls the same perl script as linux
				# somewhere a functionally identical batch file is generated?
				# a batch file which is now obsolete.
                #system 'cd', 'xs'; -- serves no purpose
                system 'perl ./xs/Build';
                #system 'cd', '..';
            } else {
                system './xs/Build';
            }
        }
        my $res = system $cpanm, '--reinstall', './xs';
        if ($res != 0) {
            die "The XS/C++ code failed to compile, aborting\n";
        }
    }
}

if (@missing_prereqs) {
    printf "The following prerequisites failed to install: %s\n", join(', ', @missing_prereqs);
    exit 1;
} elsif (!$gui) {
    eval "use App::Prove; 1" or die "Failed to load App::Prove";
    my $res = App::Prove->new->run ? 0 : 1;
    if ($res == 0) {
        print "If you also want to use the GUI you can now run `perl Build.PL --gui` to install the required modules.\n";
    } else {
        print "Some tests failed. Please report the failure to the author!\n";
    }
    exit $res;
}

__END__
