# Short Powershell script to build a wrapper exec

$perlver = $args[0] 
$perllib = "-lperl$perlver"
Write-Host $perllib

g++ -I'C:\strawberry\perl\lib\CORE\' $perllib -L'C:\strawberry\perl\bin\'  .\shell.cpp -o slic3r.exe
