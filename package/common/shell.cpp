#include <EXTERN.h> // from the Perl distribution
#include <perl.h> // from the Perl distribution

#ifdef WIN32
// Perl win32 specific includes, found in perl\\lib\\CORE\\win32.h
// Defines the windows specific convenience RunPerl() function,
// which is not available on other operating systems.
#include <win32.h>
#include <wchar.h>
#endif

#include <cstdio>
#include <cstdlib>

#ifdef WIN32
int main(int argc, char **argv, char **env)
{
	
	// If the Slic3r is installed in a localized directory (containing non-iso
	// characters), spaces or semicolons, use short file names.
	char    exe_path[MAX_PATH] = {0};
	char    script_path[MAX_PATH];
	char    gui_flag[6] = {"--gui"};
#ifdef FORCE_GUI
	char**  command_line = (char**)malloc(sizeof(char*) * ((++ argc) + 2));
#else
	char**  command_line = (char**)malloc(sizeof(char*) * ((++ argc) + 1));
#endif
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
#ifdef FORCE_GUI
		command_line[argc] = gui_flag;
		command_line[argc+1] = NULL;
#else
		command_line[argc] = NULL;
#endif
		// Unset the PERL5LIB and PERLLIB environment variables.
		SetEnvironmentVariable("PERL5LIB", NULL);
		SetEnvironmentVariable("PERLLIB", NULL);
#if 0
		printf("Arguments: \r\n");
		for (size_t i = 0; i < argc + 1; ++ i)
			printf(" %d: %s\r\n", i, command_line[i]);
#endif
	}
#ifdef FORCE_GUI
	RunPerl(argc+1, command_line, NULL);
#else
	RunPerl(argc, command_line, NULL);
#endif
	free(command_line);
}
#else

int main(int argc, char **argv, char **env)
{
    PerlInterpreter *my_perl = perl_alloc();
    if (my_perl == NULL) {
        fprintf(stderr, "Cannot start perl interpreter. Exiting.\n");
        return -1;
    }
    perl_construct(my_perl);

#ifdef FORCE_GUI
    char* command_line[] = { "slic3r", "slic3r.pl", "--gui" };
#else
    char* command_line[] = { "slic3r", "slic3r.pl" };
#endif
    perl_parse(my_perl, NULL, 3, command_line, (char **)NULL);
    perl_run(my_perl);
    perl_destruct(my_perl);
    perl_free(my_perl);
}
#endif
