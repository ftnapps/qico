/******************************************************************
 * Janus protocol defines
 * $Id: janus.h,v 1.1.1.1 2003/07/12 21:26:45 sisoft Exp $
 ******************************************************************/
#ifndef __JANUS_H__
#define __JANUS_H__

/* Misc. Constants */
#define JANUS_EFFICIENCY 90  /* Estimate Janus xfers at 90% throughput       */

/* File Transmission States */
#define XDONE      0         /* All done, no more files to transmit          */
#define XSENDFNAME 1         /* Send filename packet                         */
#define XRCVFNACK  2         /* Wait for filename packet ACK                 */
#define XSENDBLK   3         /* Send next block of file data                 */
#define XRCVEOFACK 4         /* Wait for EOF packet ACK                      */
#define XSENDFREQNAK 5
#define XRCVFRNAKACK 6


/* File Reception States */
#define RDONE      0         /* All done, nothing more to receive            */
#define RRCVFNAME  1         /* Wait for filename packet                     */
#define RRCVBLK    2         /* Wait for next block of file data             */

/* Packet Types */
#define NOPKT       0        /* No packet received yet; try again later      */
#define BADPKT     '@'       /* Bad packet received; CRC error, overrun, etc.*/
#define FNAMEPKT   'A'       /* Filename info packet                         */
#define FNACKPKT   'B'       /* Filename packet ACK                          */
#define BLKPKT     'C'       /* File data block packet                       */
#define RPOSPKT    'D'       /* Transmitter reposition packet                */
#define EOFACKPKT  'E'       /* EOF packet ACK                               */
#define HALTPKT    'F'       /* Immediate screeching halt packet             */
#define HALTACKPKT 'G'       /* Halt packet ACK for ending batch   			*/

#define FREQPKT    'H'       /* File request packet */
#define FREQNAKPKT 'I'       /* Requested file not found */
#define FRNAKACKPKT 'J'       /* We've understood it */


/* Non-byte values returned by rcvbyte() */
#define PKTSTRT   (-16)
#define PKTSTRT32   (-32)
#define PKTEND    (-10)

/* Bytes we need to watch for */
#define PKTSTRTCHR  'a'
#define PKTENDCHR   'b'
#define PKTSTRTCHR32    'c'

#define BUFMAX 2048

#define JCAP_CRC32      0x80
#define JCAP_FREQ       0x40
#define JCAP_CLEAR8     0x20
#define OUR_JCAPS       (JCAP_FREQ|JCAP_CRC32)


extern void sendpkt(byte *buf, int len, int type);
extern void sendpkt32(byte *buf, int len, int type);
extern byte rcvpkt();
extern void endbatch();
extern void txbyte(int c);
void getfname(flist_t **l);
extern int janus();

#endif
