# Short Powershell script to build a wrapper exec
if ($args[0])
{
	$perlver = $args[0]
} else 
{
	$perlver = 524
}

$perllib = "-lperl$perlver"
$shell_loc = "..\common\shell.cpp"

# Build the resource file (used to load icon, etc)
windres slic3r.rc -O coff -o slic3r.res

# Compile an object file that does not have gui forced.
g++ -c -I'C:\strawberry\perl\lib\CORE\' $shell_loc -o slic3r.o

# Compile an object file with --gui automatically passed as an argument
g++ -c -I'C:\strawberry\perl\lib\CORE\' -DFORCE_GUI $shell_loc -o slic3r-gui.o

# Build the EXE for the unforced version as slic3r-console
g++ -static-libgcc -static-libstdc++ -L'C:\strawberry\c\lib' -L'C:\strawberry\perl\bin' -L'C:\strawberry\perl\lib\CORE\' $perllib slic3r.o slic3r.res -o slic3r-console.exe | Write-Host

# Build the EXE for the forced GUI
g++ -static-libgcc -static-libstdc++ -L'C:\strawberry\c\lib' -mwindows -L'C:\strawberry\perl\bin' -L'C:\strawberry\perl\lib\CORE\' $perllib slic3r-gui.o slic3r.res -o slic3r.exe | Write-Host

# Build an extra copy of the GUI version that creates a console window
g++ -static-libgcc -static-libstdc++ -L'C:\strawberry\c\lib' -L'C:\strawberry\perl\bin' -L'C:\strawberry\perl\lib\CORE\' $perllib slic3r-gui.o slic3r.res -o slic3r-debug-console.exe | Write-Host

