/**********************************************************
 * File: ls_zsend.c
 * Created at Sun Oct 29 18:51:46 2000 by lev // lev@serebryakov.spb.ru
 * 
 * $Id: ls_zsend.c,v 1.2 2000/11/06 08:56:16 lev Exp $
 **********************************************************/
/*

   ZModem file transfer protocol. Written from scratches.
   Support CRC16, CRC32, RLE Encoding, variable header, ZedZap (big blocks).
   Send files.

*/
#include "mailer.h"
#include "ftn.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "qconf.h"
#include "ver.h"
#include "qipc.h"
#include "globals.h"
#include "defs.h"
#include "ls_zmodem.c"


/* Function, which start receiving and receive all files */
int ls_