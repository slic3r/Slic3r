# Written by Joseph Lenox
# Licensed under the same license as the rest of Slic3r.
# ------------------------
# You need to have Strawberry Perl 5.22 installed for this to work, 
echo "Make this is run from the perl command window." 
echo "Requires PAR."

# Change this to where you have Strawberry Perl installed.
#SET STRAWBERRY_PATH=C:\Strawberry
$STRAWBERRY_PATH=C:\Strawberry
# ([io.fileinfo](Get-Command "perl.exe").Path).basename

cpanm "PAR::Packer"

pp -a $STRAWBERRY_PATH\perl\bin\perl5.22.1.exe;perl5.22.1.exe -a $STRAWBERRY_PATH\perl\bin\freeglut.dll;freeglut.dll -a $STRAWBERRY_PATH\perl\bin\perl522.dll;perl522.dll -a $STRAWBERRY_PATH\perl\bin\libgcc_s_sjlj-1.dll;libgcc_s_sjlj-1.dll -a $STRAWBERRY_PATH\perl\bin\libstdc++-6.dll;libstdc++-6.dll -a $STRAWBERRY_PATH\perl\bin\libwinpthread-1.dll;libwinpthread-1.dll -a $STRAWBERRY_PATH\perl\site\lib\Alien\wxWidgets\msw_3_0_2_uni_gcc_3_4\lib\wxbase30u_gcc_custom.dll -a $STRAWBERRY_PATH\perl\site\lib\Alien\wxWidgets\msw_3_0_2_uni_gcc_3_4\lib\wxmsw30u_adv_gcc_custom.dll -a $STRAWBERRY_PATH\perl\site\lib\Alien\wxWidgets\msw_3_0_2_uni_gcc_3_4\lib\wxmsw30u_core_gcc_custom.dll -a $STRAWBERRY_PATH\perl\site\lib\Alien\wxWidgets\msw_3_0_2_uni_gcc_3_4\lib\wxmsw30u_gl_gcc_custom.dll -a $STRAWBERRY_PATH\perl\site\lib\Alien\wxWidgets\msw_3_0_2_uni_gcc_3_4\lib\wxmsw30u_html_gcc_custom.dll -a ..\utils;script\utils -a var;script\var -a autorun.bat;slic3r.bat -M Encode::Locale -M Moo -M Thread::Semaphore -M OpenGL -M Slic3r::XS -M Unicode::Normalize -M Wx -M Class::Accessor -M Wx::DND -M Wx::Grid -M Wx::Print -M Wx::Html -M Wx::GLCanvas -M Math::Trig -M threads -M threads::shared -M Thread::Queue -e -p -x slic3r.pl -o ..\slic3r.par

cp ..\slic3r.par ..\slic3r-$(git rev-parse --abbrev-ref HEAD)-$(git rev-parse --short HEAD).zip
