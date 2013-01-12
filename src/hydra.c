/*=============================================================================
                       The HYDRA protocol was designed by
                 Arjen G. Lentz, LENTZ SOFTWARE-DEVELOPMENT and
                             Joaquim H. Homrighausen
                  COPYRIGHT (C) 1991-1993; ALL RIGHTS RESERVED
=============================================================================*/
/*
 * $Id: hydra.c,v 1.29 2005/06/10 20:56:25 mitry Exp $
 *
 * $Log: hydra.c,v $
 * Revision 1.29  2005/06/10 20:56:25  mitry
 * Changed exit from hydra_send()
 *
 * Revision 1.28  2005/05/16 20:31:50  mitry
 * Changed code a bit
 *
 * Revision 1.27  2005/05/06 20:53:05  mitry
 * Changed misc code
 *
 * Revision 1.26  2005/04/07 20:58:24  mitry
 * Cleaned code
 *
 * Revision 1.25  2005/04/04 19:43:55  mitry
 * Added timeout arg to BUFFLUSH() - tty_bufflush()
 *
 * Revision 1.24  2005/04/01 20:32:48  mitry
 * Changed hydra_deinit()
 *
 * Revision 1.23  2005/03/31 20:11:18  mitry
 * Added check for tty_gothup's HUP_OPERATOR value
 *
 * Revision 1.22  2005/03/29 20:35:11  mitry
 * Check tx queue before sending next data block
 *
 * Revision 1.21  2005/03/28 15:17:42  mitry
 * Added non-blocking i/o support
 *
 * Revision 1.20  2005/02/23 21:26:23  mitry
 * Removed commented code
 *
 * Revision 1.19  2005/02/19 21:34:38  mitry
 * hydra_chat() is dynamically assigned now
 * Fixed debug log output on linux OS
 *
 * Revision 1.18  2005/02/12 18:57:17  mitry
 * Hydra-4/8/16k and chat are back. Chat is still buggy. Oversized Hydra too.
 *
 */

#include "headers.h"
#include "hydra.h"
#include "crc.h"
#include "tty.h"

#ifdef NEED_DEBUG
/* HYDRA Transmitter States ------------------------------------------------ */

static char *h_tx_states[] = {
    "HTX_DONE",        /*  0  All over and done                 */
    "HTX_START",       /*  1  Send start autostr + START pkt    */
    "HTX_SWAIT",       /*  2  Wait for any pkt or timeout       */
    "HTX_INIT",        /*  3  Send INIT pkt                     */
    "HTX_INITACK",     /*  4  Wait for INITACK pkt              */
    "HTX_RINIT",       /*  5  Wait for HRX_INIT -> HRX_FINFO    */
    "HTX_FINFO",       /*  6  Send FINFO pkt                    */
    "HTX_FINFOACK",    /*  7  Wait for FINFOACK pkt             */
    "HTX_XDATA",       /*  8  Send next packet with file data   */
    "HTX_DATAACK",     /*  9  Wait for DATAACK packet           */
    "HTX_XWAIT",       /* 10  Wait for HRX_END                  */
    "HTX_EOF",         /* 11  Send EOF pkt                      */
    "HTX_EOFACK",      /* 12  End of file, wait for EOFACK pkt  */
    "HTX_REND",        /* 13  Wait for HRX_END && HTD_DONE      */
    "HTX_END",         /* 14  Send END pkt (finish session)     */
    "HTX_ENDACK"       /* 15  Wait for END pkt from other side  */
};

static int h_tx_states_len = sizeof( h_tx_states ) / sizeof( char * );

/* HYDRA Device Packet Transmitter States ---------------------------------- */
static char *h_dpkttx_states[] = {
    "HTD_DONE",        /* 0   No device data pkt to send        */
    "HTD_DATA",        /* 1   Send DEVDATA pkt                  */
    "HTD_DACK"         /* 2   Wait for DEVDACK pkt              */
};

static int h_dpkttx_states_len = sizeof( h_dpkttx_states ) / sizeof( char * );

/* HYDRA Receiver States --------------------------------------------------- */
static char *h_rx_states[] = {
    "HRX_DONE",        /* 0   All over and done                 */
    "HRX_INIT",        /* 1   Wait for INIT pkt                 */
    "HRX_FINFO",       /* 2   Wait for FINFO pkt of next file   */
    "HRX_DATA"         /* 3   Wait for next DATA pkt            */
};

static int h_rx_states_len = sizeof( h_rx_states ) / sizeof( char * );

/* HYDRA Packet Types ------------------------------------------------------ */
static char *h_packets[] = {
    "HPKT_START",      /* 'A' Startup sequence                  */
    "HPKT_INIT",       /* 'B' Session initialisation            */
    "HPKT_INITACK",    /* 'C' Response to INIT pkt              */
    "HPKT_FINFO",      /* 'D' File info (name, size, time)      */
    "HPKT_FINFOACK",   /* 'E' Response to FINFO pkt             */
    "HPKT_DATA",       /* 'F' File data packet                  */
    "HPKT_DATAACK",    /* 'G' File data position ACK packet     */
    "HPKT_RPOS",       /* 'H' Transmitter reposition packet     */
    "HPKT_EOF",        /* 'I' End of file packet                */
    "HPKT_EOFACK",     /* 'J' Response to EOF packet            */
    "HPKT_END",        /* 'K' End of session                    */
    "HPKT_IDLE",       /* 'L' Idle - just saying I'm alive      */
    "HPKT_DEVDATA",    /* 'M' Data to specified device          */
    "HPKT_DEVDACK"     /* 'N' Response to DEVDATA pkt           */
};

static int h_packets_len = sizeof( h_packets ) / sizeof( char * );

/* HYDRA Packet Format: START[<data>]<type><crc>END ------------------------ */
static char *h_formats[] = {
    "HCHR_PKTEND",     /* 'a' End of packet (any format)        */
    "HCHR_BINPKT",     /* 'b' Start of binary packet            */
    "HCHR_HEXPKT",     /* 'c' Start of hex encoded packet       */
    "HCHR_ASCPKT",     /* 'd' Start of shifted 7bit encoded pkt */
    "HCHR_UUEPKT"      /* 'e' Start of uuencoded packet         */
};

static int h_formats_len = sizeof( h_formats ) / sizeof( char * );

#endif

static int hydra_modifier;


/* HYDRA Some stuff to aid readability of the source and prevent typos ----- */
#define h_updcrc16(crc,c)  (crc16tab[(       crc ^ (c)) & 0xff] ^ ((crc >> 8) & 0x00ff))
#define h_updcrc32(crc,c)  (crc32tab[((byte) crc ^ (c)) & 0xff] ^ ((crc >> 8) & 0x00ffffffL))
#define h_crc16poly        (0x8408)
#define h_crc32poly        (0xedb88320L)
#define h_crc16test(crc)   (((crc) == CRC16PRP_TEST) ? 1 : 0)
#define h_crc32test(crc)   (((crc) == CRC32_TEST) ? 1 : 0)
#define h_uuenc(c)         (((c) & 0x3f) + '!')
#define h_uudec(c)         (((c) - '!') & 0x3f)
/*
#define h_long1(buf)       (*((long *) (buf)))
#define h_long2(buf)       (*((long *) ((buf) + ((int) LONGx1))))
#define h_long3(buf)       (*((long *) ((buf) + ((int) LONGx2))))
*/
typedef dword              h_timer;
#define h_timer_set(t)     timer_set(t)
#define h_timer_running(t) timer_running(t)
#define h_timer_expired(t) timer_expired(t)
#define h_timer_reset()    timer_reset()
#define h_timer_get()      timer_start()			/*AGL:15jul93*/

#define h_timer_expired_agl(t,now) ((now) > (t))		/*AGL:15jul93*/

#define inteli(x) (x)
#define intell(x) (x)

#define crc16block(data, len)	crc16prp(data, len)

/* HYDRA's memory ---------------------------------------------------------- */
static  int     originator;                     /* are we the orig side?     */
static  int     batchesdone;                    /* No. HYDRA batches done    */
static  boolean hdxlink;                        /* hdx link & not orig side  */
static  dword   options;                        /* INIT options hydra_init() */
static  word    timeOut;                        /* general timeout in secs   */
static  char    abortstr[] = { 24,24,24,24,24,24,24,24,8,8,8,8,8,8,8,8,8,8,0 };
static  char   *hdxmsg     = "Fallback to one-way xfer";
static  char   *pktprefix  = "";
static  char   *autostr    = "hydra\r";
static  word   *crc16tab;                       /* CRC-16 table              */
static  dword  *crc32tab;                       /* CRC-32 table              */

static  byte   *txBuf,         *rxBuf;          /* packet buffers            */
static  dword   txoptions,      rxoptions;      /* HYDRA options (INIT seq)  */
static  char    txpktprefix[H_PKTPREFIX + 1];   /* pkt prefix str they want  */
static  long    txwindow,       rxwindow;       /* window size (0=streaming) */
static  h_timer                 braindead;      /* braindead timer           */
static  byte   *txbufin;                        /* read data from disk here  */
static  byte    txLastc;                        /* last byte put in txbuf    */
static  byte                    rxdle;          /* count of received H_DLEs  */
static  byte                    rxpktformat;    /* format of pkt receiving   */
static  byte                   *rxBufPtr;       /* current position in rxbuf */
static  byte                   *rxBufMax;       /* highwatermark of rxbuf    */
/*
static  char    txfname[MAX_PATH+1],
                rxfname[MAX_PATH+1];*/          /* fname of current files    */
/*
static  char                   *rxpathname;*/   /* pointer to rx pathname    */
static  h_timer /* txftime, */  rxftime;        /* file timestamp (UNIX)     */
static  long    txfsize,        rxfsize;        /* file length               */

/*static  int     txfd,           rxfd;            file handles              */
static  word                    rxpktlen;       /* length of last packet     */
static  word                    rxBlkLen;       /* len of last good data blk */
static  byte    txState,        rxState;        /* xmit/recv states          */
static  long    txPos,          rxPos;          /* current position in files */
static  word    txBlkLen;                       /* length of last block sent */
static  word    txMaxBlkLen;                    /* max block length allowed  */
static  long    txlastack;                      /* last dataack received     */
static  long    txStart; /*,        rxStart*/   /* time we started this file */
static  long    txoffset,       rxoffset;       /* offset in file we begun   */
static  h_timer txtimer,        rxtimer;        /* retry timers              */
static  word    txretries,      rxretries;      /* retry counters            */
static  long                    rxlastsync;     /* filepos last sync retry   */
static  long    txsyncid,       rxsyncid;       /* id of last resync         */
static  word    txgoodneeded;                   /* to send before larger blk */
static  word    txgoodbytes;                    /* no. sent at this blk size */

static  long    hydra_txwindow, hydra_rxwindow; /* Hydra's tx/rx window size */

static  h_timer tnow;                           /* keeps now time            */
static  dword   hoCAN;                          /* our HCAN_OPTIONS          */
static  boolean verboseLog;                     /* use verbose ("HCHR_PKTEND")
                                                   of short ('a') form of msg
                                                   in debug log              */


typedef struct _HYDRA_PKT HYPKT;

struct _HYDRA_PKT {
    int len;			/* Packet length */
    byte *pkt;			/* Packet itself (ready to send) */
};

typedef struct _HYDRA_TXBUF HYTXB;

struct _HYDRA_TXBUF {
    byte *tx_buf;		/* Transmit buffer */
    int tx_ptr;			/* Next byte to send pointer */
    int tx_left;		/* Data size left to send */
    int tx_size;		/* tx_buf size */
    int prefix_done;

    HYPKT *pqueue;		/* Outgoing packets queue */
    int npkts;			/* Number of packets in queue */
};

static  HYTXB hytxb;

struct _h_flags {
    char  *str;
    dword  val;
};

static struct _h_flags h_flags[] = {
    { "XON", HOPT_XONXOFF },
    { "TLN", HOPT_TELENET },
    { "CTL", HOPT_CTLCHRS },
    { "HIC", HOPT_HIGHCTL },
    { "HI8", HOPT_HIGHBIT },
    { "BRK", HOPT_CANBRK  },
    { "ASC", HOPT_CANASC  },
    { "UUE", HOPT_CANUUE  },
    { "C32", HOPT_CRC32   },
    { "DEV", HOPT_DEVICE  },
    { "FPT", HOPT_FPT     },
    { NULL , 0x0L         }
};


/*---------------------------------------------------------------------------*/
static void hydra_msgdev(byte *data, word len)
{       /* text is already NUL terminated by calling func hydra_devrecv() */
    len = len;
    write_log( "HydraMsg: %s",data );
}/*hydra_msgdev()*/

static void hydra_chat(byte *data, word len)
{
    c_devrecv( data, len);
}

/*---------------------------------------------------------------------------*/
static  word    devtxstate;                     /* dev xmit state            */
static  h_timer devtxtimer;                     /* dev xmit retry timer      */
static  word    devtxretries;                   /* dev xmit retry counter    */
static  long    devtxid,        devrxid;        /* id of last devdata pkt    */
static  char    devtxdev[H_FLAGLEN + 1];        /* xmit device ident flag    */
static  byte   *devtxbuf;                       /* ptr to usersupplied dbuf  */
static  word    devtxlen;                       /* len of data in xmit buf   */

struct _h_dev {
    char  *dev;
    void (*func) (byte *data, word len);
};

static  struct _h_dev h_dev[] = {
    { "MSG", hydra_msgdev },                /* internal protocol msg     */
    { "CON", hydra_chat   },                /* text to console (chat)    */
    { "PRN", NULL         },                /* data to printer           */
    { "ERR", NULL         },                /* text to error output      */
    { NULL , NULL         }
};


/*---------------------------------------------------------------------------*/
boolean hydra_devfree(void)
{
    if ( devtxstate || !( txoptions & HOPT_DEVICE ) || txState >= HTX_END )
        return (false);                      /* busy or not allowed       */
    else
        return (true);                       /* allowed to send a new pkt */
}/*hydra_devfree()*/


/*---------------------------------------------------------------------------*/
boolean hydra_devsend(char *dev, byte *data, word len)
{
    if ( !dev || !data || !len || !hydra_devfree() )
        return (false);

#ifndef HYDRA8K16K
    strncpy( devtxdev, dev, H_FLAGLEN );
    devtxdev[H_FLAGLEN] = '\0';
#else    
    xstrcpy( devtxdev, dev, H_FLAGLEN + 1);
#endif

    strupr( devtxdev );
    devtxbuf = data;
    
#ifndef HYDRA8K16K
    devtxlen = (word) (( len > H_MAXBLKLEN ) ? H_MAXBLKLEN : len );
#else    
    devtxlen= (word) (( len > H_MAXBLKLEN( hydra_modifier )) ?
        H_MAXBLKLEN( hydra_modifier ) : len );
#endif

    devtxid++;
    devtxtimer   = h_timer_reset();
    devtxretries = 0;
    devtxstate   = HTD_DATA;

    /* special for chat, only prolong life if our side keeps typing! */
    if ( chattimer > 1L && !strcmp( devtxdev, "CON" ) && txState == HTX_REND )
        braindead = h_timer_set( H_BRAINDEAD );

    return (true);
}/*hydra_devsend()*/


/*---------------------------------------------------------------------------*/
boolean hydra_devfunc(char *dev, void (*func) (byte *data, word len))
{
    register int i;

    for( i = 0; h_dev[i].dev; i++ ) {
        if ( !strncasecmp( dev, h_dev[i].dev, H_FLAGLEN )) {
            h_dev[i].func = func;
            return (true);
        }
    }

    return (false);
}/*hydra_devfunc()*/


#ifdef NEED_DEBUG

#define ERR_LEN		17

#define HFS( f )	hydra_format_str( (char) f )
#define HPS( p )	hydra_packet_str( (char) p )

/*---------------------------------------------------------------------------*/
static char* hydra_packet_str(const char c)
{
    register int i = c - 'A';
    static char err[ERR_LEN];
    char *r = err;

    if ( i < 0 || i >= h_packets_len ) {
        snprintf( err, ERR_LEN, "'%c' (Unknown)", c );
    } else {
        if ( verboseLog )
            r = h_packets[ i ];
        else {
            snprintf( err, ERR_LEN, "'%c'", c );
        }
    }
    return r;
}


/*---------------------------------------------------------------------------*/
static char* hydra_format_str(const char c)
{
    register int i = c - 'a';
    static char err[ERR_LEN];
    char *r = err;

    if ( i < 0 || i >= h_formats_len ) {
        snprintf( err, ERR_LEN, "'%c' (Unknown)", c );
    } else {
        if ( verboseLog )
            r = h_formats[ i ];
        else {
            snprintf( err, ERR_LEN, "'%c'", c );
        }
    }
    return r ;
}


/*---------------------------------------------------------------------------*/
static char* hydra_tx_state_str(const int c)
{
    static char err[ERR_LEN];
    char *r = err;

    if ( c < 0 || c >= h_tx_states_len ) {
        snprintf( err, ERR_LEN, "'%d' (Unknown)", c );
    } else
        r = h_tx_states[ c ];
    return r;
}


/*---------------------------------------------------------------------------*/
static char* hydra_rx_state_str(const int c)
{
    static char err[ERR_LEN];
    char *r = err;

    if ( c < 0 || c >= h_rx_states_len ) {
        snprintf( err, ERR_LEN, "'%d' (Unknown)", c );
    } else
        r = h_rx_states[ c ];
    return r;
}


/*---------------------------------------------------------------------------*/
static char* hydra_dpkttx_state_str(const int c)
{
    static char err[ERR_LEN];
    char *r = err;

    if ( c < 0 || c >= h_dpkttx_states_len ) {
        snprintf( err, ERR_LEN, "'%d' (Unknown)", c );
    } else
        r = h_dpkttx_states[ c ];
    return r;
}


/*---------------------------------------------------------------------------*/
static char* h_revdate(long revstamp)
{
    static char buf[12];
    struct tm *t;
    time_t tt;

    tt = (time_t) revstamp;
    t = localtime( &tt );
    strftime( buf, 12, "%d %b %Y", t );

    return buf;
}/* h_revdate() */
#endif


/*---------------------------------------------------------------------------*/
static void hydra_devrecv(void)
{
    register char *p = (char *) rxBuf;
    register int   i;
    word len = rxpktlen;

    p += (int) (LONGx1);                            /* skip the id long  */
    len -= (word) (LONGx1);
    for( i = 0; h_dev[i].dev; i++ ) {               /* walk through devs */
        if ( !strncmp( p, h_dev[i].dev, H_FLAGLEN )) {
            if ( h_dev[i].func ) {
                len -= ((word) strlen(p) + 1);      /* sub devstr len    */
                p += ((int) strlen(p) + 1);         /* skip devtag       */
                p[len] = '\0';                      /* NUL terminate     */
                (*h_dev[i].func)((byte *) p, len);  /* call output func  */
            }
            break;
        }
    }
}/*hydra_devrecv()*/


/*---------------------------------------------------------------------------*/
static void *hydra_putlong(void *buf, long val)
{
    register byte *b = (byte *) buf;

    b[0] = (byte) val;
    b[1] = (byte) ( val >> 8 );
    b[2] = (byte) ( val >> 16 );
    b[3] = (byte) ( val >> 24 );

    return buf;
}

/*---------------------------------------------------------------------------*/
static char *hydra_putword(void *buf, word val)
{
    register byte *b = (byte *) buf;

    b[0] = (byte) val;
    b[1] = (byte) ( val >> 8 );
	    
    return buf;
}

/*---------------------------------------------------------------------------*/
static long hydra_getlong(const void *buf)
{

    register byte *b = (byte *) buf;
    register dword d = 0L;

    d += b[0];
    d += b[1] << 8;
    d += b[2] << 16;
    d += b[3] << 24;

    return d;
}

/*---------------------------------------------------------------------------
static int hydra_getword(const void *buf)
{
    register byte *b = (byte *) buf;
    register word w = 0;

    w += b[0];
    w += b[1] << 8;

    return w;
}
*/

/*---------------------------------------------------------------------------*/
static void put_flags(char *buf, struct _h_flags flags[], long val)
{
    register char *p;
    register int   i;

    p = buf;
    for( i = 0; flags[i].val; i++ ) {
        if ( val & flags[i].val ) {
            if ( p > buf )
                *p++ = ',';
            strcpy( p, flags[i].str );
            p += H_FLAGLEN;
        }
    }
    *p = '\0';
}/*put_flags()*/


/*---------------------------------------------------------------------------*/
static dword get_flags(char *buf, struct _h_flags flags[])
{
    register dword  val;
    register char  *p;
    register int    i;

    DEBUG(('H',4,"flags='%s'", buf));

    val = 0x0L;
    while(( p = strsep( &buf, "," ))) {
        DEBUG(('H',4,"flag='%s'", p));
        for( i = 0; flags[i].val; i++ ) {
            if ( !strcmp( p, flags[i].str )) {
                val |= flags[i].val;
                break;
            }
        }
    }

        return (val);
}/*get_flags()*/


/*---------------------------------------------------------------------------*/
static int hydra_adjust_blklen(int i)
{
    if      ( i <=  64 )  i =   64;
    else if ( i <= 128 )  i =  128;
    else if ( i <= 256 )  i =  256;
    else if ( i <= 512 )  i =  512;

#ifndef HYDRA8K16K
    else                  i = 1024;
#else
    else if ( i <= 1024 ) i = 1024;
    else if ( i <= 2048 ) i = 2048;
    else if ( i <= 4096 ) i = 4096;
    else                  i = 8192;

    if      ( hydra_modifier == 8 && i > 8192 ) i = 8192;
    else if ( hydra_modifier == 4 && i > 4096 )	i = 4096;
    else if ( hydra_modifier == 2 && i > 2048 )	i = 2048;
    else if ( hydra_modifier == 1 && i > 1024 ) i = 1024;
#endif
    
    return i;

}/* hydra_adjust_blklen() */


/*---------------------------------------------------------------------------*/
static byte *put_binbyte(register byte *p, register byte c)
{
    register byte n;

    n = c;
    if (txoptions & HOPT_HIGHCTL)
       n &= 0x7f;

    if (n == H_DLE ||
        ((txoptions & HOPT_XONXOFF) && (n == XON || n == XOFF)) ||
        ((txoptions & HOPT_TELENET) && n == '\r' && txLastc == '@') ||
        ((txoptions & HOPT_CTLCHRS) && (n < 32 || n == 127))) {
       *p++ = H_DLE;
       c ^= 0x40;
    }

    *p++ = c;
    txLastc = n;

    return (p);
}/*put_binbyte()*/


/*---------------------------------------------------------------------------*/
static int hydra_get_timeout(void)
{
    int to = timeOut;
    tnow = h_timer_get();

    if ( h_timer_running( braindead ) && !h_timer_expired_agl( braindead, tnow ))
        to = MIN( to, timer_rest( braindead ));

    if ( h_timer_running( txtimer ) && !h_timer_expired_agl( txtimer, tnow ))
        to = MIN( to, timer_rest( txtimer ));

    if ( h_timer_running( devtxtimer ) && !h_timer_expired_agl( devtxtimer, tnow ))
        to = MIN( to, timer_rest( devtxtimer ));

    return MAX( 1, to );
}


/*---------------------------------------------------------------------------*/
static int hydra_check_timers(void)
{
    tnow = h_timer_get();

    switch( tty_gothup ) {
        case HUP_LINE:
            return H_CARRIER;

        case HUP_NONE:
            break;

        default:
            return H_SYSABORT;
    }

    if ( chattimer < 2 && txState == HTX_REND && rxState == HRX_DONE ) {
        chattimer = 0;
        return (HPKT_IDLE);
    }

    if ( h_timer_running( braindead ) && h_timer_expired_agl( braindead, tnow )) {
        DEBUG(('H',2,"rxpkt BrainDead (timer=%08lx  time=%08lx)", braindead, tnow));
        return (H_BRAINTIME);
    }

    if ( h_timer_running( txtimer ) && h_timer_expired_agl( txtimer, tnow )) {
        DEBUG(('H',2,"rxpkt TxTimer (timer=%08lx  time=%08lx)", txtimer, tnow));
        return (H_TXTIME);
    }

    if ( h_timer_running( devtxtimer ) && h_timer_expired_agl( devtxtimer, tnow )) {
        DEBUG(('H',2,"rxpkt DevTxTimer (timer=%08lx  time=%08lx)", devtxtimer, tnow));
        return (H_DEVTXTIME);
    }

    return (H_NOPKT);
}

           	
#ifdef NEED_DEBUG
/*---------------------------------------------------------------------------*/
void debugTransmittedPkt(int type, int crc32, word len, byte format, dword crcDebug)
{
    char *s1, *s2, *s3, *s4;
    char buf1[1024] = { '\0' };
    char buf2[1024] = { '\0' };
    int buflen;

    switch (type) {
        case HPKT_START:
             strcat( buf1, "START" );
             break;

        case HPKT_INIT:
             s1 = ((char *) txbufin) + ((int) strlen((char *) txbufin)) + 1;
             s2 = s1 + ((int) strlen(s1)) + 1;
             s3 = s2 + ((int) strlen(s2)) + 1;
             s4 = s3 + ((int) strlen(s3)) + 1;
             sprintf( buf1, "INIT (appinfo='%s')", (char *) txbufin );
             sprintf( buf2, "(can='%s'  want='%s'  options='%s'  pktprefix='%s')",
                 s1, s2, s3, s4 );
             break;

        case HPKT_INITACK:
             strcat( buf1, "INITACK");
             break;

        case HPKT_FINFO:
             sprintf( buf1, "FINFO ('%s')", txbufin );
             break;
        
        case HPKT_FINFOACK:
             if ( rxfd ) {
                 if (rxPos > 0L)    s1 = "Resume";
                 else               s1 = "BoF";
             }
             else if (rxPos == -1L) s1 = "Have";
             else if (rxPos == -2L) s1 = "Skip";
             else                   s1 = "EoB";
             sprintf( buf1, "FINFOACK (pos=%ld %s  rxstate=%s)"/*  rxfd=%d)"*/,
                 rxPos,s1, hydra_rx_state_str( rxState ) /* ,rxfd */);
             break;
        
        case HPKT_DATA:
             sprintf( buf1, "DATA (ofs=%ld  len=%d)",
                 hydra_getlong( txbufin ), (int) (len - 5));
             break;

        case HPKT_DATAACK:
             sprintf( buf1, "DATAACK (ofs=%ld)", hydra_getlong( txbufin ));
             break;

        case HPKT_RPOS:
             sprintf( buf1, "RPOS (pos=%ld%s  blklen=%ld  syncid=%ld)",
                 rxPos, rxPos < 0L ? " Skip" : "",
                 hydra_getlong( txbufin + (LONGx1) ), rxsyncid );
             break;

        case HPKT_EOF:
             sprintf( buf1, "EOF (ofs=%ld%s)",
                 txPos, txPos < 0L ? " Skip" : "" );
             break;

        case HPKT_EOFACK:
             strcat( buf1, "EOFACK" );
             break;

        case HPKT_IDLE:
             strcat( buf1, "IDLE" );
             break;

        case HPKT_END:
             strcat( buf1, "END" );
             break;

        case HPKT_DEVDATA:
             sprintf( buf1, "DEVDATA (id=%ld  dev='%s'  len=%u)",
                 devtxid, devtxdev, devtxlen );
             break;

        case HPKT_DEVDACK:
             sprintf( buf1, "DEVDACK (id=%ld)", hydra_getlong( rxBuf ));
             break;

        default: /* This couldn't possibly happen! ;-) */
             strcat( buf1, "**None**" );
             break;
    }

    buflen = strlen( buf1 );

    s1 = HFS( format );
    s2 = HPS( type );

    DEBUG(('H',2,"txpkt %s%s(crc%d:%08x format=%s  type=%s  len=%d)",
        buflen < 9 ?  buf1 : "",
        buflen < 9 ?  " " : "",
        crc32 ? 32 : 16, crcDebug,
        s1, s2, (word) len - 1));

    /* xfree( s2 );
    xfree( s1 ); */

    if ( buflen > 8 )
        DEBUG(('H',3,"  -> %s", buf1));
    
    if ( buf2[0] )
        DEBUG(('H',3,"  -> %s", buf2));
    DEBUG(('H',4,"now: %08x, rxtimer: %08x, txtimer: %08x, braindead: %d",
        tnow, rxtimer, txtimer, tnow - braindead));
}
#endif

/*---------------------------------------------------------------------------*/
static void txpkt(register word len, int type)
{
    register byte *in, *out;
    register word  c, n;
    boolean crc32 = false;
    byte    format;

#ifdef NEED_DEBUG
    dword crcDebug;
#endif

    tnow = h_timer_get();
    txbufin[len++] = type;

    switch (type) {
        case HPKT_START:
        case HPKT_INIT:
        case HPKT_INITACK:
        case HPKT_END:
        case HPKT_IDLE:
             format = HCHR_HEXPKT;
             break;

        default:
             /* COULD do smart format selection depending on data and options! */
             if (txoptions & HOPT_HIGHBIT) {
                if ((txoptions & HOPT_CTLCHRS) && (txoptions & HOPT_CANUUE))
                   format = HCHR_UUEPKT;
                else if (txoptions & HOPT_CANASC)
                   format = HCHR_ASCPKT;
                else
                   format = HCHR_HEXPKT;
             }
             else
                format = HCHR_BINPKT;
             break;
    }

    if (format != HCHR_HEXPKT && (txoptions & HOPT_CRC32))
        crc32 = true;

    if (crc32) {
        dword crc = (~(crc32block(txbufin,len)));

#ifdef NEED_DEBUG
        crcDebug = crc;
#endif
        hydra_putlong( &txbufin[len], crc );
        len += (LONGx1);
    } else {
        word crc = (word) (~(crc16block(txbufin,len)));

#ifdef NEED_DEBUG
        crcDebug = crc;
#endif
        hydra_putword( &txbufin[len], crc );
        len += sizeof( word );
    }

#ifdef NEED_DEBUG
    debugTransmittedPkt( type, crc32, len, format, crcDebug );
#endif

    in = txbufin;
    out = txBuf;
    txLastc = 0;
    *out++ = H_DLE;
    *out++ = format;

    switch (format) {
        case HCHR_HEXPKT:
             for (; len > 0; len--, in++) {
                 if (*in & 0x80) {
                    *out++ = '\\';
                    *out++ = hexdigitslower[((*in) >> 4) & 0x0f];
                    *out++ = hexdigitslower[(*in) & 0x0f];
                 }
                 else if (*in < 32 || *in == 127) {
                    *out++ = H_DLE;
                    *out++ = (*in) ^ 0x40;
                 }
                 else if (*in == '\\') {
                    *out++ = '\\';
                    *out++ = '\\';
                 }
                 else
                    *out++ = *in;
             }
             break;

        case HCHR_BINPKT:
             for (; len > 0; len--)
                 out = put_binbyte(out,*in++);
             break;

        case HCHR_ASCPKT:
             for (n = c = 0; len > 0; len--) {
                 c |= (word) ((*in++) << n);
                 out = put_binbyte(out,c & 0x7f);
                 c >>= 7;
                 if (++n >= 7) {
                    out = put_binbyte(out,c & 0x7f);
                    n = c = 0;
                 }
             }
             if (n > 0)
                out = put_binbyte(out,c & 0x7f);
             break;

        case HCHR_UUEPKT:
             for ( ; len >= 3; in += 3, len -= (word) 3) {
                 *out++ = h_uuenc(in[0] >> 2);
                 *out++ = h_uuenc(((in[0] << 4) & 0x30) | ((in[1] >> 4) & 0x0f));
                 *out++ = h_uuenc(((in[1] << 2) & 0x3c) | ((in[2] >> 6) & 0x03));
                 *out++ = h_uuenc(in[2] & 0x3f);
             }
             if (len > 0) {
                *out++ = h_uuenc(in[0] >> 2);
                *out++ = h_uuenc(((in[0] << 4) & 0x30) | ((in[1] >> 4) & 0x0f));
                if (len == 2)
                   *out++ = h_uuenc((in[1] << 2) & 0x3c);
             }
             break;
    }

    *out++ = H_DLE;
    *out++ = HCHR_PKTEND;

    if (type != HPKT_DATA && format != HCHR_BINPKT) {
        *out++ = '\r';
        *out++ = '\n';
    }

#if 0
    /* Well be done in real transmit code */
    for (in = (byte *) txpktprefix; *in; in++) {
        switch (*in) {
            case 221: /* transmit break signal for one second */
                      tty_send_break();
                      break;
            
            case 222: /* delay one second before next character */
                      qsleep( 1000 );
                      break;

            case 223: /* transmit a NULL (ASCII 0) character */
                      PUTCHAR(0);
                      break;
            
            default:  PUTCHAR(*in);
                      break;
        }
    }

    PUTBLK( txBuf, (word) (out - txBuf) );
#endif
    
    hytxb.pqueue = xrealloc( hytxb.pqueue, sizeof( HYPKT ) * ( hytxb.npkts + 1 ));
    hytxb.pqueue[hytxb.npkts].len = (out - txBuf);
    hytxb.pqueue[hytxb.npkts].pkt = (byte *) xmalloc( hytxb.pqueue[hytxb.npkts].len );
    memcpy( hytxb.pqueue[hytxb.npkts].pkt, txBuf, hytxb.pqueue[hytxb.npkts].len );
    hytxb.npkts++;
    hytxb.prefix_done = false;

}/*txpkt()*/


static int hydra_txpkt(void)
{
    int rc;
    int i;

    if ( hytxb.tx_left == 0 ) {		/* tx buffer is empty - try to fill it out */
        hytxb.tx_ptr = 0;

        if ( hytxb.npkts ) {			/* there are unsent packets */
            for( i = 0; i < hytxb.npkts; i++ ) {
                if ( hytxb.pqueue[i].pkt ) {
                    if ( hytxb.pqueue[i].len + hytxb.tx_left > hytxb.tx_size )
                        break;

                    memcpy( (void *) (hytxb.tx_buf + hytxb.tx_left),
                        hytxb.pqueue[i].pkt, hytxb.pqueue[i].len );
                    hytxb.tx_left += hytxb.pqueue[i].len;
                    xfree( hytxb.pqueue[i].pkt );
                    DEBUG(('H',4,"put pkt to tx buf, %d", hytxb.pqueue[i].len));
                }
            }

            if ( i >= hytxb.npkts ) {		/* all packets were put in tx buf */
                xfree( hytxb.pqueue );
                hytxb.npkts = 0;
            }
        }
    }

    if ( hytxb.tx_left == 0 )
        return 1;

    if (( rc = tty_write( hytxb.tx_buf + hytxb.tx_ptr, hytxb.tx_left )) < 0 ) {
        if ( rc != TTY_TIMEOUT )
            return rc;
    } else {
        hytxb.tx_left -= rc;
        hytxb.tx_ptr += rc;
        DEBUG(('H',2,"txpkt sent %d",rc));
    }
    return 1;
}


#ifdef NEED_DEBUG
/*---------------------------------------------------------------------------*/
static void debugReceivedPkt(dword crcDebug, int crcType)
{
    char *s1, *s2, *s3, *s4;
    char buf1[1024] = { '\0' };
    char buf2[1024] = { '\0' };
    int buflen;

    switch (rxBuf[rxpktlen]) {
        case HPKT_START:
            strcat( buf1, "START" );
            break;

        case HPKT_INIT:
             s1 = ((char *) rxBuf) + ((int) strlen((char *) rxBuf)) + 1;
             s2 = s1 + ((int) strlen(s1)) + 1;
             s3 = s2 + ((int) strlen(s2)) + 1;
             s4 = s3 + ((int) strlen(s3)) + 1;
             sprintf( buf1, "INIT (appinfo='%s')", (char *) rxBuf );
             sprintf( buf2, "(can='%s'  want='%s'  options='%s'  pktprefix='%s')",
                 s1, s2, s3, s4 );
             break;

        case HPKT_INITACK:
            strcat( buf1, "INITACK" );
            break;

        case HPKT_FINFO:
            sprintf( buf1, "FINFO ('%s'  rxstate=%s)", rxBuf, hydra_rx_state_str( rxState ));
            break;

        case HPKT_FINFOACK:
            sprintf( buf1, "FINFOACK (pos=%ld  txstate=%s)"/*  txfd=%d)"*/,
                hydra_getlong( rxBuf ), hydra_tx_state_str( txState )/*, txfd*/);
            break;

        case HPKT_DATA:
            sprintf( buf1, "DATA (rxstate=%s  pos=%ld  len=%u)",
                hydra_rx_state_str( rxState ), hydra_getlong( rxBuf ),
                (word) (rxpktlen - ((int) (LONGx1))));
            break;

        case HPKT_DATAACK:
            sprintf( buf1, "DATAACK (rxstate=%s  pos=%ld)",
                hydra_rx_state_str( rxState ), hydra_getlong( rxBuf ));
            break;

        case HPKT_RPOS:
            sprintf( buf1, "RPOS (pos=%ld%s  blklen=%u->%ld  syncid=%ld%s  txstate=%s)"/*  txfd=%d)"*/,
                hydra_getlong( rxBuf ),
                hydra_getlong( rxBuf ) < 0L ? " Skip" : "",
                txBlkLen, hydra_getlong( rxBuf + (LONGx1)),
                hydra_getlong( rxBuf + (LONGx2)),
                hydra_getlong( rxBuf + (LONGx2)) == rxsyncid ? " Dup" : "",
                hydra_tx_state_str( txState )/*, txfd*/);
            break;

        case HPKT_EOF:
            sprintf( buf1, "EOF (rxstate=%s  pos=%ld%s)",
                hydra_rx_state_str( rxState ), hydra_getlong( rxBuf ),
                hydra_getlong( rxBuf ) < 0L ? " Skip" : "");
            break;

        case HPKT_EOFACK:
            sprintf( buf1, "EOFACK (txstate=%s)", hydra_tx_state_str( txState ));
            break;

        case HPKT_IDLE:
            strcat( buf1, "IDLE" );
            break;

        case HPKT_END:
            strcat( buf1, "END" );
            break;

        case HPKT_DEVDATA:
            s1 = ((char *) rxBuf) + ((int) (LONGx1));
            sprintf( buf1, "DEVDATA (id=%ld  dev=%s  len=%u",
                hydra_getlong( rxBuf ), s1,
                rxpktlen - (((int) (LONGx1)) + ((int) strlen(s1)) + 1));
            break;

        case HPKT_DEVDACK:
            sprintf( buf1, "DEVDACK (devtxstate=%s  id=%ld)",
                hydra_dpkttx_state_str( devtxstate ),
                hydra_getlong( rxBuf ));
            break;

        default:
            sprintf( buf1, "Unkown pkttype %d (txstate=%s  rxstate=%s)",
                (int) rxBuf[rxpktlen], hydra_tx_state_str( txState ),
                hydra_rx_state_str( rxState ));
            break;
    }

    buflen = strlen( buf1 );

    s1 = HFS( rxpktformat );
    s2 = HPS( rxBuf[rxpktlen] );

    DEBUG(('H',2,"rxpkt %s%s(crc%d:%08x format=%s  type=%s  len=%d)",
        buflen < 9 ?  buf1 : "",
        buflen < 9 ?  " " : "",
        crcType, crcDebug, s1, s2, rxpktlen));

    if ( buflen > 8 )
        DEBUG(('H',3,"  <- %s", buf1));
    
    if ( buf2[0] )
        DEBUG(('H',3,"  <- %s", buf2));

    DEBUG(('H',4,"now: %08x, rxtimer: %08x, txtimer: %08x, braindead: %d",
        tnow, rxtimer, txtimer, tnow - braindead));
}
#endif


/*---------------------------------------------------------------------------*/
static int rxpkt(boolean canread)
{
    register byte *p = rxBufPtr;
    register byte *q = rxBuf;
    register int   c, n, i;
    register dword crc;

#ifdef NEED_DEBUG
    dword crcDebug;
    int crcType;
#endif

    if (( i = hydra_check_timers()) != H_NOPKT )
        return i;

    if ( !canread )
        return H_NOPKT;

    p = rxBufPtr;

    while(( c = GETCHAR( 0 )) >= 0 ) {

        if ( rxoptions & HOPT_HIGHBIT )
            c &= 0x7f;

        n = c;

        if ( rxoptions & HOPT_HIGHCTL )
            n &= 0x7f;

        if ( n != H_DLE &&
            ((( rxoptions & HOPT_XONXOFF ) && ( n == XON || n == XOFF )) ||
             (( rxoptions & HOPT_CTLCHRS ) && ( n < 32 || n == 127 ))))
            continue;

        if ( rxdle || c == H_DLE ) {
            switch (c) {
                case H_DLE:
                    if ( ++rxdle >= 5 )
                        return ( H_CANCEL );
                    break;

                case HCHR_PKTEND:
                    if ( p == NULL )
                        break;

                    rxBufPtr = p;

                    switch( rxpktformat ) {
                        case HCHR_BINPKT:
                            q = rxBufPtr;
                            break;

                        case HCHR_HEXPKT:
                            for( p = q = rxBuf; p < rxBufPtr; p++ ) {
                                if ( *p == '\\' && *++p != '\\' ) {
                                    i = *p;
                                    n = *++p;
                                    if (( i -= '0' ) > 9 ) i -= ( 'a' - ':' );
                                    if (( n -= '0' ) > 9 ) n -= ( 'a' - ':' );
                                    if (( i & ~0x0f ) || ( n & ~0x0f )) {
                                        c = H_NOPKT; 	/*AGL:17aug93*/
                                        break;
                                    }
                                    *q++ = ( i << 4 ) | n;
                                }
                                else
                                    *q++ = *p;
                            }
                            if ( p > rxBufPtr )
                                c = H_NOPKT;
                            break;

                        case HCHR_ASCPKT:
                            n = i = 0;
                            for( p = q = rxBuf; p < rxBufPtr; p++ ) {
                                i |= (( *p & 0x7f ) << n );
                                if (( n += 7 ) >= 8 ) {
                                    *q++ = (byte) ( i & 0xff );
                                    i >>= 8;
                                    n -= 8;
                                }
                            }
                            break;

                        case HCHR_UUEPKT:
                            n = (int) ( rxBufPtr - rxBuf );
                            for( p = q = rxBuf; n >= 4; n -= 4, p += 4 ) {
                                if ( p[0] <= ' ' || p[0] >= 'a' ||
                                     p[1] <= ' ' || p[1] >= 'a' ||
                                     p[2] <= ' ' || p[2] >= 'a' ||	/*AGL:17aug93*/
                                     p[3] <= ' ' || p[3] >= 'a') {	/*AGL:17aug93*/
                                    c = H_NOPKT;
                                    break;
                                }
                                *q++ = (byte) ((h_uudec(p[0]) << 2) | (h_uudec(p[1]) >> 4));
                                *q++ = (byte) ((h_uudec(p[1]) << 4) | (h_uudec(p[2]) >> 2));
                                *q++ = (byte) ((h_uudec(p[2]) << 6) | h_uudec(p[3]));
                            }
                            if ( n >= 2 ) {
                                if ( p[0] <= ' ' || p[0] >= 'a' ||	  /*AGL:17aug93*/
                                     p[1] <= ' ' || p[1] >= 'a') {	  /*AGL:17aug93*/
                                    c = H_NOPKT;
                                    break;
                                }
                                *q++ = (byte) ((h_uudec(p[0]) << 2) | (h_uudec(p[1]) >> 4));
                                if ( n == 3 ) {
                                    if ( p[2] <= ' ' || p[2] >= 'a' ) {  /*AGL:17aug93*/
                                        c = H_NOPKT;
                                        break;
                                    }
                                    *q++ = (byte) ((h_uudec(p[1]) << 4) | (h_uudec(p[2]) >> 2));
                                }
                            }
                            break;

                        default:   /* This'd mean internal fluke */
                            DEBUG(('H',2,"rxpkt <PKTEND> (pktformat=%s dec=%d hex=%02x) Fluke ??",
                                    HFS( rxpktformat ), rxpktformat, rxpktformat));
                            c = H_NOPKT;
                            break;
                    }

                    rxBufPtr = NULL;

                    if (c == H_NOPKT)
                       break;

                    rxpktlen = (word) (q - rxBuf);

                    if (rxpktformat != HCHR_HEXPKT && (rxoptions & HOPT_CRC32)) {
                       if (rxpktlen < 5) {
                          c = H_NOPKT;
                          break;
                       }

                       crc = crc32block(rxBuf,rxpktlen);
#ifdef NEED_DEBUG
                       crcDebug = crc;
                       crcType = 32;
#endif
                       n = h_crc32test( crc );
                       rxpktlen -= (word) (LONGx1);  /* remove CRC-32 */
                    }
                    else {
                       if (rxpktlen < 3) {
                          c = H_NOPKT;
                          break;
                       }

                       crc = crc16block(rxBuf,rxpktlen);
#ifdef NEED_DEBUG
                       crcDebug = crc;
                       crcType = 16;
#endif
                       n = h_crc16test( crc );
                       rxpktlen -= (word) sizeof (word);  /* remove CRC-16 */
                    }

                    rxpktlen--;                     /* remove type  */

#ifdef NEED_DEBUG
                    debugReceivedPkt( crcDebug, crcType );
#endif
                    if (n) {
                       return ((word) rxBuf[rxpktlen]);
                    }/*goodpkt*/

                    DEBUG(('H',3,"Bad CRC (format=%s  type=%s  len=%d)",
                        HFS( rxpktformat ), HPS( rxBuf[rxpktlen] ), rxpktlen));

                    break;

                case HCHR_BINPKT: 
                case HCHR_HEXPKT: 
                case HCHR_ASCPKT: 
                case HCHR_UUEPKT:
                    DEBUG(('H',2,"rxpkt <PKTSTART> (pktformat=%s)", HFS( c )));
                    rxpktformat = c;
                    p = rxBufPtr = rxBuf;
                    rxdle = 0;
                    break;

                default:
                    if (p) {
                        if (p < rxBufMax)
                           *p++ = (byte) (c ^ 0x40);
                        else {
                           DEBUG(('H',2,"rxpkt Pkt too long - discarded"));
                           p = NULL;
                        }
                    }
                    rxdle = 0;
                    break;
            }
        }
        else if (p) {
           if (p < rxBufMax)
              *p++ = (byte) c;
           else {
              DEBUG(('H',2,"rxpkt Pkt too long - discarded"));
              p = NULL;
           }
        }
    }

    if ( c != TTY_TIMEOUT && NOTTO( c ))
        return H_CARRIER;

    rxBufPtr = p;

    return (H_NOPKT);
}/*rxpkt()*/


/*---------------------------------------------------------------------------*/
static void hydra_status(boolean xmit)
{
    check_cps();

    if ( xmit ) {
        sendf.foff = txPos;
        qpfsend();
    } else {
        recvf.foff = rxPos;
        qpfrecv();
    }
}/*hydra_status()*/


/*---------------------------------------------------------------------------*/
void hydra_init(dword want_options, int orig, int hmod)
{
    hydra_modifier = hmod;
    originator = orig;

    memset( &hytxb, 0, sizeof( HYTXB ));
    hytxb.prefix_done = true;

#ifndef HYDRA8K16K
    hytxb.tx_size = H_BUFLEN;
#else
    hytxb.tx_size = ( H_BUFLEN( hydra_modifier ) + 4 );
#endif

    txBuf = (byte *) xmalloc( hytxb.tx_size );
    rxBuf = (byte *) xmalloc( hytxb.tx_size );
    hytxb.tx_buf = (byte *) xmalloc( hytxb.tx_size );

#ifndef HYDRA8K16K
    txbufin  = txBuf + ((H_MAXBLKLEN + H_OVERHEAD + 5) << 1);
    rxBufMax = rxBuf + H_MAXPKTLEN;
#else
    txbufin  = txBuf + (( H_MAXBLKLEN( hydra_modifier ) + H_OVERHEAD + 5 ) << 1 );
    rxBufMax = rxBuf + H_MAXPKTLEN( hydra_modifier );
#endif

    crc16tab = crc16prp_tab;
    crc32tab = crc32_tab;

    batchesdone = 0;

    if (originator)
        hdxlink = false;
    else
        hdxlink = cfgi( CFG_HYDRAHDX ) == 1;

    verboseLog = cfgi( CFG_HYDRALOGVERBOSE );

    hoCAN = HCAN_OPTIONS;
    if ( cfgi( CFG_HYDRACRC16 ) == 1 )
        hoCAN &= ~HOPT_CRC32;

    options = (want_options & hoCAN) & ~HUNN_OPTIONS;

    DEBUG(('H',5,"Want CRC%d", (cfgi( CFG_HYDRACRC16 ) ? 16 : 32)));

    timeOut = (word) (40960L / effbaud);
    if      (timeOut < H_MINTIMER) timeOut = H_MINTIMER;
    else if (timeOut > H_MAXTIMER) timeOut = H_MAXTIMER;

    DEBUG(('H',5,"effbaud: %d", effbaud));
    DEBUG(('H',5,"Timeout: %d", timeOut));

#ifndef HYDRA8K16K

    if ( effbaud >= 2400UL)					/*AGL:09mar94*/
        txMaxBlkLen = H_MAXBLKLEN;				/*AGL:09mar94*/
    else {							/*AGL:09mar94*/
        txMaxBlkLen = (word) ((effbaud / 300) * 128);		/*AGL:09mar94*/
        if ( txMaxBlkLen < 256 ) txMaxBlkLen = 256;		/*AGL:09mar94*/
    }								/*AGL:09mar94*/

    if (is_ip /* || arqlink */) {				/*AGL:14jan95*/
        rxBlkLen = txBlkLen = txMaxBlkLen;
        txgoodneeded = 0;
    }
    else {
        rxBlkLen = txBlkLen = (word) ((effbaud < 2400UL) ? 256 : 512);
        txgoodneeded = txMaxBlkLen;				/*AGL:23feb93*/
    }

#else

    if ( hydra_modifier == 1 ) {
        if ( effbaud >= 2400UL) {
            txMaxBlkLen = H_MAXBLKLEN( 1 );
        } else {
            txMaxBlkLen = (word) ((effbaud / 300) * 128);
            if ( txMaxBlkLen < 256 ) txMaxBlkLen = 256;
        }

        if (is_ip /* || arqlink */) {				/*AGL:14jan95*/
           rxBlkLen = txBlkLen = txMaxBlkLen;
           txgoodneeded = 0;
        } else {
           rxBlkLen = txBlkLen = (word) ((effbaud < 2400UL) ? 256 : 512);
           txgoodneeded = txMaxBlkLen;				/*AGL:23feb93*/
        }

    } else {
        txMaxBlkLen = H_MAXBLKLEN( hydra_modifier );
        rxBlkLen = txBlkLen = txMaxBlkLen >> 2;
        txgoodneeded = 1024;
    }

#endif

    txgoodbytes  = 0;

#ifdef HYDRA8K16K
    DEBUG(('H',5,"hydra_modifier: %d", hydra_modifier));
    DEBUG(('H',5,"H_MAXBLKLEN: %d",H_MAXBLKLEN(hydra_modifier)));
    DEBUG(('H',5,"H_MAXPKTLEN: %d",H_MAXPKTLEN(hydra_modifier)));
    DEBUG(('H',5,"H_BUFLEN: %d",H_BUFLEN(hydra_modifier)));
#endif
    DEBUG(('H',5,"rxBlkLen: %d", rxBlkLen));
    DEBUG(('H',5,"txMaxBlkLen: %d", txMaxBlkLen));
    DEBUG(('H',5,"txgoodneeded: %d", txgoodneeded));

    txState = HTX_DONE;
    hydra_rxwindow = (dword) cfgi( CFG_HRXWIN );
    hydra_txwindow = (dword) cfgi( CFG_HTXWIN );

    hydra_devfunc( "CON", ( rnode->opt & MO_CHAT ? hydra_chat : NULL ));

    write_log("Hydra: %s-directional mode session", hdxlink ? "Uni" : "Bi" );

    /* chattimer = -1L; */

}/*hydra_init()*/


/*---------------------------------------------------------------------------*/
void hydra_deinit(void)
{
    int i;

    qpreset( 0 );
    qpreset( 1 );
    for( i = 0; i < hytxb.npkts; i++ )
        if ( hytxb.pqueue[i].pkt )
            xfree( hytxb.pqueue[i].pkt );
    xfree( hytxb.pqueue );
    xfree( hytxb.tx_buf );
    xfree( rxBuf );
    xfree( txBuf );

}/*hydra_deinit()*/


/*---------------------------------------------------------------------------*/
int hydra_send(char *txpathname, char *txalias)
{
    int    res = XFER_OK;
    int    i, pkttype, rc;
    char   *p, *q;
    struct stat statf;
    boolean rd, wd;
    struct timeval tv;

    DEBUG(('H',1,"hydra_batch: %s, %s", txpathname, txalias));

    rxstatus = 0;

    /*-------------------------------------------------------------------*/
    if (txState == HTX_DONE) {
        txState        = HTX_START;
        txoptions      = HTXI_OPTIONS;
        txpktprefix[0] = '\0';

        rxState    = HRX_INIT;
        rxoptions  = HRXI_OPTIONS;
        rxfd       = NULL;
        rxdle      = 0;
        rxBufPtr   = NULL;
        rxtimer    = h_timer_reset();
        devtxid    = devrxid = 0L;
        devtxtimer = h_timer_reset();
        devtxstate = HTD_DONE;

        braindead  = h_timer_set(H_BRAINDEAD);
    }
    else
        txState = HTX_FINFO;

    txtimer   = h_timer_reset();
    txretries = 0;

    /*-------------------------------------------------------------------*/
    if ( txpathname ) {
        if ( !( txfd = txopen( txpathname, txalias )))
            return XFER_SKIP;

        txStart  = 0L;
        txsyncid = 0L;
    }
    /*
    else {
        txfd = NULL;
        xstrcpy( txfname, "", 2 );
    }
    */

    /*-------------------------------------------------------------------*/
    do {
        /*----------------------------------------------------------------*/
        switch (devtxstate) {
            /*---------------------------------------------------------*/
            case HTD_DATA:
                 if (txState > HTX_RINIT) {
                    hydra_putlong( txbufin, (dword) devtxid );
                    p = ((char *) txbufin + (LONGx1));
#ifndef HYDRA8K16K
                    xstrcpy( p, devtxdev, H_BUFLEN );
#else
                    xstrcpy( p, devtxdev, 1020 );
#endif
                    p += H_FLAGLEN + 1;
                    memcpy(p,devtxbuf,devtxlen);
                    txpkt((word) ((LONGx1) + H_FLAGLEN + 1 + devtxlen), HPKT_DEVDATA);
                    devtxtimer = h_timer_set((!rxState && txState == HTX_REND) ?
                         timeOut >> 1 : 
                         timeOut); /*AGL:10mar93*/
                    devtxstate = HTD_DACK;
                 }
                 break;

            /*---------------------------------------------------------*/
            default:
                 break;

            /*---------------------------------------------------------*/
        }

        /*----------------------------------------------------------------*/
        switch (txState) {
            /*---------------------------------------------------------*/
            case HTX_START:
                 PUTBLK((byte *) autostr,(int) strlen(autostr));
                 txpkt((word) 0,HPKT_START );
                 txtimer = h_timer_set(H_START);
                 txState = HTX_SWAIT;
                 break;

            /*---------------------------------------------------------*/
            case HTX_INIT:
                 p = (char *) txbufin;
                 snprintf(p,256,"%08lx%s,%s %s",
                     H_REVSTAMP, progname, version, osname);
                 p += ((int) strlen(p)) + 1; /* our app info & HYDRA rev. */
                 put_flags(p,h_flags,hoCAN);           /* what we CAN  */
                 
                 p += ((int) strlen(p)) + 1;
                 put_flags(p,h_flags,options);         /* what we WANT */
                 p += ((int) strlen(p)) + 1;
                 sprintf(p,"%08lx%08lx",               /* TxRx windows */
                     hydra_txwindow,hydra_rxwindow);
                 p += ((int) strlen(p)) + 1;
                 strcpy(p,pktprefix);     /* pkt prefix string we want */
                 p += ((int) strlen(p)) + 1;

                 txoptions = HTXI_OPTIONS;
                 txpkt((word) ((byte *) p - (byte *) txbufin), HPKT_INIT );
                 txoptions = rxoptions;
                 txtimer = h_timer_set(timeOut >> 1);
                 txState = HTX_INITACK;
                 break;

            /*---------------------------------------------------------*/
            case HTX_FINFO:
                 if (txfd) {
                     if (!txretries) {
                         DEBUG(('H',1,"HSend: %s%s%s (%ldb), %d min.",
                                txpathname, txalias ? " -> " : "",
                                txalias ? txalias : "",
                                (dword) sendf.ftot,
                                (int) (txfsize * 10L / effbaud + 27L) / 54L));
                     }
                     snprintf((char *) txbufin, 1024, "%08lx%08lx%08lx%08lx%08lx%s",
                         (dword) sendf.mtime, (dword) sendf.ftot,
                         0L, 0L, 0L, fnc( sendf.fname ));
                         /* we shall support HOPT_NFI
                         ( sendf.nf == 1 ) ? sendf.allf : sendf.nf, fnc( sendf.fname ));
                         */
                     strlwr((char *) txbufin + 40 );
                     i = strlen((char *) txbufin ) + 1;
                     if ( sendf.fname ) {
                         strcpy( (char *) txbufin + i, sendf.fname );
                         i += strlen( sendf.fname ) + 1;
                     }
                 }
                 else {
                     if (!txretries) {
                         DEBUG(('H',1,"HSend: End of batch"));
                         qpreset(1);
                     }
                     *txbufin = '\0';
                     i = 1;
                 }
                 txpkt((word) i, HPKT_FINFO );
                 txtimer = h_timer_set(txretries ? timeOut >> 1 : timeOut);
                 txState = HTX_FINFOACK;
                 break;

            /*---------------------------------------------------------*/
            case HTX_XDATA:
                 if ( hytxb.npkts )
                     break;

                 if (txPos < 0L)
                     i = -1;                                    /* Skip */
                 else {
                     hydra_putlong( txbufin, (dword) txPos );
                     if ((i = fread( txbufin + 4, 1, txBlkLen, txfd )) < 0) {
                         DEBUG(('H',1,"HSend: File read error"));
                         sline("HSend: File read error");
                         txclose( &txfd, FOP_ERROR );
                         txPos = H_SUSPEND;                     /* Skip -2L */
                     }
                 }

                 if (i > 0) {
                     txPos += i;
                     txpkt((word) (LONGx1 + i), HPKT_DATA);

                     if (txBlkLen < txMaxBlkLen &&
                         (txgoodbytes += (word) i) >= txgoodneeded) {

                         txBlkLen <<= 1;
                         if (txBlkLen >= txMaxBlkLen) {
                             txBlkLen = txMaxBlkLen;
                             txgoodneeded = 0;
                         }
                         txgoodbytes = 0;
                     }

                     if (txwindow && (txPos >= (txlastack + txwindow))) {
                         txtimer = h_timer_set(txretries ? timeOut >> 1 : timeOut);
                         txState = HTX_DATAACK;
                     }

                     if (!txStart)
                         txStart = time(NULL);
                     hydra_status(true);
                     break;
                 }

                 /* fallthrough to HTX_EOF */

            /*---------------------------------------------------------*/
            case HTX_EOF:
                 hydra_putlong( txbufin, (dword) txPos );
                 txpkt((word) LONGx1, HPKT_EOF);
                 txtimer = h_timer_set(txretries ? timeOut >> 1 : timeOut);
                 txState = HTX_EOFACK;
                 break;

            /*---------------------------------------------------------*/
            case HTX_END:
                 txpkt((word) 0, HPKT_END);
                 txpkt((word) 0, HPKT_END);
                 txtimer = h_timer_set(timeOut >> 1);
                 txState = HTX_ENDACK;
                 break;

            /*---------------------------------------------------------*/
            default:
                 break;

            /*---------------------------------------------------------*/
        }

        /*----------------------------------------------------------------*/
        /* while (txState && (pkttype = rxpkt()) != H_NOPKT) { */
        /*----------------------------------------------------------*/

        rd = !tty_hasinbuf();
        wd = (hytxb.tx_left || hytxb.npkts);
        DEBUG(('H',4,"rxstate=%s, txstate=%s", hydra_rx_state_str( rxState ), hydra_tx_state_str( txState )));

        getevt();

        if (( rd || wd ) && !tty_gothup ) {
            tv.tv_sec = hydra_get_timeout();
            tv.tv_usec = 0;

            DEBUG(('H',2,"select: r=%d, w=%d, sec=%d, usec=%d", rd, wd, tv.tv_sec, tv.tv_usec));
            rc = tty_select( &rd, &wd, &tv );
            DEBUG(('H',2,"select done: %d (r=%d, w=%d)", rc, rd, wd));
        }

        rd = rd || tty_hasinbuf();

        if ( wd && !tty_gothup )
            hydra_txpkt();

        pkttype = rxpkt( rd );

        DEBUG(('H',4,"rxstate=%s, txstate=%s, pkttype=%d",
            hydra_rx_state_str( rxState ),
            hydra_tx_state_str( txState ),
            pkttype ));

        /*----------------------------------------------------------*/
        switch (pkttype) {
            /*---------------------------------------------------*/
            case H_CARRIER:
            case H_CANCEL:
            case H_SYSABORT:
            case H_BRAINTIME:
                 switch (pkttype) {
                     case H_CARRIER:   p = "Carrier lost";          break;
                     case H_CANCEL:    p = "Aborted by other side"; break;
                     case H_SYSABORT:  p = "Aborted by operator";   break;
                     case H_BRAINTIME: p = "Other end died";        break;
                 }
                 write_log( "Hydra: %s",p );
                 txState = HTX_DONE;
                 res = XFER_ABORT;
                 break;

            /*---------------------------------------------------*/
            case H_TXTIME:
                 if (txState == HTX_XWAIT || txState == HTX_REND) {
                    txpkt((word) 0, HPKT_IDLE);
                    txtimer = h_timer_set(H_IDLE);
                    break;
                 }

                 if (++txretries > H_RETRIES) {
                     write_log( "Hydra: Too many errors" );
                     txState = HTX_DONE;
                     res = XFER_ABORT;
                     break;
                 }

                 DEBUG(('H',1,"Hydra: Timeout - Retry %u", txretries));
                 sline( "Hydra: Timeout - Retry %u", txretries );

                 txtimer = h_timer_reset();

                 switch (txState) {
                     case HTX_SWAIT:    txState = HTX_START; break;
                     case HTX_INITACK:  txState = HTX_INIT;  break;
                     case HTX_FINFOACK: txState = HTX_FINFO; break;
                     case HTX_DATAACK:  txState = HTX_XDATA; break;
                     case HTX_EOFACK:   txState = HTX_EOF;   break;
                     case HTX_ENDACK:   txState = HTX_END;   break;
                 }
                 break;

            /*---------------------------------------------------*/
            case H_DEVTXTIME:
                 if (++devtxretries > H_RETRIES) {
                     write_log( "HydraDevTX: Too many errors");
                     txState = HTX_DONE;
                     res = XFER_ABORT;
                     break;
                 }

                 DEBUG(('H',1,"HydraDevTX: Timeout - Retry %u", devtxretries));
                 sline( "HydraDevTX: Timeout - Retry %u", devtxretries );

                 devtxtimer = h_timer_reset();
                 devtxstate = HTD_DATA;

                 DEBUG(('H',4,"devtxtimer: %d", devtxtimer));

                 break;

            /*---------------------------------------------------*/
            case HPKT_START:
                 if (txState == HTX_START || txState == HTX_SWAIT) {
                     txtimer = h_timer_reset();
                     txretries = 0;
                     txState = HTX_INIT;
                     braindead = h_timer_set(H_BRAINDEAD);
                 }
                 break;

            /*---------------------------------------------------*/
            case HPKT_INIT:
                 if (rxState == HRX_INIT) {
                     word rxtx;

                     p = (char *) rxBuf;
                     p += strlen(p) + 1;
                     q = p + strlen(p) + 1;
                     rxtx = (char *) q - (char *) rxBuf + strlen(q) + 1;

                     rxoptions  = options | HUNN_OPTIONS;
                     rxoptions |= get_flags(q,h_flags);
                     rxoptions &= get_flags(p,h_flags);
                     rxoptions &= hoCAN;
                     if (rxoptions < (options & HNEC_OPTIONS)) {
                        write_log( "Hydra: Incompatible on this link" );
                        txState = HTX_DONE;
                        res = XFER_ABORT;
                        break;
                     }

                     p = (char *) rxBuf + rxtx;
                     DEBUG(('H',5,"p='%s'", p));

                     rxwindow = txwindow = 0L;
                     sscanf(p,"%08lx%08lx", (dword *) &rxwindow, (dword *) &txwindow);
                     
                     DEBUG(('H',5,"rxwindow: %ld", rxwindow));
                     DEBUG(('H',5,"txwindow: %ld", txwindow));
                     DEBUG(('H',5,"hydra_rxwindow: %ld", hydra_rxwindow));
                     DEBUG(('H',5,"hydra_txwindow: %ld", hydra_txwindow));
                     
                     if ( rxwindow < 0L )
                         rxwindow = 0L;
                     
                     if ( hydra_rxwindow && ( !rxwindow || hydra_rxwindow < rxwindow ))
                         rxwindow = hydra_rxwindow;

                     if ( txwindow < 0L)
                         txwindow = 0L;
                     
                     if ( hydra_txwindow && ( !txwindow || hydra_txwindow < txwindow ))
                         txwindow = hydra_txwindow;

                     DEBUG(('H',5,"rxwindow: %ld", rxwindow));
                     DEBUG(('H',5,"txwindow: %ld", txwindow));

                     p += ((int) strlen(p)) + 1;
                     xstrcpy( txpktprefix, p, H_PKTPREFIX );
                     txpktprefix[H_PKTPREFIX] = '\0';

                     if ( !batchesdone ) {
                         long revstamp;

                         p = (char *) rxBuf;
                         sscanf(p,"%08lx", (dword *) &revstamp);
                         p += 8;
                         
                         DEBUG(('H',1,"Hydra: Remote HydraRev='%s'", h_revdate(revstamp)));

                         if (( q = strchr( p, ',' )) != NULL ) {
                             *q = ' ';
                             if (( q = strchr( q + 1, ' ' )) != NULL )
                                 *q = '/';
                         }

                         DEBUG(('H',1,"Hydra: Remote App.Info='%s'", p));

                         put_flags( (char *) rxBuf, h_flags, rxoptions );

                         if ( txwindow || rxwindow )
                             write_log("Hydra: Using link options '%s', Window [%ld/%ld]",
                                 rxBuf, txwindow, rxwindow);
                         else
                             write_log("Hydra: Using link options '%s'", rxBuf);
                     }

                     chattimer = (rxoptions & HOPT_DEVICE) ? 0L : 1L;

                     txoptions = rxoptions;
                     rxState = HRX_FINFO;
                 }

                 txpkt((word) 0, HPKT_INITACK );
                 break;

            /*---------------------------------------------------*/
            case HPKT_INITACK:
                 if (txState == HTX_INIT || txState == HTX_INITACK) {
                     braindead = h_timer_set(H_BRAINDEAD);
                     txtimer = h_timer_reset();
                     txretries = 0;
                     txState = HTX_RINIT;
                 }
                 break;

            /*---------------------------------------------------*/
            case HPKT_FINFO:
                 if (rxState == HRX_FINFO) {
                     braindead = h_timer_set(H_BRAINDEAD);
                     if ( !rxBuf[0] ) {
                         DEBUG(('H',1,"HRecv: End of batch"));
                         qpreset( 0 );
                         rxPos = 0L;
                         rxState = HRX_DONE;
                         batchesdone++;
                     }
                     else {
                         dword count;

                         rxfsize = rxftime = 0L;
                         sscanf((char *) rxBuf, "%08lx%08lx", &rxftime, (dword *) &rxfsize);
                         sscanf((char *) rxBuf + 32, "%08lx", &count);

                         if ( !recvf.allf && (int) count)
                             recvf.allf = (int) count;

                         p = (char *) rxBuf + 40;

                         DEBUG(('H',1,"HRecv: Get SFN: %s", p));
                         
                         if ( strlen( p ) + 41 < rxpktlen ) {
                             p += strlen( p ) + 1;
                             DEBUG(('H',1,"HRecv: Get LFN: %s", p));
                         } else {
                             DEBUG(('H',1,"HRecv: No LFN present"));
                         }

                         switch( rxopen(p, rxftime, rxfsize, &rxfd )) {
                             case FOP_SKIP:
                                 rxPos = H_SKIP;
                                 break;

                             case FOP_SUSPEND:
                                 rxPos = H_SUSPEND;
                                 break;

                             case FOP_CONT:
                             case FOP_OK:
                                 rxoffset = rxPos = ftell(rxfd);
                                 /* rxStart = 0L; */
                                 rxtimer = h_timer_reset();
                                 rxretries = 0;
                                 rxlastsync = 0L;
                                 rxsyncid = 0L;
                                 hydra_status(false);
                                 rxState = HRX_DATA;
                                 break;
                         }

                         qpfrecv();
                     }
                 }
                 else if (rxState == HRX_DONE)
                     rxPos = (!rxBuf[0]) ? 0L : H_SUSPEND /* -2L */;

                 hydra_putlong( txbufin, (dword) rxPos );
                 txpkt((word) LONGx1, HPKT_FINFOACK);
                 break;

            /*---------------------------------------------------*/
            case HPKT_FINFOACK:
                 if (txState == HTX_FINFO || txState == HTX_FINFOACK) {
                     braindead = h_timer_set(H_BRAINDEAD);
                     txretries = 0;
                     if ( !txfd ) {
                         txtimer = h_timer_set(H_IDLE);
                         txState = HTX_REND;
                     }
                     else {
                         txtimer = h_timer_reset();
                         txPos = hydra_getlong( rxBuf );

                         if ( txPos < 0L ) {
                             switch( txPos ) {
                                 case H_SKIP:
                                     txclose( &txfd, FOP_SKIP );
                                     return XFER_SKIP;

                                 case H_SUSPEND:
                                     txclose( &txfd, FOP_SUSPEND );
                                     return XFER_SUSPEND;

                                 default:
                                     txclose( &txfd, FOP_ERROR );
                                     return XFER_ABORT;
                             }
                         } else { /* txPos >= 0L */
                             txStart = 0L;
                             txoffset = txPos;
                             txlastack = txPos;
                             hydra_status(true);
                             
                             if ( txPos > 0L ) {
                                 if ( fseek( txfd, txPos, SEEK_SET ) < 0L ) {
                                     txclose( &txfd, FOP_ERROR );
                                     txfd = NULL;
                                     txPos = H_SUSPEND;
                                     txState = HTX_EOF;
                                     break;
                                 }
                                 sendf.soff = txPos;
                             }
                             txState = HTX_XDATA;
                         }
                     }
                 }
                 break;

            /*---------------------------------------------------*/
            case HPKT_DATA:
                 if (rxState == HRX_DATA) {
                     long gotpos = hydra_getlong( rxBuf );

                     if ( rxstatus ) {
                         rxPos = ( rxstatus == RX_SUSPEND ) ? H_SUSPEND : H_SKIP;
                         rxclose( &rxfd, ( rxstatus == RX_SUSPEND ) ?
                             FOP_SUSPEND : FOP_SKIP );
                         rxretries = 1;
                         rxsyncid++;
                         hydra_putlong( txbufin, (dword) rxPos );
                         hydra_putlong( txbufin + (LONGx1), (dword) 0L );
                         hydra_putlong( txbufin + (LONGx2), (dword) rxsyncid );
                         txpkt((word) LONGx3, HPKT_RPOS );
                         rxtimer = h_timer_set( timeOut );
                         break;
                     }

                     if ( gotpos != rxPos || gotpos < 0L ) {
                         DEBUG(('H',1,"Hydra: bad pos rx/want %ld/%ld", gotpos, rxPos ));

                         if ( gotpos <= rxlastsync) {
                             rxtimer = h_timer_reset();
                             rxretries = 0;
                         }
                         rxlastsync = gotpos;

                         if (!h_timer_running(rxtimer) || 
                             h_timer_expired_agl(rxtimer, h_timer_get())) {

#ifdef NEED_DEBUG
                             word oBlkLen = rxBlkLen;
#endif

                             if (rxretries > 4) {
                                 if (txState < HTX_REND && !originator && !hdxlink) {
                                     hdxlink = true;
                                     rxretries = 0;
                                 }
                             }
                             if (++rxretries > H_RETRIES) {
                                 write_log( "HRecv: Too many errors");
                                 txState = HTX_DONE;
                                 res = XFER_ABORT;
                                 break;
                             }
                             if ( rxretries == 1 || rxretries == 4 )
                                 rxsyncid++;

                             rxBlkLen >>= 1;
                             i = hydra_adjust_blklen( rxBlkLen );

                             DEBUG(('H',1,"HRecv: Bad pkt at %ld - Retry %u (oldblklen=%u, newblklen=%u)",
                                 rxPos, rxretries, oBlkLen, i));
                             sline("HRecv: Bad pkt at %ld - Retry %u (newblklen=%u)",
                                 rxPos,rxretries,i);
                             
                             hydra_putlong( txbufin, (dword) rxPos );
                             hydra_putlong( txbufin + (LONGx1), (dword) i );
                             hydra_putlong( txbufin + (LONGx2), (dword) rxsyncid );
                             txpkt((word) LONGx3, HPKT_RPOS);
                             rxtimer = h_timer_set( timeOut );
                         }
                     } else {
                         char tmp[MAX_PATH];

                         braindead = h_timer_set(H_BRAINDEAD);
                         rxpktlen -= (word) (LONGx1);
                         rxBlkLen = rxpktlen;

                         snprintf(tmp, MAX_PATH, "%s/tmp/%s", cfgs(CFG_INBOUND), recvf.fname);
                         if ( stat( tmp, &statf ) && errno == ENOENT ) {
                             rxclose( &rxfd, FOP_SKIP );
                             rxPos = H_SKIP;
                             rxretries = 1;
                             rxsyncid++;
                             hydra_putlong( txbufin, (dword) rxPos );
                             hydra_putlong( txbufin + (LONGx1), (dword) 0L );
                             hydra_putlong( txbufin + (LONGx2), (dword) rxsyncid );
                             txpkt((word) LONGx3, HPKT_RPOS);
                             rxtimer = h_timer_set(timeOut);
                             break;
                         }

                         if ( fwrite( rxBuf + ((int) (LONGx1)), rxpktlen, 1, rxfd ) != 1) {
                             DEBUG(('H',4,"HRecv: File write error"));
                             sline("HRecv: File write error");

                             rxclose( &rxfd, FOP_ERROR );
                             rxPos = H_SUSPEND;
                             rxretries = 1;
                             rxsyncid++;
                             hydra_putlong( txbufin, (dword) rxPos );
                             hydra_putlong( txbufin + (LONGx1), (dword) 0L );
                             hydra_putlong( txbufin + (LONGx2), (dword) rxsyncid );
                             txpkt((word) LONGx3, HPKT_RPOS);
                             rxtimer = h_timer_set(timeOut);
                             break;
                         }
                         rxretries = 0;
                         rxtimer = h_timer_reset();
                         rxlastsync = rxPos;
                         rxPos += rxpktlen;
                         if (rxwindow) {
                             hydra_putlong( txbufin, (dword) rxPos );
                             txpkt((word) LONGx1, HPKT_DATAACK);
                         }
                         /*
                         if (!rxStart)
                            rxStart = time(NULL) - ((rxpktlen * 10) / effbaud);
                         */
                         hydra_status(false);
                     }/*badpkt*/
                 }/*rxState==HRX_DATA*/
                 break;

            /*---------------------------------------------------*/
            case HPKT_DATAACK:
  	       if (txState == HTX_XDATA || txState == HTX_DATAACK || /*AGL:06jan94*/
                   txState == HTX_XWAIT ||			     /*AGL:06jan94*/
                   txState == HTX_EOF || txState == HTX_EOFACK) {    /*AGL:06jan94*/
                    
                    long txla = hydra_getlong( rxBuf );
                    
                    if (txwindow && txla > txlastack) {
                       txlastack = txla;
                       if (txState == HTX_DATAACK &&
                           (txPos < (txlastack + txwindow))) {
                          txState = HTX_XDATA;
                          txretries = 0;
                          txtimer = h_timer_reset();
                       }
                    }
                 }
                 break;

            /*---------------------------------------------------*/
            case HPKT_RPOS:
                 if (txState == HTX_XDATA || txState == HTX_DATAACK ||	/*AGL:06jan94*/
                     txState == HTX_XWAIT ||				/*AGL:06jan94*/
                     txState == HTX_EOF || txState == HTX_EOFACK) {	/*AGL:06jan94*/

                     long txsid = hydra_getlong( rxBuf + (LONGx2));

#ifdef NEED_DEBUG
                     word oBlkLen;
#endif
                     if ( txsid != txsyncid) {
                         txsyncid = txsid;
                         txretries = 1;
                     }					/*AGL:14may93*/
                     else {				/*AGL:14may93*/
                         if (++txretries > H_RETRIES) {
                             write_log("HSend: too many errors");
                             txState = HTX_DONE;
                             res = XFER_ABORT;
                             break;
                         }
                         if (txretries != 4) break; 	/*AGL:14may93*/
                     }

                     txtimer = h_timer_reset();
                     txPos = hydra_getlong( rxBuf );
                     if (txPos < 0L) {
                         if ( txfd ) {
                             DEBUG(('H',1,"HSend: %s %s",
                                 (txPos == H_SUSPEND) ? "Suspending" :
                                 (txPos == H_SKIP) ? "Skipping": "Strange skipping",
                                 sendf.fname));
                             sline("HSend: %s %s",
                                 (txPos == H_SUSPEND) ? "Suspending" :
                                 (txPos == H_SKIP) ? "Skipping": "Strange skipping",
                                 sendf.fname);

                             txclose( &txfd, txPos == H_SUSPEND ? FOP_SUSPEND: FOP_SKIP );
                             txfd = NULL;
                             txState = HTX_EOF;
                         }
                         txPos = (txPos == H_SKIP ? H_SKIP : H_SUSPEND);
                         /* txPos = -2L; */
                         break;
                     }

                     txsid = hydra_getlong( rxBuf + (LONGx1));

#ifdef NEED_DEBUG
                     oBlkLen = txBlkLen;
#endif
                     if (txBlkLen > (word) txsid )
                         txBlkLen = (word) txsid;
                     else
                         txBlkLen >>= 1;
                     txBlkLen = hydra_adjust_blklen( txBlkLen );
                     txgoodbytes = 0;
                     txgoodneeded += (word) (txMaxBlkLen << 1);		/*AGL:23feb93*/
                     if (txgoodneeded > txMaxBlkLen << 3)		/*AGL:23feb93*/
                         txgoodneeded = (word) (txMaxBlkLen << 3);	/*AGL:23feb93*/

                     hydra_status(true);
                     sline("HSend: Resending from offset %ld (newblklen=%u)",txPos,txBlkLen);

                     DEBUG(('H',1,"HSend: Resending from offset %ld (oldblklen: %u, newblklen=%u)",
                         txPos, oBlkLen, txBlkLen));

                     if ( fseek(txfd, txPos, SEEK_SET) < 0L ) {
                         txclose(&txfd, FOP_ERROR);
                         txfd = NULL;
                         txPos = H_SUSPEND;
                         txState = HTX_EOF;
                         break;
                     }

                     if (txState != HTX_XWAIT)
                         txState = HTX_XDATA;
                 }
                 break;

            /*---------------------------------------------------*/
            case HPKT_EOF:
                 if (rxState == HRX_DATA) {

                     long foffset = hydra_getlong( rxBuf );

                     if ( foffset < 0L) {
                         rxclose( &rxfd, FOP_SKIP );
                         rxState = HRX_FINFO;
                         braindead = h_timer_set(H_BRAINDEAD);
                     } else if ( foffset != rxPos) {
                         if ( foffset <= rxlastsync ) {
                             rxtimer = h_timer_reset();
                             rxretries = 0;
                         }

                         rxlastsync = foffset;

                         if ( !h_timer_running(rxtimer) || 
                             h_timer_expired_agl(rxtimer, h_timer_get())) {

                             if (++rxretries > H_RETRIES) {
                                 write_log("HRecv: Too many errors");
                                 txState = HTX_DONE;
                                 res = XFER_ABORT;
                                 break;
                             }

                             if (rxretries == 1 || rxretries == 4)  /*AGL:14may93*/
                                 rxsyncid++;

                             rxBlkLen >>= 1;
                             i = hydra_adjust_blklen( rxBlkLen );
                             DEBUG(('H',1,"HRecv: Bad EOF at %ld - Retry %u (newblklen=%u)",rxPos,rxretries,i));
                             sline("HRecv: Bad EOF at %ld - Retry %u (newblklen=%u)", rxPos, rxretries, i);

                             hydra_putlong( txbufin, (dword) rxPos );
                             hydra_putlong( txbufin + (LONGx1), (dword) i );
                             hydra_putlong( txbufin + (LONGx2), (dword) rxsyncid );
                             txpkt((word) LONGx3, HPKT_RPOS);
                             rxtimer = h_timer_set(timeOut);
                         }
                     } else {
                         rxfsize = rxPos;
                         rxclose( &rxfd, FOP_OK );
                         rxfd = NULL;
                         hydra_status(false);
                         rxState = HRX_FINFO;
                         braindead = h_timer_set(H_BRAINDEAD);
                     }/*skip/badeof/eof*/
                 }/*rxState==HRX_DATA*/

                 if (rxState == HRX_FINFO)
                     txpkt((word) 0, HPKT_EOFACK );
                 break;

            /*---------------------------------------------------*/
            case HPKT_EOFACK:
                 if (txState == HTX_EOF || txState == HTX_EOFACK) {
                     braindead = h_timer_set(H_BRAINDEAD);
                     if ( txfd ) {
                         txfsize = txPos;
                         txclose( &txfd, FOP_OK );
                         return (XFER_OK);
                     } else
                         return (txPos == H_SUSPEND ? XFER_SUSPEND : XFER_SKIP);
                 }
                 break;

            /*---------------------------------------------------*/
            case HPKT_IDLE:
                 if (txState == HTX_XWAIT) {
                     hdxlink = false;
                     txtimer = h_timer_reset();
                     txretries = 0;
                     txState = HTX_XDATA;
                 }
                 else if (txState >= HTX_FINFO && txState < HTX_REND)
                     braindead = h_timer_set(H_BRAINDEAD);
                 break;

            /*---------------------------------------------------*/
            case HPKT_END:
                 /* special for chat, other side wants to quit */
                 /*
                 if (chattimer > 0L && txState == HTX_REND) {
                     chattimer = -3L;
                 */
                 if ( chattimer > 1L && txState == HTX_REND ) {
                     chattimer = 0L;
                     break;
                 }

                 if (txState == HTX_END || txState == HTX_ENDACK) {
                     txpkt((word) 0, HPKT_END);
                     txpkt((word) 0, HPKT_END);
                     txpkt((word) 0, HPKT_END);
                     
                     txState = HTX_DONE;
                     res = XFER_OK;
                 }
                 break;

            /*---------------------------------------------------*/
            case HPKT_DEVDATA:
                 if (devrxid != hydra_getlong( rxBuf )) {
                     hydra_devrecv();
                     devrxid = hydra_getlong( rxBuf );
                 }
                 hydra_putlong( txbufin, hydra_getlong( rxBuf ));
                 txpkt((word) LONGx1, HPKT_DEVDACK );
                 break;

            /*---------------------------------------------------*/
            case HPKT_DEVDACK:
                 if (devtxstate && (devtxid == hydra_getlong( rxBuf ))) {
                     devtxtimer = h_timer_reset();
                     devtxstate = HTD_DONE;
                 }
                 break;

            /*---------------------------------------------------*/
            default:  /* unknown packet types: IGNORE, no error! */
                 break;

            /*---------------------------------------------------*/
        }/*switch(pkttype)*/

        /*----------------------------------------------------------*/
        switch (txState) {
            /*---------------------------------------------------*/
            case HTX_START:
            case HTX_SWAIT:
                 if (rxState == HRX_FINFO) {
                     txtimer = h_timer_reset();
                     txretries = 0;
                     txState = HTX_INIT;
                 }
                 break;

            /*---------------------------------------------------*/
            case HTX_RINIT:
                 if (rxState == HRX_FINFO) {
                     txtimer = h_timer_reset();
                     txretries = 0;
                     txState = HTX_FINFO;
                 }
                 break;

            /*---------------------------------------------------*/
            case HTX_XDATA:
                 if (rxState && hdxlink) {
                     write_log("Hydra: %s",hdxmsg);
                     hydra_devsend("MSG",(byte *) hdxmsg,(int) strlen(hdxmsg));

                     txtimer = h_timer_set(H_IDLE);
                     txState = HTX_XWAIT;
                 }
                 break;

            /*---------------------------------------------------*/
            case HTX_XWAIT:
                 if (!rxState) {
                     txtimer = h_timer_reset();
                     txretries = 0;
                     txState = HTX_XDATA;
                 }
                 break;

            /*---------------------------------------------------*/
            case HTX_REND:
                 if (!rxState && !devtxstate) {
                     /* special for chat, braindead will protect */
                     /*
                     if (chattimer > 0L) break;
                     if (chattimer == 0L) chattimer = -3L;
                     */
                     if ( chattimer > 1L )
                         break;
                     chattimer = 0L;

                     txtimer = h_timer_reset();
                     txretries = 0;
                     txState = HTX_END;
                 }
                 break;

            /*---------------------------------------------------*/
        }/*switch(txState)*/
        /* }/+while(txState&&pkttype) */
    } while (txState);

    if ( txfd )
        txclose( &txfd, ( res == XFER_OK ) ? FOP_OK : FOP_ERROR );
    if ( rxfd )
        rxclose( &rxfd, ( res == XFER_OK ) ? FOP_OK : FOP_ERROR );


	if ( res == XFER_ABORT ) {
		PURGEOUT();
		if ( tty_gothup != HUP_LINE ) {
			DEBUG(('H',2,"Sending abortstr"));
			PUTSTR( abortstr );
			tnow = timer_set( 2 );

			while( !timer_expired( tnow ))
				if ( GETCHAR( 1 ) == H_DLE && ( tty_gothup != HUP_LINE ))
					tnow = timer_set( 2 );

			if ( tty_gothup != HUP_LINE )
				BUFFLUSH( 5 );
			else {
				PURGEALL();
			}
		} else {
			PURGE();
		}
	} else if ( batchesdone > 1 ) {
		BUFFLUSH( 5 );
	}

    DEBUG(('H',1,"Hydra: batches done: %d", batchesdone));

    return (res);
}/*hydra_send()*/

/* end of hydra.c */
