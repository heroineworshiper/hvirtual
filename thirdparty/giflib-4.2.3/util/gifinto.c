/*****************************************************************************
*   "Gif-Lib" - Yet another gif library.
*
* Written by:  Gershon Elber				Ver 0.1, Jul. 1989
******************************************************************************
* Program to read stdin, and save it into the specified file iff the
* result is long enough to be a valid GIF and inspired by the rle utah
* tool kit I decided to implement and add it.
* Options:
* -v : verbose mode.
* -s minsize : the minimum file size to keep it.
* -h : on-line help.
******************************************************************************
* History:
* 7 Jul 89 - Version 1.0 by Gershon Elber.
* 22 Dec 89 - Fix problem with tmpnam (Version 1.1).
*****************************************************************************/

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

#ifdef __MSDOS__
#include <io.h>
#endif /* __MSDOS__ */

#include "gif_lib.h"
#include "getarg.h"

#define PROGRAM_NAME	"gifinto"

#define DEFAULT_MIN_FILE_SIZE	14     /* More than GIF stamp + screen desc. */
#define	DEFAULT_OUT_NAME	"GifInto.Gif"
#define DEFAULT_TMP_NAME	"TempInto.$$$"

static char
    *VersionStr =
	PROGRAM_NAME
	VERSION_COOKIE
	"	Gershon Elber,	"
	__DATE__ ",   " __TIME__ "\n"
	"(C) Copyright 1989 Gershon Elber.\n";
static char
    *CtrlStr =
	PROGRAM_NAME
	" v%- s%-MinFileSize!d h%- GifFile!*s";

static int
    MinFileSize = DEFAULT_MIN_FILE_SIZE;

/******************************************************************************
* The is simply: read until EOF, then close the output, test its length, and
* if non zero then rename it.
******************************************************************************/
int main(int argc, char **argv)
{
    int	NumFiles;
    bool Error, MinSizeFlag = false, HelpFlag = false;
    char **FileName = NULL,
        TmpName[80], FoutTmpName[80], FullPath[80], DefaultName[80], s[80], *p;
    FILE *Fin, *Fout;

    if ((Error = GAGetArgs(argc, argv, CtrlStr, &GifNoisyPrint,
		&MinSizeFlag, &MinFileSize, &HelpFlag,
		&NumFiles, &FileName)) != false ||
		(NumFiles > 1 && !HelpFlag)) {
	if (Error)
	    GAPrintErrMsg(Error);
	else if (NumFiles != 1)
	    GIF_MESSAGE("Error in command line parsing - one GIF file please.");
	GAPrintHowTo(CtrlStr);
	exit(EXIT_FAILURE);
    }

    if (HelpFlag) {
	(void)fprintf(stderr, VersionStr, GIFLIB_MAJOR, GIFLIB_MINOR);
	GAPrintHowTo(CtrlStr);
	exit(EXIT_SUCCESS);
    }

    /* Open the stdin in binary mode and increase its buffer size: */
#ifdef __MSDOS__
    setmode(0, O_BINARY);		  /* Make sure it is in binary mode. */
#endif

    Fin = fdopen(0, "rb");   /* Make it into a stream: */

    if (Fin == NULL)
    {
        GIF_EXIT("Failed to open input.");
    }

    if (setvbuf(Fin, NULL, _IOFBF, GIF_FILE_BUFFER_SIZE)) /* Incr. stream buf.*/
    {
        GIF_EXIT("Failed to set buffering on input.");
    }

    /* Isolate the directory where our destination is, and set tmp file name */
    /* in the very same directory. This code is isecure because it creates   */
    /* predictable names, but it's not worth the effort and risk to fix.     */
    strncpy(FullPath, *FileName, sizeof(FullPath)-1);
    if ((p = strrchr(FullPath, '/')) != NULL ||
	(p = strrchr(FullPath, '\\')) != NULL)
	p[1] = 0;
    else if ((p = strrchr(FullPath, ':')) != NULL)
	p[1] = 0;
    else
	FullPath[0] = 0;		  /* No directory or disk specified. */

    strcpy(FoutTmpName, FullPath);   /* Generate destination temporary name. */
    /* Make sure the temporary is made in the current directory: */
    /* coverity[secure_temp] */
    p = tmpnam(TmpName);
    if (strrchr(p, '/')) p = strrchr(p, '/') + 1;
    if (strrchr(p, '\\')) p = strrchr(p, '\\') + 1;
    if (strlen(p) == 0) p = DEFAULT_TMP_NAME;
    strcat(FoutTmpName, p);

    Fout = fopen(FoutTmpName, "wb");
    if (Fout == NULL)
    {
	GIF_EXIT("Failed to open output.");
    }

    if (setvbuf(Fout, NULL, _IOFBF, GIF_FILE_BUFFER_SIZE)) /*Incr. stream buf.*/
    {
	GIF_EXIT("Failed to set buffering on output.");
    }

    while (!feof(Fin)) {
	if (putc(getc(Fin), Fout) == EOF)
	    GIF_EXIT("Failed to write output.");
    }

    fclose(Fin);
    if (ftell(Fout) >= (long) MinFileSize) {
	fclose(Fout);
	unlink(*FileName);
	if (rename(FoutTmpName, *FileName) != 0) {
	    strcpy(DefaultName, FullPath);
	    strcat(DefaultName, DEFAULT_OUT_NAME);
	    if (rename(FoutTmpName, DefaultName) == 0) {
		(void)snprintf(s, sizeof(s), 
			      "Failed to rename out file - left as %s.",
			      DefaultName);
		GIF_MESSAGE(s);
	    }
	    else {
		unlink(FoutTmpName);
		GIF_MESSAGE("Failed to rename out file - deleted.");
	    }
	}
    }
    else {
	fclose(Fout);
	unlink(FoutTmpName);
	GIF_MESSAGE("File too small - not renamed.");
    }

    return 0;
}
