/*
 *  Copyright (C) 1999 AT&T Laboratories Cambridge.  All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

/*
 * argsresources.c - deal with command-line args and resources.
 */
static const char *ID = "$Id$";

#include "vncsnapshot.h"
#include "version.h"

static int setNumber(int *argc, char ***argv, void *arg, int value);
static int setString(int *argc, char ***argv, void *arg, int value);
static int setFlag(int *argc, char ***argv, void *arg, int value);

static char * rect = NULL;

typedef struct {
    const char *optionstring;
    int (*set)(int *argc, char ***argv, void *arg, int value);
    void *arg;
    int   value;
} Options;


/* Options - excluding -listen, -tunnel, and -via */
Options cmdLineOptions[] = {
  {"-passwd",        setString, &appData.passwordFile, 0},
  {"-encodings",     setString, &appData.encodingsString, 0},
  {"-compresslevel", setNumber, &appData.compressLevel, 0},
  {"-vncQuality",    setNumber, &appData.qualityLevel, 0},
  {"-quality",       setNumber, &appData.saveQuality, 0},
  {"-debug",         setFlag,   &appData.debug, 0},
  {"-quiet",         setFlag,   &appData.quiet, 1},
  {"-verbose",       setFlag,   &appData.quiet, 0},
  {"-rect",          setString, &rect, 0},
    /* soft cursor doesn't include cursor location. Sigh. */
/*  {"-cursor",        setFlag, &appData.useRemoteCursor, 1},
  {"-nocursor",      setFlag, &appData.useRemoteCursor, 0},*/
  {NULL, NULL, NULL, 0}
};



/*
 * vncServerHost and vncServerPort are set either from the command line or
 * from a dialog box.
 */

char vncServerHost[256];
int vncServerPort = 0;
char *vncServerName;


/*
 * appData is our application-specific data which can be set by the user with
 * application resource specs.  The AppData structure is defined in the header
 * file.
 */

AppData appData = {
    NULL,   /* encodingsString */
    NULL,   /* passwordFile */
    0,      /* debug */
    4,      /* compressLevel */
    9,      /* qualityLevel */
    0,      /* useRemoteCursor [not useful] */
    100,    /* saveQuality */
    NULL,   /* outputFilename */
    0,      /* quiet */
    0, 0,   /* rect width, height */
    0, 0,   /* rect x, y */
    };


/*
 * removeArgs() is used to remove some of command line arguments.
 */

void
removeArgs(int *argc, char** argv, int idx, int nargs)
{
  int i;
  if ((idx+nargs) > *argc) return;
  for (i = idx+nargs; i < *argc; i++) {
    argv[i-nargs] = argv[i];
  }
  *argc -= nargs;
}

/*
 * usage() prints out the usage message.
 */

void
usage(void)
{
  fprintf(stderr,
	  "TightVNC snapshot version " VNC_SNAPSHOT_VERSION " (based on TightVNC 1.2.2 and RealVNC 3.3.6)\n"
	  "\n"
	  "Usage: %s [<OPTIONS>] [<HOST>]:<DISPLAY#> filename\n"
	  "       %s [<OPTIONS>] -listen [<DISPLAY#>] filename\n"
	  "       %s [<OPTIONS>] -tunnel <HOST>:<DISPLAY#> filename\n"
	  "       %s [<OPTIONS>] -via <GATEWAY> [<HOST>]:<DISPLAY#> filename\n"
	  "\n"
	  "<OPTIONS> are:\n"
	  "        -passwd <PASSWD-FILENAME>\n"
	  "        -encodings <ENCODING-LIST> (e.g. \"tight copyrect\")\n"
	  "        -compresslevel <COMPRESS-VALUE> (0..9: 0-fast, 9-best)\n"
	  "        -vncQuality <JPEG-QUALITY-VALUE> (0..9: 0-low, 9-high)\n"
          "        -quality <JPEG-QUALITY-VALUE> (0..100: 0-low, 100-high)\n"
          "        -quiet\n"
          "        -verbose\n"
          "        -rect wxh+x+y\n"
/*	  "        -cursor\n"
	  "        -nocursor\n"*/
	  "\n", programName, programName, programName, programName);
  exit(1);
}


/*
 * GetArgsAndResources() deals with resources and any command-line arguments
 * not already processed by XtVaAppInitialize().  It sets vncServerHost and
 * vncServerPort and all the fields in appData.
 */

void
GetArgsAndResources(int argc, char **argv)
{
  int i;
  int   argsleft;
  char **arg;
  int processed;

  argsleft = argc;
  arg = argv+1;

    /* Must have at least one argument */
  if (argc < 2) {
      usage();
  }

  do {
      processed = 0;
      for (i = 0; cmdLineOptions[i].optionstring != NULL; i++) {
          if (strcmp(cmdLineOptions[i].optionstring, arg[0]) == 0) {
              processed = cmdLineOptions[i].set(&argsleft, &arg, cmdLineOptions[i].arg, cmdLineOptions[i].value);
              argsleft--;
              arg++;
          }
      }
  } while (processed);

    /* Parse rectangle provided. */
    if (rect != NULL) {
        /* We could use sscanf, but the return value is not consistent
         * across all platforms.
         */
        long w, h;
        long x, y;
        char *end = NULL;

        w = strtol(rect, &end, 10);
        if (end == NULL || end == rect || *end != 'x') {
            fprintf(stderr, "%s: invalid rectangle specification %s\n",
                    programName, rect);
            usage();
        }
        end++;
        h = strtol(end, &end, 10);
        if (end == NULL || end == rect || (*end != '+' && *end != '-')) {
            fprintf(stderr, "%s: invalid rectangle specification %s\n",
                    programName, rect);
            usage();
        }
        /* determine sign */
        appData.rectXNegative = *end == '-';
        end++;
        x = strtol(end, &end, 10);
        if (end == NULL || end == rect || (*end != '+' && *end != '-')) {
            fprintf(stderr, "%s: invalid rectangle specification %s\n",
                    programName, rect);
            usage();
        }
        /* determine sign */
        appData.rectYNegative = *end == '-';
        end++;
        y = strtol(end, &end, 10);
        if (end == NULL || end == rect || *end != '\0') {
            fprintf(stderr, "%s: invalid rectangle specification %s\n",
                    programName, rect);
            usage();
        }

        appData.rectWidth = w;
        appData.rectHeight = h;
        appData.rectX = x;
        appData.rectY = y;
    }

    argc = argsleft;
    argv = arg;

  /* Check any remaining command-line arguments.  If -listen was specified
     there should be none.  Otherwise the only argument should be the VNC
     server name.  If not given then pop up a dialog box and wait for the
     server name to be entered. */

  if (listenSpecified) {
    if (argc != 2) {
      fprintf(stderr,"\n%s -listen: invalid command line argument: %s\n",
	      programName, argv[0]);
      usage();
    }
    appData.outputFilename = argv[0];
    return;
  }

  if (argc != 3) {
    usage();
  } else {
    vncServerName = argv[0];
    appData.outputFilename = argv[1];

    if (vncServerName[0] == '-')
      usage();
  }

  if (strlen(vncServerName) > 255) {
    fprintf(stderr,"VNC server name too long\n");
    exit(1);
  }

  for (i = 0; vncServerName[i] != ':' && vncServerName[i] != 0; i++);

  strncpy(vncServerHost, vncServerName, i);

  if (vncServerName[i] == ':') {
    vncServerPort = atoi(&vncServerName[i+1]);
  } else {
    vncServerPort = 0;
  }

  if (vncServerPort < 100)
    vncServerPort += SERVER_PORT_OFFSET;
}

static int setNumber(int *argc, char ***argv, void *arg, int value)
{
    long number;
    char *end;
    int ok;

    ok = 0;
    if (*argc > 2) {
        (*argc) --;
        (*argv)++;
        end = NULL;

        number = strtol((*argv)[0], &end, 0);
        if (end != NULL && *end == '\0') {
            *((int *) arg) = number;
            ok = 1;
        }
    }

    return ok;
}

static int setString(int *argc, char ***argv, void *arg, int value)
{
    int ok;

    ok = 0;
    if (*argc > 2) {
        (*argc) --;
        (*argv)++;
        *((char **) arg) = (*argv)[0];
        ok = 1;
    }
    return ok;
}

static int setFlag(int *argc, char ***argv, void *arg, int value)
{
    *((Bool *)arg) = value;
    return 1;
}
