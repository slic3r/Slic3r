# Short Powershell script to build a wrapper exec
Param
(
    [string]$perlVersion = "524",
    [string]$STRAWBERRY_PATH = "C:\Strawberry",
    # Path to C++ compiler, or just name if it is in path
    [string]$cxx = "g++"
)

function Get-ScriptDirectory
{
  $Invocation = (Get-Variable MyInvocation -Scope 1).Value
  Split-Path $Invocation.MyCommand.Path
}
$scriptDir = Get-ScriptDirectory

$perllib = "-lperl$perlVersion"
$shell_loc = "${scriptDir}\..\common\shell.cpp"

# Build the resource file (used to load icon, etc)
windres ${scriptDir}\slic3r.rc -O coff -o ${scriptDir}\slic3r.res

# Compile an object file that does not have gui forced.
Invoke-Expression "$cxx -c -I'${STRAWBERRY_PATH}\perl\lib\CORE\' $shell_loc -o ${scriptDir}/slic3r.o"


# Compile an object file with --gui automatically passed as an argument
Invoke-Expression "$cxx -c -I'${STRAWBERRY_PATH}\perl\lib\CORE\' -DFORCE_GUI $shell_loc -o ${scriptDir}/slic3r-gui.o"

# Build the EXE for the unforced version as slic3r-console
Invoke-Expression "$cxx -static-libgcc -static-libstdc++ -L'${STRAWBERRY_PATH}\c\lib' -L'${STRAWBERRY_PATH}\perl\bin' -L'${STRAWBERRY_PATH}\perl\lib\CORE\' $perllib ${scriptDir}/slic3r.o ${scriptDir}/slic3r.res -o ${scriptDir}/slic3r-console.exe | Write-Host"

# Build the EXE for the forced GUI
Invoke-Expression "$cxx -static-libgcc -static-libstdc++ -L'${STRAWBERRY_PATH}\c\lib' -mwindows -L'${STRAWBERRY_PATH}\perl\bin' -L'${STRAWBERRY_PATH}\perl\lib\CORE\' $perllib ${scriptDir}/slic3r-gui.o ${scriptDir}/slic3r.res -o ${scriptDir}/slic3r.exe | Write-Host"

# Build an extra copy of the GUI version that creates a console window
Invoke-Expression "$cxx -static-libgcc -static-libstdc++ -L'${STRAWBERRY_PATH}\c\lib' -L'${STRAWBERRY_PATH}\perl\bin' -L'${STRAWBERRY_PATH}\perl\lib\CORE\' $perllib ${scriptDir}/slic3r-gui.o ${scriptDir}/slic3r.res -o ${scriptDir}/slic3r-debug-console.exe | Write-Host"

