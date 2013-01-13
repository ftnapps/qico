/*=============================================================================
                       The HYDRA protocol was designed by
                 Arjen G. Lentz, LENTZ SOFTWARE-DEVELOPMENT and
                             Joaquim H. Homrighausen
                  COPYRIGHT (C) 1991-1993; ALL RIGHTS RESERVED
=============================================================================*/
/*
 * $Id: hydra.h,v 1.12 2005/05/16 11:20:13 mitry Exp $
 *
 * $Log: hydra.h,v $
 * Revision 1.12  2005/05/16 11:20:13  mitry
 * Updated function prototypes. Changed code a bit.
 *
 * Revision 1.11  2005/04/07 20:58:24  mitry
 * Cleaned code
 *
 * Revision 1.10  2005/03/31 19:40:38  mitry
 * Update function prototypes and it's duplication
 *
 * Revision 1.9  2005/03/28 15:17:42  mitry
 * Added non-blocking i/o support
 *
 * Revision 1.8  2005/02/12 18:57:17  mitry
 * Hydra-4/8/16k and chat are back. Chat is still buggy. Oversized Hydra too.
 *
 */


/* HYDRA Specification Revision/Timestamp ---------Revision------Date------- */
#define H_REVSTAMP   0x2b1aab00L                /* 001           01 Dec 1992 */
#define H_REVISION   1

/* HYDRA Basic Values ------------------------------------------------------ */
#ifndef XON
#define XON          ('Q' - '@')        /* Ctrl-Q (^Q) xmit-on character     */
#define XOFF         ('S' - '@')        /* Ctrl-S (^S) xmit-off character    */
#endif
#define H_DLE        ('X' - '@')        /* Ctrl-X (^X) HYDRA DataLinkEscape  */
#define H_MINBLKLEN    64               /* Min. length of a HYDRA data block */
#define H_OVERHEAD      8               /* Max. no. control bytes in a pkt   */

#ifndef HYDRA8K16K

#define H_MAXBLKLEN  2048               /* Max. length of a HYDRA data block */
#define H_MAXPKTLEN  ((H_MAXBLKLEN + H_OVERHEAD + 5) * 3)     /* Encoded pkt */
#define H_BUFLEN     (H_MAXPKTLEN + 16) /* Buffer sizes: max.enc.pkt + slack */

#else

#define H_MAXBLKLEN(n) (2048*(n))       /* Max. length of a HYDRA data block */
#define H_MAXPKTLEN(n) ((H_MAXBLKLEN(n)+H_OVERHEAD+5)*3)      /* Encoded pkt */
#define H_BUFLEN(n)    (H_MAXPKTLEN(n)+16) /* Buffer sizes: max.enc.pkt + slack */

#endif

#define H_PKTPREFIX    31               /* Max length of pkt prefix string   */
#define H_FLAGLEN       3               /* Length of a flag field            */
#define H_RETRIES      10               /* No. retries in case of an error   */
#define H_MINTIMER     10               /* Minimum timeout period            */
#define H_MAXTIMER     60               /* Maximum timeout period            */
#define H_START         5               /* Timeout for re-sending startstuff */
#define H_IDLE         20               /* Idle? tx IDLE pkt every 20 secs   */
#define H_BRAINDEAD   120               /* Braindead in 2 mins (120 secs)    */

/* HYDRA Special file offsets ---------------------------------------------- */
#define H_SKIP      (-1L)               /* Skip file                         */
#define H_SUSPEND   (-2L)               /* Suspend (refuse) file             */

/* HYDRA Return codes ------------------------------------------------------ */
#define XFER_ABORT    (-1)              /* Failed on this file & abort xfer  */
#define XFER_SKIP       0               /* Skip this file but continue xfer  */
#define XFER_OK         1               /* File was sent, continue transfer  */
/* New */
#define XFER_SUSPEND    2               /* Suspend file (don't delete)       */


/* HYDRA Transmitter States ------------------------------------------------ */
#define HTX_DONE        0               /* All over and done                 */
#define HTX_START       1               /* Send start autostr + START pkt    */
#define HTX_SWAIT       2               /* Wait for any pkt or timeout       */
#define HTX_INIT        3               /* Send INIT pkt                     */
#define HTX_INITACK     4               /* Wait for INITACK pkt              */
#define HTX_RINIT       5               /* Wait for HRX_INIT -> HRX_FINFO    */
#define HTX_FINFO       6               /* Send FINFO pkt                    */
#define HTX_FINFOACK    7               /* Wait for FINFOACK pkt             */
#define HTX_XDATA       8               /* Send next packet with file data   */
#define HTX_DATAACK     9               /* Wait for DATAACK packet           */
#define HTX_XWAIT      10               /* Wait for HRX_END                  */
#define HTX_EOF        11               /* Send EOF pkt                      */
#define HTX_EOFACK     12               /* End of file, wait for EOFACK pkt  */
#define HTX_REND       13               /* Wait for HRX_END && HTD_DONE      */
#define HTX_END        14               /* Send END pkt (finish session)     */
#define HTX_ENDACK     15               /* Wait for END pkt from other side  */

/* HYDRA Device Packet Transmitter States ---------------------------------- */
#define HTD_DONE        0               /* No device data pkt to send        */
#define HTD_DATA        1               /* Send DEVDATA pkt                  */
#define HTD_DACK        2               /* Wait for DEVDACK pkt              */

/* HYDRA Receiver States --------------------------------------------------- */
#define HRX_DONE        0               /* All over and done                 */
#define HRX_INIT        1               /* Wait for INIT pkt                 */
#define HRX_FINFO       2               /* Wait for FINFO pkt of next file   */
#define HRX_DATA        3               /* Wait for next DATA pkt            */

/* HYDRA Packet Types ------------------------------------------------------ */
#define HPKT_START     'A'              /* Startup sequence                  */
#define HPKT_INIT      'B'              /* Session initialisation            */
#define HPKT_INITACK   'C'              /* Response to INIT pkt              */
#define HPKT_FINFO     'D'              /* File info (name, size, time)      */
#define HPKT_FINFOACK  'E'              /* Response to FINFO pkt             */
#define HPKT_DATA      'F'              /* File data packet                  */
#define HPKT_DATAACK   'G'              /* File data position ACK packet     */
#define HPKT_RPOS      'H'              /* Transmitter reposition packet     */
#define HPKT_EOF       'I'              /* End of file packet                */
#define HPKT_EOFACK    'J'              /* Response to EOF packet            */
#define HPKT_END       'K'              /* End of session                    */
#define HPKT_IDLE      'L'              /* Idle - just saying I'm alive      */
#define HPKT_DEVDATA   'M'              /* Data to specified device          */
#define HPKT_DEVDACK   'N'              /* Response to DEVDATA pkt           */

#define HPKT_HIGHEST   'N'              /* Highest known pkttype in this imp */

/* HYDRA Internal Pseudo Packet Types -------------------------------------- */
#define H_NOPKT         0               /* No packet (yet)                   */
#define H_CANCEL      (-1)              /* Received cancel sequence 5*Ctrl-X */
#define H_CARRIER     (-2)              /* Lost carrier                      */
#define H_SYSABORT    (-3)              /* Aborted by operator on this side  */
#define H_TXTIME      (-4)              /* Transmitter timeout               */
#define H_DEVTXTIME   (-5)              /* Device transmitter timeout        */
#define H_BRAINTIME   (-6)              /* Braindead timeout (quite fatal)   */

/* HYDRA Packet Format: START[<data>]<type><crc>END ------------------------ */
#define HCHR_PKTEND    'a'              /* End of packet (any format)        */
#define HCHR_BINPKT    'b'              /* Start of binary packet            */
#define HCHR_HEXPKT    'c'              /* Start of hex encoded packet       */
#define HCHR_ASCPKT    'd'              /* Start of shifted 7bit encoded pkt */
#define HCHR_UUEPKT    'e'              /* Start of uuencoded packet         */

/* HYDRA Local Storage of INIT Options (Bitmapped) ------------------------- */
#define HOPT_XONXOFF  (0x00000001L)     /* Escape XON/XOFF                   */
#define HOPT_TELENET  (0x00000002L)     /* Escape CR-'@'-CR (Telenet escape) */
#define HOPT_CTLCHRS  (0x00000004L)     /* Escape ASCII 0-31 and 127         */
#define HOPT_HIGHCTL  (0x00000008L)     /* Escape above 3 with 8th bit too   */
#define HOPT_HIGHBIT  (0x00000010L)     /* Escape ASCII 128-255 + strip high */
#define HOPT_CANBRK   (0x00000020L)     /* Can transmit a break signal       */
#define HOPT_CANASC   (0x00000040L)     /* Can transmit/handle ASC packets   */
#define HOPT_CANUUE   (0x00000080L)     /* Can transmit/handle UUE packets   */
#define HOPT_CRC32    (0x00000100L)     /* Packets with CRC-32 allowed       */
#define HOPT_DEVICE   (0x00000200L)     /* DEVICE packets allowed            */
#define HOPT_FPT      (0x00000400L)     /* Can handle filenames with paths   */
#define HOPT_NFI      (0x00000800L)     /* Numerated FINFO/RPOS/FINFOACK     */

/* What we can do */
#define HCAN_OPTIONS  (HOPT_XONXOFF|HOPT_TELENET|HOPT_CTLCHRS|HOPT_HIGHCTL|HOPT_HIGHBIT|HOPT_CANBRK|HOPT_CANASC|HOPT_CANUUE|HOPT_CRC32|HOPT_DEVICE)
/* leave HOPT_DEVICE for later processing */

/* Vital options if we ask for any; abort if other side doesn't support them */
#define HNEC_OPTIONS  (HOPT_XONXOFF|HOPT_TELENET|HOPT_CTLCHRS|HOPT_HIGHCTL|HOPT_HIGHBIT|HOPT_CANBRK)

/* Non-vital options; nice if other side supports them, but doesn't matter */
#define HUNN_OPTIONS  (HOPT_CANASC|HOPT_CANUUE|HOPT_CRC32|HOPT_DEVICE)

/* Default options */
#define HDEF_OPTIONS  (0x0L)

/* rxoptions during init (needs to handle ANY link yet unknown at that point */
#define HRXI_OPTIONS  (HOPT_XONXOFF|HOPT_TELENET|HOPT_CTLCHRS|HOPT_HIGHCTL|HOPT_HIGHBIT)

/* ditto, but this time txoptions */
#define HTXI_OPTIONS  (0x0L)

/* HYDRA Prototypes */

void	hydra_init(dword, int, int);
void	hydra_deinit(void);
int	hydra_send(char *, char *);
boolean	hydra_devfree(void);
boolean	hydra_devsend(char *, byte *, word);
boolean	hydra_devfunc(char *, void (*) (byte *, word));

/* end of hydra.h */
