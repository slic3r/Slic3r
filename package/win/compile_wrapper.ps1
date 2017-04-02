# Short Powershell script to build a wrapper exec
if ($args[0])
{
	$perlver = $args[0]
} else 
{
	$perlver = 518
}

$perllib = "-lperl$perlver"
$shell_loc = "..\common\shell.cpp"

windres slic3r.rc -O coff -o slic3r.res
g++ -c -I'C:\strawberry\perl\lib\CORE\' $shell_loc -o slic3r.o
g++ -c -I'C:\strawberry\perl\lib\CORE\' -DFORCE_GUI $shell_loc -o slic3r-gui.o
g++ -static-libgcc -static-libstdc++ -L'C:\strawberry\c\lib' -L'C:\strawberry\perl\bin' -L'C:\strawberry\perl\lib\CORE\' $perllib slic3r.o slic3r.res -o slic3r-console.exe | Write-Host
g++ -static-libgcc -static-libstdc++ -L'C:\strawberry\c\lib' -mwindows -L'C:\strawberry\perl\bin' -L'C:\strawberry\perl\lib\CORE\' $perllib slic3r-gui.o slic3r.res -o slic3r.exe | Write-Host
g++ -static-libgcc -static-libstdc++ -L'C:\strawberry\c\lib' -L'C:\strawberry\perl\bin' -L'C:\strawberry\perl\lib\CORE\' $perllib slic3r-gui.o slic3r.res -o slic3r-debug-console.exe | Write-Host

