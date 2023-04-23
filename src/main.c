/**
 * Cosmore
 * Copyright (c) 2020-2023 Scott Smitelli and contributors
 *
 * Based on COSMO{1..3}.EXE distributed with "Cosmo's Cosmic Adventure"
 * Copyright (c) 1992 Apogee Software, Ltd.
 *
 * This source code is licensed under the MIT license found in the LICENSE file
 * in the root directory of this source tree.
 */

/*****************************************************************************
 *                           COSMORE MAIN FUNCTION                           *
 *                                                                           *
 * This is the only compilation unit that specifies pure 8086 mode. This is  *
 * necessary to allow the program to successfully start on an IBM PC or XT,  *
 * test for the presence of an 80286 processor, fail to detect one, and      *
 * present the error message and prompt. The Makefile controls this option.  *
 *                                                                           *
 * You almost certainly want InnerMain() in game1.c instead.                 *
 *****************************************************************************/

#include "glue.h"

void main(int argc, char *argv[])
{
    int cputype = GetProcessorType();

    if (cputype < CPUTYPE_80188) {
        byte response;

        /* Grammatical errors preserved faithfully */
        printf("You're computer appears to be an 8088/8086 XT system.\n\n");
        printf("Cosmo REQUIRES an AT class (80286) or better to run due to\n");
        printf("it's high-speed animated graphics.\n\n");
        printf("Note:  This game will crash on XT systems.\n");
        printf("Do you wish to continue if you really have an AT system or better (Y/N)?");

        response = getch();
        if (response == 'Y' || response == 'y') {
            InnerMain(argc, argv);
        }

        exit(EXIT_SUCCESS);
    } else {
        InnerMain(argc, argv);
    }
}
