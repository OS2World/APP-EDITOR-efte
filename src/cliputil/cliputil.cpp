/*    cliputil.cpp
 *
 *    Copyright (c) 2008, eFTE SF Group (see AUTHORS file)
 *    Copyright (c) 1994-1996, Marko Macek
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "clip.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "..\exceptq.h"


#define MAXCLIPTEXT 256 * 1024   /* just for demo */

char buffer[MAXCLIPTEXT];

int main(int argc, char **argv) {
    EXCEPTIONREGISTRATIONRECORD exRegRec;
    ClipData cd;

    LoadExceptq(&exRegRec, "");
    if ((argc == 2) && (strcmp(argv[1], "-s") == 0)) {
        cd.fLen = fread(buffer, 1, MAXCLIPTEXT, stdin);
        cd.fChar = buffer;
        if (PutClipText(&cd) == -1) {
            fprintf(stderr, "Coult not set clipboard text\n");
            UninstallExceptq(&exRegRec);
            return 1;
        }
    } else if (argc == 1) {
        if (GetClipText(&cd) == 0) {
            if ((cd.fLen != 0) && cd.fChar) {
                printf("%s", cd.fChar);
            }
        } else {
            fprintf(stderr, "Could not get clipboard text\n");
            UninstallExceptq(&exRegRec);
            return 1;
        }
    } else {
        fprintf(stderr,
                "Usage: %s {-s}\n"
                "\n"
                "Examples:\n"
                "    cliputil | more\n"
                "    dir | cliputil -s\n",
                argv[0]);
        UninstallExceptq(&exRegRec);
        return 1;
    }
    UninstallExceptq(&exRegRec);
    return 0;
}
