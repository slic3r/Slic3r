# Short Powershell script to build a wrapper exec
if ($args[0])
{
	$perlver = $args[0]
} else 
{
	$perlver = 518
}

$perllib = "-lperl$perlver"
Write-Host $perllib

g++ -v -I'C:\strawberry\perl\lib\CORE\' -static-libgcc -static-libstdc++ -L'C:\strawberry\c\lib' -L'C:\strawberry\perl\bin' -L'C:\strawberry\perl\lib\CORE\' $perllib .\shell.cpp -o slic3r.exe | Write-Host

