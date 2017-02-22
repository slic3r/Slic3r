# Short Powershell script to build a wrapper exec

$perlver = $args[0] 
$perllib = "-lperl$perlver"
Write-Host $perllib

g++ -v -I'C:\strawberry\perl\lib\CORE\' -static-libgcc -static-libstdc++ $perllib -L'C:\strawberry\perl\bin\'  .\shell.cpp -o slic3r.exe | Write-Host
