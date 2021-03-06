/*    fte.cpp
 *
 *    Copyright (c) 2008, eFTE SF Group (see AUTHORS file)
 *    Copyright (c) 1994-1997, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */
#include "fte.h"
#include "log.h"
#include "c_history.h"
#ifdef USE_LOCALE
#include <locale.h>
#endif
#if defined(OS2)
#define INCL_DOS
#include <os2.h>
#define INCL_LOADEXCEPTQ
#include "exceptq.h"
#endif
#if defined(UNIX)
// variables used by vfte
uid_t effuid;
gid_t effgid;
#endif /* UNIX */
//#define  __PMPRINTF__
//#include "PMPRINTF.H"

char ConfigFileName[MAXPATH] = "";

static void Version()
{
    DieError(4, "eFTE/2 version " VERSION " " COPYRIGHT "\n");
}

static void Usage()
{
    DieError(4, "Usage: " PROGRAM " [-?] [-h] [--help] [-CDHTmlrt] files...\n"
             "Version: " VERSION " " COPYRIGHT "\n"
             "   You may distribute under the terms of either the GNU General Public\n"
             "   License or the Artistic License, as specified in the README file.\n"
             "\n"
             "Options:\n"
             "  --                End of options, only files remain.\n"
             "  -+                Next option is file.\n"
             "  -? -h --help      Display usage.\n"
             "  --debug<clean>    Write to efte.log append if not clean \n"
             "  --version         Display eFTE/2 version.\n"
             "  -v                Increase verbosity level.\n"
             "  -!                Ignore config file, use builtin defaults (also -c).\n"
             "  -C[<.fte>]        Use specified configuration file must be full path to file (no arg=builtin).\n"
             "  -d<defineterm>    Defines a term for use in \%if() statements in config files \n"
             "  -D[<.dsk>]        Load/Save desktop from <.dsk> file (no arg=disable desktop).\n"
             "  -H[<.his>]        Load/Save history from <.his> file (no arg=disable history).\n"
             "  -m[<mode>]        Override mode for remaining files (no arg=no override).\n"
             "  -l<line>[,<col>]  Go to line (and column) in next file.\n"
             "  -r                Open next file as read-only.\n"
             "  -T[<tagfile>]     Load tags file at startup.\n"
             "  -t<tag>           Locate specified tag.\n"
             "  -L<Lang>          Choose the language where lang is the 2 letter lanuage code or \n \t \tnothing to default to english"
//           "       -p        Load files into already running eFTE.\n"
	);
}

#if !defined(OS2) && !defined(UNIX)
/*
 * findPathExt() returns a ^ to the suffix in a file name string. If the
 * name contains a suffix, the pointer ^ts to the suffix' dot character,
 * if the name has no suffix the pointer points to the NUL terminator of
 * the file name string.
 * .lib: CBASE.LIB
 */
static char *findPathExt(char *filename)
{
    char *p, *sps;

    for (p = filename, sps = NULL; *p; p++) {
	if (ISSLASH(*p))
	    sps = NULL;
	if (*p == '.')
	    sps = p;
    }
    if (sps == NULL)
	sps = p;
    return sps;
}
#endif

#if defined(NT) && defined(MSVC) && !defined(__WATCOMC__)
char *getProgramName(char *name)
{
    return _pgmptr;
}
#endif

#if defined(OS2) && defined(__EMX__)

// argv[0] on emx does not contain full path

char *getProgramName(char *name)
{
    char ProgramName[MAXPATH];
    PTIB tib;
    PPIB pib;

    DosGetInfoBlocks(&tib, &pib);
    if (DosQueryModuleName(pib->pib_hmte, sizeof(ProgramName), ProgramName) !=
	0)
	return name;
    return strdup(ProgramName);
}

#endif

static int CmdLoadConfiguration(int &argc, char **argv)
{
    int ign = 0;
    int QuoteAll = 0, QuoteNext = 0;
    int haveConfig = 0;
    int Arg;

    for (Arg = 1; Arg < argc; Arg++) {
	if (!QuoteAll && !QuoteNext && (argv[Arg][0] == '-')) {
	    if (argv[Arg][1] == '-') {
		if (strcmp(argv[Arg], "--help") == 0) {
		    Usage();
		    return 0;
		}
		else if (strcmp(argv[Arg], "--version") == 0) {
		    Version();
		    return 0;
		}

		int debug_clean = strcmp(argv[Arg], "--debugclean") == 0;

		if (debug_clean || strcmp(argv[Arg], "--debug") == 0) {
#ifndef FTE_NO_LOGGING
		    char path[MAXPATH];

#ifdef UNIX
		    ExpandPath("~/.efte", path, sizeof(path));
#else
		    JustDirectory(argv[0], path, sizeof(path));
#endif
		    Slash(path, 1);
		    strlcat(path, "efte.log", sizeof(path));
		    if (debug_clean)
			unlink(path);

		    globalLog.SetLogFile(path);
		    printf("Trace Log in: %s\n", path);
#else
		    printf("--debug, --debugclean disabled\n");
#endif
		}
		else
		    QuoteAll = 1;
	    }
            else if (argv[Arg][1] == '!') {
                haveConfig = 2;
                strcpy(ConfigFileName, "edefault.fte");
	    }
	    else if (argv[Arg][1] == '+') {
		QuoteNext = 1;
	    }
	    else if (argv[Arg][1] == 'v') {
		verbosity++;
	    }
	    else if (argv[Arg][1] == '?' || argv[Arg][1] == 'h') {
		Usage();
		return 0;
	    }
	    else if (argv[Arg][1] == 'd') {
		DefineWord(argv[Arg] + 2);
            }
            else if (argv[Arg][1] == 'L') {
                translate = -1;
                if (argv[Arg][2] && argv[Arg][3] && strnicmp("en", &argv[Arg][2], 2))
                    snprintf(menupath, MAXPATH, "menu_%c%c", argv[Arg][2], argv[Arg][3]);
	    }
	    else if (argv[Arg][1] == 'c' || argv[Arg][1] == 'C') {
		if (argv[Arg][2]) {
		    ExpandPath(argv[Arg] + 2, ConfigFileName,
			       sizeof(ConfigFileName));
		    haveConfig = 1;
		}
                else {
                    haveConfig = 2;
                    strcpy(ConfigFileName, "edefault.fte");
                }
	    }
	}
    }

    if (haveConfig == 1) {
	if (access(ConfigFileName, 0) != 0) {
	    DieError(1, "Could not access configuration file '%s'.\n"
		     "Does it exist? Have you provided the full pathname?", ConfigFileName);
	}
    }
    else if (haveConfig != 2)
        strcpy(ConfigFileName, "mymain.fte");
#if 0
    if (translate != -1) {
        char *lang;

        lang = getenv("LANG");
        if (lang) {
            if (!strnicmp(lang, "es", 2)) {      //Spanish
                translate = 1;
                strcpy(menupath, "menu_es");
            }
            else if (!strnicmp(lang, "de", 2)) { // German
                translate = 1;
                strcpy(menupath, "menu_de");
            }
            else if (!strnicmp(lang, "cs", 2)) { // Czech
                translate = 1;
                strcpy(menupath, "menu_cs");
            }
            else if (!strnicmp(lang, "nl", 2)) { // Dutch
                translate = 1;
                strcpy(menupath, "menu_nl");
            }
            else if (!strnicmp(lang, "fr", 2)) { // French
                translate = 1;
                strcpy(menupath, "menu_fr");
            }
            else if (!strnicmp(lang, "it", 2)) { // Itailian
                translate = 1;
                strcpy(menupath, "menu_it");
            }
            else if (!strnicmp(lang, "no", 2)) { // Norwegian
                translate = 1;
                strcpy(menupath, "menu_no");
            }
            else if (!strnicmp(lang, "ru", 2)) { // Russian
                translate = 1;
                strcpy(menupath, "menu_ru");
            }
            else if (!strnicmp(lang, "sv", 2)) { //  Swedish
                translate = 1;
                strcpy(menupath, "menu_sv");
            }
            else
                translate = 0;
        } // if lang
    } // if tanslate
#endif
    LoadConfig(argc, argv, ConfigFileName);
    for (Arg = 1; Arg < argc; Arg++) {
	if (!QuoteAll && !QuoteNext && (argv[Arg][0] == '-')) {
	    if (argv[Arg][1] == '-' && argv[Arg][2] == '\0') {
		QuoteAll = 1;
	    }
	    else if (argv[Arg][1] == '+') {
		QuoteNext = 1;
	    }
	    else if (argv[Arg][1] == 'D') {
		ExpandPath(argv[Arg] + 2, DesktopFileName,
			   (strstr(strupr(argv[Arg]), ".DSK") + 5) - argv[Arg]);
		if (IsDirectory(DesktopFileName)) {
		    Slash(DesktopFileName, 1);
		    strlcat(DesktopFileName, DESKTOP_NAME,
			    sizeof(DesktopFileName));
		}
		if (DesktopFileName[0] == 0) {
		    LoadDesktopOnEntry = 0;
		    SaveDesktopOnExit = 0;
		}
		else {
		    LoadDesktopOnEntry = 1;
		}
	    }
	    else if (argv[Arg][1] == 'H') {
		strlcpy(HistoryFileName, argv[Arg] + 2,
			(strstr(strupr(argv[Arg]), ".HIS") + 5) - argv[Arg]);
		if (HistoryFileName[0] == 0) {
		    KeepHistory = 0;
		}
		else {
		    KeepHistory = 1;
		}
	    }
	}
	else {
	    if (LoadDesktopOnEntry == 2) {
		LoadDesktopOnEntry = 0;
		SaveDesktopOnExit = 0;
		DesktopFileName[0] = 0;
	    }
	}
    }
    if (LoadDesktopOnEntry == 2)
	LoadDesktopOnEntry = 1;
    return 1;
}

int main(int argc, char **argv)
{
#if defined(OS2)
    EXCEPTIONREGISTRATIONRECORD exRegRec;

    LoadExceptq(&exRegRec, "I", "eFTE/2 ver "VERSION);
#endif
#if defined(_DEBUG) && defined(MSVC) && defined(MSVCDEBUG)
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif //_DEBUG && MSVC && MSVCDEBUG

#if defined(__EMX__) || (defined(NT) && defined(MSVC) && !defined(__WATCOMC__))
    argv[0] = getProgramName(argv[0]);
#endif

#if defined(UNIX) && defined(LINUX)
    // security fix - when we need to be suid to access vcsa
    effuid = geteuid();
    effgid = getegid();

    if (getuid() != effuid)
	seteuid(getuid());
    if (getgid() != effgid)
	setegid(getgid());
#endif
#ifdef USE_LOCALE
    // setup locale from environment
    setlocale(LC_ALL, "");
#endif

    if (CmdLoadConfiguration(argc, argv) == 0)
	return 1;

    STARTFUNC("main");
    EGUI *g = new EGUI(argc, argv, ScreenSizeX, ScreenSizeY);

    if (gui == 0 || g == 0)
	DieError(1, "Failed to initialize display\n");

    gui->Run();

#if defined(OS2) && !defined(DBMALLOC) && defined(CHECKHEAP)
    if (_heapchk() != _HEAPOK)
	DieError(0, "Heap memory is corrupt.");
#endif

    delete gui;

    gui = 0;

#if defined(__EMX__)
    free(argv[0]);
#endif

#if defined(OS2) && !defined(DBMALLOC) && defined(CHECKHEAP)
    if (_heapchk() != _HEAPOK)
	DieError(0, "Heap memory is corrupt.");
#endif

#if defined(_DEBUG) && defined(MSVC) && defined(MSVCDEBUG)
    _CrtSetDbgFlag((_CRTDBG_LEAK_CHECK_DF) |
		   _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG));
#endif //_DEBUG && MSVC && MSVCDEBUG

#if defined(__DEBUG_ALLOC__)
    _dump_allocated(64);
#endif
#if defined(OS2)
    UninstallExceptq(&exRegRec);
#endif
    ENDFUNCRC(0);
    //return 0;
}
