/* Misc. Constants */
#define JANUS_EFFICIENCY 90  /* Estimate Janus xfers at 90% throughput       */

/* File Transmission States */
#define XDONE      0         /* All done, no more files to transmit          */
#define XSENDFNAME 1         /* Send filename packet                         */
#define XRCVFNACK  2         /* Wait for filename packet ACK                 */
#define XSENDBLK   3         /* Send next block of file data                 */
#define XRCVEOFACK 4         /* Wait for EOF packet ACK                      */

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
#define HALTACKPKT 'G'       /* Halt packet ACK for ending batch             */

/* Non-byte values returned by rcvbyte() */
#define BUFEMPTY  (-1)
#define PKTSTRT   (-2)
#define PKTEND    (-3)
#define NOCARRIER (-4)

/* Bytes we need to watch for */
#define PKTSTRTCHR  'a'
#define PKTENDCHR   'b'

/* Various action flags */
#define GOOD_XFER    0
#define FAILED_XFER  1
#define INITIAL_XFER 2
#define ABORT_XFER   3

