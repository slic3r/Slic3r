#include <EXTERN.h> // from the Perl distribution
#include <perl.h> // from the Perl distribution

// Perl win32 specific includes, found in perl\\lib\\CORE\\win32.h
// Defines the windows specific convenience RunPerl() function,
// which is not available on other operating systems.
#include <win32.h>
// the standard Windows. include
//#include <Windows.h>
#include <cstdio>
#include <cstdlib>
#include <wchar.h>

int main(int argc, char **argv, char **env)
{
	
	// replaces following Windows batch file: @"%~dp0\perl5.24.0.exe"
	// "%~dp0\slic3r.pl" --DataDir "C:\Users\Public\Documents\Prusa3D\Slic3r
	// settings MK2"%*

	// If the Slic3r is installed in a localized directory (containing non-iso
	// characters), spaces or semicolons, use short file names.

	char    exe_path[MAX_PATH] = {0};
	char    script_path[MAX_PATH];
	char**  command_line = (char**)malloc(sizeof(char*) * ((++ argc) + 1));
	{
		// Unicode path. This will not be used directly, but to test, whether
		// there are any non-ISO characters, in which case the path is converted to a
		// short path (using 8.3 directory names).

		wchar_t exe_path_w[MAX_PATH] = {0};
		char    drive[_MAX_DRIVE];
		char    dir[_MAX_DIR];
		char    fname[_MAX_FNAME];
		char    ext[_MAX_EXT];
		bool    needs_short_paths = false;
		int     len;
		int     i;
		GetModuleFileNameA(NULL, exe_path, MAX_PATH-1);
		GetModuleFileNameW(NULL, exe_path_w, MAX_PATH-1);
		len = strlen(exe_path);

		if (len != wcslen(exe_path_w)) {
			needs_short_paths = true;
		} else {
			for (i = 0; i < len; ++ i)
				if ((wchar_t)exe_path[i] != exe_path_w[i] || exe_path[i] == ' ' ||
						exe_path[i] == ';') {
					needs_short_paths = true;
					break;
				}
		}
		if (needs_short_paths) {
			wchar_t exe_path_short[MAX_PATH] = {0};
			GetShortPathNameW(exe_path_w, exe_path_short, MAX_PATH);
			len = wcslen(exe_path_short);
			for (i = 0; i <= len; ++ i)
				exe_path[i] = (char)exe_path_short[i];
		}
		_splitpath(exe_path, drive, dir, fname, ext);
		_makepath(script_path, drive, dir, NULL, NULL);
		if (needs_short_paths)
			printf("Slic3r installed in a loclized path. Using an 8.3 path: \"%s\"\n",
					script_path);
		SetDllDirectoryA(script_path);
		_makepath(script_path, drive, dir, "slic3r", "pl");
		command_line[0] = exe_path;
		command_line[1] = script_path;
		memcpy(command_line + 2, argv + 1, sizeof(char*) * (argc - 2));
		command_line[argc] = NULL;
		// Unset the PERL5LIB and PERLLIB environment variables.
		SetEnvironmentVariable("PERL5LIB", NULL);
		SetEnvironmentVariable("PERLLIB", NULL);
#if 0
		printf("Arguments: \r\n");
		for (size_t i = 0; i < argc + 1; ++ i)
			printf(" %d: %s\r\n", i, command_line[i]);
#endif
	}
	RunPerl(argc, command_line, NULL);
	free(command_line);
}

