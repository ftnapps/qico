/*=============================================================================
                       The HYDRA protocol was designed by
                 Arjen G. Lentz, LENTZ SOFTWARE-DEVELOPMENT and
                             Joaquim H. Homrighausen
                  COPYRIGHT (C) 1991-1993; ALL RIGHTS RESERVED
 =============================================================================*/
/* $Id: hydra.c,v 1.1.1.1 2003/07/12 21:26:43 sisoft Exp $ */
#include "headers.h"
#include <stdarg.h>
#include "defs.h"
#include "hydra.h"
#include "byteop.h"

#define SLONG 4 /* sizeof(long) */
#define SWORD 2 /* sizeof(word) */

#ifdef NEED_DEBUG

char *hstates[]={
"HTX_DONE",
"HTX_START",
"HTX_SWAIT",
"HTX_INIT",
"HTX_INITACK",
"HTX_RINIT",
"HTX_FINFO",
"HTX_FINFOACK",
"HTX_XDATA",
"HTX_DATAACK",  
"HTX_XWAIT",
"HTX_EOF",
"HTX_EOFACK",   
"HTX_REND",     
"HTX_END",      
"HTX_ENDACK"};

char *hpkts[]={
"HPKT_START",
"HPKT_INIT",
"HPKT_INITACK",
"HPKT_FINFO",
"HPKT_FINFOACK",
"HPKT_DATA",
"HPKT_DATAACK",
"HPKT_RPOS",
"HPKT_EOF",
"HPKT_EOFACK",
"HPKT_END",
"HPKT_IDLE",
"HPKT_DEVDATA",
"HPKT_DEVDACK"
};

#endif

int hydra_modifier;

#define crc16block crc16prp
#define crc32block crc32

/* HYDRA Some stuff to aid readability of the source and prevent typos ----- */
#define h_crc16test(crc)   (((crc) == 0xf0b8     ) ? 1 : 0)
#define h_crc32test(crc)   (((crc) == 0xdebb20e3L) ? 1 : 0)
#define h_uuenc(c)         (((c) & 0x3f) + '!')
#define h_uudec(c)         (((c) - '!') & 0x3f)
typedef long               h_timer;
#define h_timer_set(t)     (time(NULL) + (t))
#define h_timer_running(t) (t != 0L)
#define h_timer_expired(t) (time(NULL) > (t))
#define h_timer_reset()    (0L)

/* HYDRA's memory ---------------------------------------------------------- */
static  int     originator;
static  int     batchesdone;                    /* No. HYDRA batches done    */
static  boolean hdxlink;                        /* hdx link & not orig side  */
static  dword   options;                        /* INIT options hydra_init() */
static  char   *hdxmsg     = "fallback to one-way xfer";
static  char   *pktprefix  = "";
static  char   *autostr    = "hydra\r";

static  char    txpktprefix[H_PKTPREFIX + 1];   /* pkt prefix str they want  */
static  h_timer                 braindead;      /* braindead timer           */
static  byte   *txbufin;                        /* read data from disk here  */
static  byte                    rxdle;          /* count of received H_DLEs  */
static  byte                    rxpktformat;    /* format of pkt receiving   */
static  word                    rxpktlen;       /* length of last packet     */
/*CYRILM:static*/
word    txmaxblklen;                    	/* max block length allowed  */
static  long    txlastack;                      /* last dataack received     */
static  long    txoffset,       rxoffset;       /* offset in file we begun   */
static  h_timer txtimer,        rxtimer;        /* retry timers              */
static  long                    rxlastsync;     /* filepos last sync retry   */
static  word    txgoodneeded;                   /* to send before larger blk */
static  word    txgoodbytes;                    /* no. sent at this blk size */

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

static void hydra_msgdev (byte *data, word len)
{       /* text is already NUL terminated by calling func hydra_devrecv() */
	len = len;
	write_log("HydraMsg: %s",data);
}/*hydra_msgdev()*/

static void h_devrecv(byte *data,word len)
{
	c_devrecv(data,len);
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
	{ "CON", h_devrecv    },                /* text to console (chat)    */
	{ "PRN", NULL         },                /* data to printer           */
	{ "ERR", NULL         },                /* text to error output      */
	{ NULL , NULL         }
};

/*---------------------------------------------------------------------------*/

boolean hydra_devfree (void)
{
	if (devtxstate || !(txoptions & HOPT_DEVICE) || txstate >= HTX_END)
		return (false);                      /* busy or not allowed       */
	    else
		return (true);                       /* allowed to send a new pkt */
}/*hydra_devfree()*/

/*---------------------------------------------------------------------------*/

boolean hydra_devsend(char *dev, byte *data, word len)
{
	if (!dev || !data || !len|| !hydra_devfree())return (false);

	xstrcpy(devtxdev,dev,H_FLAGLEN+1);
	strupr(devtxdev);
	devtxbuf = data;
	devtxlen=(len>H_MAXBLKLEN(hydra_modifier))?H_MAXBLKLEN(hydra_modifier):len;

	devtxid++;
	devtxtimer   = h_timer_reset();
	devtxretries = 0;
	devtxstate   = HTD_DATA;

	/* special for chat, only prolong life if our side keeps typing! */
	if (chattimer > 0L && !strcmp(devtxdev,"CON") && txstate == HTX_REND)
		braindead = h_timer_set(H_BRAINDEAD);

	return (true);
}/*hydra_devsend()*/


/*---------------------------------------------------------------------------*/
boolean hydra_devfunc (char *dev, void (*func) (byte *data, word len))
{
	register int i;

	for (i = 0; h_dev[i].dev; i++) {
		if (!strncasecmp(dev,h_dev[i].dev,H_FLAGLEN)) {
			h_dev[i].func = func;
			return (true);
		}
	}

	return (false);
}/*hydra_devfunc()*/


/*---------------------------------------------------------------------------*/
static void hydra_devrecv (void)
{
	register char *p = (char *) rxbuf;
	register int   i;
	word len = rxpktlen;

	p += 4;                       /* skip the id long  */
	len -= 4;
	for (i = 0; h_dev[i].dev; i++) {                /* walk through devs */
		if (!strncmp(p,h_dev[i].dev,H_FLAGLEN)) {
			if (h_dev[i].func) {
				len -= ((int) strlen(p)) + 1;         /* sub devstr len    */
				p += ((int) strlen(p)) + 1;           /* skip devtag       */
				p[len] = '\0';                        /* NUL terminate     */
				(*h_dev[i].func)((byte *) p,len);     /* call output func  */
			}
			break;
		}
	}
}/*hydra_devrecv()*/


/*---------------------------------------------------------------------------*/
static void put_flags (char *buf, struct _h_flags flags[], long val, size_t size)
{
	register char *p;
	register int   i;

	p = buf;
	for (i = 0; flags[i].val; i++) {
		if (val & flags[i].val) {
			if (p > buf) *p++ = ',';
			xstrcpy(p,flags[i].str,size-(p-buf));
			p += H_FLAGLEN;
		}
	}
	*p = '\0';
}/*put_flags()*/


/*---------------------------------------------------------------------------*/
static dword get_flags (char *buf, struct _h_flags flags[])
{
	register dword  val;
	register char  *p;
	register int    i;

	val = 0x0L;
	for (p = strtok(buf,","); p; p = strtok(NULL,",")) {
		for (i = 0; flags[i].val; i++) {
			if (!strcmp(p,flags[i].str)) {
				val |= flags[i].val;
				break;
			}
		}
	}

	return (val);
}/*get_flags()*/


/*---------------------------------------------------------------------------*/
static byte *put_binbyte (register byte *p, register byte c)
{
	register byte n;

	n = c;
	if (txoptions & HOPT_HIGHCTL)
		n &= 0x7f;

	if (n == H_DLE ||
		((txoptions & HOPT_XONXOFF) && (n == XON || n == XOFF)) ||
		((txoptions & HOPT_TELENET) && n == '\r' && txlastc == '@') ||
		((txoptions & HOPT_CTLCHRS) && (n < 32 || n == 127))) {
		*p++ = H_DLE;
		c ^= 0x40;
	}

	*p++ = c;
	txlastc = n;

	return (p);
}/*put_binbyte()*/


/*---------------------------------------------------------------------------*/
static void txpkt (register word len, int type)
{
	register byte *in, *out;
	register word  c, n;
	boolean crc32b = false;
	byte    format;
	static char hexdigit[] = "0123456789abcdef";

	getipcm();

	DEBUG(('H',1,"txpkt %s (%c) len=%d", hpkts[type-'A'], type, len));
	if(type==HPKT_DEVDATA)DEBUG(('H',3,"devdata: %s",txbufin+8));
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
		crc32b = true;


	if (crc32b) {
		dword crc = (~crc32block((char *) txbufin,len)) & 0xffffffff;

		txbufin[len++] = crc;
		txbufin[len++] = crc >> 8;
		txbufin[len++] = crc >> 16;
		txbufin[len++] = crc >> 24;
	}
	else {
		word crc = (~crc16block((char *) txbufin,len)) & 0xffff;

		txbufin[len++] = crc;
		txbufin[len++] = crc >> 8;
	}

	in = txbufin;
	out = txbuf;
	txlastc = 0;
	*out++ = H_DLE;
	*out++ = format;

	switch (format) {
	case HCHR_HEXPKT:
		for (; len > 0; len--, in++) {
			if (*in & 0x80) {
				*out++ = '\\';
				*out++ = hexdigit[((*in) >> 4) & 0x0f];
				*out++ = hexdigit[(*in) & 0x0f];
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
			c |= ((*in++) << n);
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
		for ( ; len >= 3; in += 3, len -= 3) {
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

	for (in = (byte *) txpktprefix; *in; in++) {
		switch (*in) {
		case 0xDD: /* transmit break signal for one second */
			break;
		case 0xDE: sleep(2);
			break;
		case 0xDF: PUTCHAR(0);
			break;
		default:  PUTCHAR(*in);
			break;
		}
	}

	PUTBLK(txbuf,(word) (out - txbuf));
}/*txpkt()*/

	
/*---------------------------------------------------------------------------*/
static int rxpkt (void)
{
	register byte *p, *q=NULL;
	register int   c, n, i;

/*  	if (keyabort()) */
/*  		return (H_SYSABORT); */

	getipcm();

	p = rxbufptr;

	while ((c = GETCHAR(0)) >= 0) {
		if (rxoptions & HOPT_HIGHBIT)
			c &= 0x7f;

		n = c;
		if (rxoptions & HOPT_HIGHCTL)
			n &= 0x7f;
		if (n != H_DLE &&
			(((rxoptions & HOPT_XONXOFF) && (n == XON || n == XOFF)) ||
			 ((rxoptions & HOPT_CTLCHRS) && (n < 32 || n == 127))))
			continue;

		if (rxdle || c == H_DLE) {
			switch (c) {
			case H_DLE:
				if (++rxdle >= 5)
					return (H_CANCEL);
				break;

			case HCHR_PKTEND:
				rxbufptr = p;

				/* BUGFIX by Igor Vanin */
                if (!rxbufptr) c=H_NOPKT;
                else switch (rxpktformat) {
				case HCHR_BINPKT:
					q = rxbufptr;
					break;

				case HCHR_HEXPKT:
					for (p = q = rxbuf; p < rxbufptr; p++) {
						if (*p == '\\' && *++p != '\\') {
							i = *p;
							n = *++p;
							if ((i -= '0') > 9) i -= ('a' - ':');
							if ((n -= '0') > 9) n -= ('a' - ':');
							if ((i & ~0x0f) || (n & ~0x0f)) {
								i = H_NOPKT;
								break;
							}
							*q++ = (i << 4) | n;
						}
						else
							*q++ = *p;
					}
					if (p > rxbufptr)
						c = H_NOPKT;
					break;

				case HCHR_ASCPKT:
					n = i = 0;
					for (p = q = rxbuf; p < rxbufptr; p++) {
						i |= ((*p & 0x7f) << n);
						if ((n += 7) >= 8) {
							*q++ = (byte) (i & 0xff);
							i >>= 8;
							n -= 8;
						}
					}
					break;

				case HCHR_UUEPKT:
					n = (int) (rxbufptr - rxbuf);
					for (p = q = rxbuf; n >= 4; n -= 4, p += 4) {
						if (p[0] <= ' ' || p[0] >= 'a' ||
							p[1] <= ' ' || p[1] >= 'a' ||
							p[2] <= ' ' || p[2] >= 'a') {
							c = H_NOPKT;
							break;
						}
						*q++ = (byte) ((h_uudec(p[0]) << 2) | (h_uudec(p[1]) >> 4));
						*q++ = (byte) ((h_uudec(p[1]) << 4) | (h_uudec(p[2]) >> 2));
						*q++ = (byte) ((h_uudec(p[2]) << 6) | h_uudec(p[3]));
					}
					if (n >= 2) {
						if (p[0] <= ' ' || p[0] >= 'a') {
							c = H_NOPKT;
							break;
						}
						*q++ = (byte) ((h_uudec(p[0]) << 2) | (h_uudec(p[1]) >> 4));
						if (n == 3) {
							if (p[0] <= ' ' || p[0] >= 'a') {
								c = H_NOPKT;
								break;
							}
							*q++ = (byte) ((h_uudec(p[1]) << 4) | (h_uudec(p[2]) >> 2));
						}
					}
					break;

				default:   /* This'd mean internal fluke */
					c = H_NOPKT;
					break;
				}

				rxbufptr = NULL;

				if (c == H_NOPKT)
					break;

				rxpktlen = (word) (q - rxbuf);
				if (rxpktformat != HCHR_HEXPKT && (rxoptions & HOPT_CRC32)) {
					if (rxpktlen < 5) {
						c = H_NOPKT;
						break;
					}
					n = h_crc32test(crc32block((char *) rxbuf,rxpktlen));
					rxpktlen -= 4;  /* remove CRC-32 */
				}
				else {
					if (rxpktlen < 3) {
						c = H_NOPKT;
						break;
					}
					n = h_crc16test(crc16block((char *) rxbuf,rxpktlen));
					rxpktlen -= (int) SWORD;  /* remove CRC-16 */
				}

				rxpktlen--;                     /* remove type  */

				if (n) {
					DEBUG(('H',1,"rxpkt %s (%c) len=%d", hpkts[rxbuf[rxpktlen]-'A'], rxbuf[rxpktlen], rxpktlen));
					return ((int) rxbuf[rxpktlen]);
				}/*goodpkt*/

				break;

			case HCHR_BINPKT: 
			case HCHR_HEXPKT: 
			case HCHR_ASCPKT: 
			case HCHR_UUEPKT:
				rxpktformat = c;
				p = rxbufptr = rxbuf;
				rxdle = 0;
				break;

			default:
				if (p) {
					if (p < rxbufmax)
						*p++ = (byte) (c ^ 0x40);
					else {
						p = NULL;
					}
				}
				rxdle = 0;
				break;
			}
		}
		else if (p) {
			if (p < rxbufmax)
				*p++ = (byte) c;
			else {
				p = NULL;
			}
		}
	}
	if(NOTTO(c)) return H_CARRIER;

	rxbufptr = p;

	if(!chattimer&&txstate==HTX_REND&&rxstate==HRX_DONE) {
		chattimer=-1L;
		return(HPKT_IDLE);
	}
	if (h_timer_running(braindead) && h_timer_expired(braindead)) {
		return (H_BRAINTIME);
	}
	if (h_timer_running(txtimer) && h_timer_expired(txtimer)) {
		return (H_TXTIME);
	}
	if (h_timer_running(devtxtimer) && h_timer_expired(devtxtimer)) {
		return (H_DEVTXTIME);
	}

/*    sys_idle();*/
	UHASDATA(200);
	return (H_NOPKT);
}/*rxpkt()*/


/*---------------------------------------------------------------------------*/
static void hydra_status (boolean xmit)
{
	check_cps();
	if(xmit) {
		sendf.foff=txpos;
		qpfsend();
	} else {
		recvf.foff=rxpos;
		qpfrecv();
	}
}/*hydra_status()*/

/*---------------------------------------------------------------------------*/
void hydra_init (dword want_options, boolean orig, int hmod, int rxwin, int txwin)
{
	hydra_modifier=hmod;
	
	txbuf=xmalloc(H_BUFLEN(hydra_modifier)+1);
	rxbuf=xmalloc(H_BUFLEN(hydra_modifier)+1);

	txbufin  = txbuf + ((H_MAXBLKLEN(hmod) + H_OVERHEAD + 5) * 2);
	rxbufmax = rxbuf + H_MAXPKTLEN(hmod);

	batchesdone = 0;

	originator=orig;
	if (originator)
		hdxlink = false;

	options = (want_options & HCAN_OPTIONS) & ~HUNN_OPTIONS;

	timeout = (word) (40960L / effbaud);
	if      (timeout < H_MINTIMER) timeout = H_MINTIMER;
	else if (timeout > H_MAXTIMER) timeout = H_MAXTIMER;

	if(hmod==1) {
		txmaxblklen = (effbaud / 300) * 128;
		if      (txmaxblklen < 256)         txmaxblklen = 256;
		else if (txmaxblklen > H_MAXBLKLEN(1)) txmaxblklen = H_MAXBLKLEN(1);
		rxblklen = txblklen = (effbaud < 2400U) ? 256 : 512;
	} else {
		txmaxblklen = H_MAXBLKLEN(hmod);
		rxblklen = txblklen = txmaxblklen / 4;
	}

	txgoodbytes  = 0;
	txgoodneeded = 1024;

	txstate = HTX_DONE;

	write_log("hydra %s-directional mode session",hdxlink ? "uni" : "bi");

	rxwindow = rxwin;
	txwindow = txwin;
}/*hydra_init()*/


/*---------------------------------------------------------------------------*/
void hydra_deinit (void)
{
	qpreset(0);qpreset(1);
	xfree(txbuf);xfree(rxbuf);
}/*hydra_deinit()*/


/*---------------------------------------------------------------------------*/
int hydra_file(char *txpathname, char *txalias)
{
	int   res=XFER_OK;
	int   pkttype;
	char *p, *q;
	int   i, count;
	time_t rxftime;
	size_t rxfsize;
	unsigned long hydra_txwindow=txwindow,hydra_rxwindow=rxwindow;
	struct stat statf;

	rxstatus=0;

	if(txpathname)
		if(!(txfd=txopen(txpathname, txalias))) return XFER_SKIP;
	
	/*-------------------------------------------------------------------*/
	if (txstate == HTX_DONE) {
		txstate        = HTX_START;
		txoptions      = HTXI_OPTIONS;
		txpktprefix[0] = '\0';

		rxstate   = HRX_INIT;
		rxoptions = HRXI_OPTIONS;
		rxdle     = 0;
		rxbufptr  = NULL;
		rxtimer   = h_timer_reset();

		devtxid    = devrxid = 0L;
		devtxtimer = h_timer_reset();
		devtxstate = HTD_DONE;

		braindead = h_timer_set(H_BRAINDEAD);
	} else txstate = HTX_FINFO;

	/*-------------------------------------------------------------------*/

	txstart  = 0L;
	txsyncid = 0L;

	txtimer   = h_timer_reset();
	txretries = 0;

	/*-------------------------------------------------------------------*/
	do {
		/*----------------------------------------------------------------*/
		switch (devtxstate) {
			/*---------------------------------------------------------*/
		case HTD_DATA:
			if (txstate > HTX_RINIT) {
				STORE32(txbufin,devtxid);
				p = (char *)(txbufin + 4);
				xstrcpy(p,devtxdev,1020);
				p += H_FLAGLEN + 1;
				memcpy(p,devtxbuf,devtxlen);
				txpkt(4 + H_FLAGLEN + 1 + devtxlen,HPKT_DEVDATA);
				devtxtimer = h_timer_set(timeout);
				devtxstate = HTD_DACK;
			}
			break;

			/*---------------------------------------------------------*/
		default:
			break;

			/*---------------------------------------------------------*/
		}

		/*----------------------------------------------------------------*/
		switch (txstate) {
			/*---------------------------------------------------------*/
		case HTX_START:
			PUTBLK((unsigned char*)autostr,(int) strlen(autostr));
			txpkt(0,HPKT_START);
			txtimer = h_timer_set(H_START);
			txstate = HTX_SWAIT;
			break;

			/*---------------------------------------------------------*/
		case HTX_INIT:
			p = (char *) txbufin;
			snprintf(p,1024-(p-(char*)txbufin),"%08lx%s,%s %s",
					H_REVSTAMP, progname, version, osname);
			p += ((int) strlen(p)) + 1;/* our app info & HYDRA rev. */
			put_flags(p,h_flags,HCAN_OPTIONS,1024-(p-(char*)txbufin));    /* what we CAN  */
			p += ((int) strlen(p)) + 1;
			put_flags(p,h_flags,options,1024-(p-(char*)txbufin));         /* what we WANT */
			p += ((int) strlen(p)) + 1;
			snprintf(p,1024-(p-(char*)txbufin),"%08lx%08lx",               /* TxRx windows */
					hydra_txwindow,hydra_rxwindow);
			p += ((int) strlen(p)) + 1;
			xstrcpy(p,pktprefix,1024-(p-(char*)txbufin));     /* pkt prefix string we want */
			p += ((int) strlen(p)) + 1;

			txoptions = HTXI_OPTIONS;
			txpkt((word) (((byte *) p) - txbufin), HPKT_INIT);
			txoptions = rxoptions;
			txtimer = h_timer_set(timeout / 2);
			txstate = HTX_INITACK;
			break;

			/*---------------------------------------------------------*/
		case HTX_FINFO:
			if (txfd) {
				int off;
				snprintf((char *) txbufin, 1024, "%08lx%08x%08lx%08lx%08x%s",
						sendf.mtime, sendf.ftot, 0L, 0L,
						(sendf.nf==1)?sendf.allf:sendf.nf,
						fnc(sendf.fname));
				strlwr((char *) txbufin + 40);
				off=strlen((char *)txbufin)+1;
				xstrcpy((char *)txbufin+off,sendf.fname,1024-off);
				i=strlen(sendf.fname)+strlen((char *)txbufin)+2;
//				hydra_status(true);
			} else {
				if (!txretries) {
/* 					write_log("hydra: End of batch"); */
					qpreset(1);
				}
				xstrcpy((char *) txbufin,"",2);
				i=1;
			}
			txpkt(i, HPKT_FINFO);
			txtimer = h_timer_set(txretries ? timeout / 2 : timeout);
			txstate = HTX_FINFOACK;
			break;

			/*---------------------------------------------------------*/
		case HTX_XDATA:
			
			if (txpos < 0L)
				i = -1;                                    /* Skip */
			else {
				STORE32(txbufin,txpos);
				if ((i = fread(txbufin + 4,1,txblklen,txfd)) < 0) {
					sline("hydra: file read error");
					DEBUG(('H',1,"hydra: file read error"));
					txclose(&txfd, FOP_ERROR);
					txpos = H_SUSPEND;                            /* Skip */
				}
			}

			if (i > 0) {
				txpos += i;
			
				txpkt(4 + i, HPKT_DATA);

				if (txblklen < txmaxblklen &&
					(txgoodbytes += i) >= txgoodneeded) {
					txblklen <<= 1;
					if (txblklen >= txmaxblklen) {
						txblklen = txmaxblklen;
						txgoodneeded = 0;
					}
					txgoodbytes = 0;
				}

				if (txwindow && (txpos >= (txlastack + txwindow))) {
					txtimer = h_timer_set(txretries ? timeout / 2 : timeout);
					txstate = HTX_DATAACK;
				}

				if (!txstart)
					txstart = time(NULL);
				hydra_status(true);
				break;
			}

			/* fallthrough to HTX_EOF */

			/*---------------------------------------------------------*/
		case HTX_EOF:
			STORE32(txbufin,txpos);
			txpkt(4,HPKT_EOF);
			txtimer = h_timer_set(txretries ? timeout / 2 : timeout);
			txstate = HTX_EOFACK;
			break;

			/*---------------------------------------------------------*/
		case HTX_END:
			txpkt(0,HPKT_END);
			txpkt(0,HPKT_END);
			txtimer = h_timer_set(timeout / 2);
			txstate = HTX_ENDACK;
			break;

			/*---------------------------------------------------------*/
		default:
			break;

			/*---------------------------------------------------------*/
		}

		/*----------------------------------------------------------------*/
		if((pkttype=rxpkt())!=H_NOPKT && txstate) {
/*  		while (txstate && ) { */ 
			
			DEBUG(('H',1,"txstate %s (%d) pkttype %s (%d) '%c'",
				hstates[txstate], txstate,
				(pkttype>='A')?hpkts[pkttype-'A']:"$", pkttype,
				(pkttype>='A' && pkttype<='N')?pkttype:'*'));
			/*----------------------------------------------------------*/
			switch (pkttype) {
				/*---------------------------------------------------*/
			case H_CARRIER:
			case H_CANCEL:
			case H_SYSABORT:
			case H_BRAINTIME:
				switch (pkttype) {
				case H_CARRIER:
				case H_CANCEL:
				case H_SYSABORT:
				case H_BRAINTIME:
					break;
				}
/* 				write_log("Hydra: %s",p); */
				if(txstate!=HTX_ENDACK)	res = XFER_ABORT;
				txstate = HTX_DONE;
				break;

				/*---------------------------------------------------*/
			case H_TXTIME:
				if (txstate == HTX_XWAIT || txstate == HTX_REND) {
					txpkt(0,HPKT_IDLE);
					txtimer = h_timer_set(H_IDLE);
					break;
				}

				if (++txretries > H_RETRIES) {
					write_log("hydra: too many errors");
					txstate = HTX_DONE;
					res = XFER_ABORT;
					break;
				}

				sline("hydra: Timeout - Retry %u",txretries);
				DEBUG(('H',1,"hydra: Timeout - Retry %u",txretries));
				txtimer = h_timer_reset();

				switch (txstate) {
				case HTX_SWAIT:    txstate = HTX_START; break;
				case HTX_INITACK:  txstate = HTX_INIT;  break;
				case HTX_FINFOACK: txstate = HTX_FINFO; break;
				case HTX_DATAACK:  txstate = HTX_XDATA; break;
				case HTX_EOFACK:   txstate = HTX_EOF;   break;
				case HTX_ENDACK:   txstate = HTX_END;   break;
				}
				break;

				/*---------------------------------------------------*/
			case H_DEVTXTIME:
				if (++devtxretries > H_RETRIES) {
					write_log("HD: Too many errors");
					txstate = HTX_DONE;
					res = XFER_ABORT;
					break;
				}

				sline("HD: Timeout - Retry %u",devtxretries);
				DEBUG(('H',1,"HD: Timeout - Retry %u",devtxretries));

				devtxtimer = h_timer_reset();
				devtxstate = HTD_DATA;
				break;

				/*---------------------------------------------------*/
			case HPKT_START:
				if (txstate == HTX_START || txstate == HTX_SWAIT) {
					txtimer = h_timer_reset();
					txretries = 0;
					txstate = HTX_INIT;
					braindead = h_timer_set(H_BRAINDEAD);
				}
				break;

				/*---------------------------------------------------*/
			case HPKT_INIT:
				if (rxstate == HRX_INIT) {
					p = (char *) rxbuf;
					p += ((int) strlen(p)) + 1;
					q = p + ((int) strlen(p)) + 1;
					rxoptions  = options | HUNN_OPTIONS;
/* 					write_log("Other end hydra can %s", p); */
/* 					write_log("Other end hydra wants %s", q); */
					rxoptions |= get_flags(q,h_flags);
					rxoptions &= get_flags(p,h_flags);
					rxoptions &= HCAN_OPTIONS;
					if (rxoptions < (options & HNEC_OPTIONS)) {
						write_log("hydra is incompatible on this link!");
						txstate = HTX_DONE;
						res = XFER_ABORT;
						break;
					}
					p = q + ((int) strlen(q)) + 1;
					rxwindow = txwindow = 0L;
					sscanf(p,"%08lx%08lx", &rxwindow,&txwindow);
					if (rxwindow < 0L) rxwindow = 0L;
					if (hydra_rxwindow &&
						(!rxwindow || hydra_rxwindow < rxwindow))
						rxwindow = hydra_rxwindow;
					if (txwindow < 0L) txwindow = 0L;
					if (hydra_txwindow &&
						(!txwindow || hydra_txwindow < txwindow))
						txwindow = hydra_txwindow;
					p += ((int) strlen(p)) + 1;
					xstrcpy(txpktprefix,p,H_PKTPREFIX+1);

					if (!batchesdone) {
						long revstamp;

						p = (char *) rxbuf;
						sscanf(p,"%08lx",&revstamp);
						p += 8;
						DEBUG(('H',1,"other end hydra is %s, rev %u",p,revstamp));
						if((q=strchr(p,','))!=NULL)*q='-';
						if(strchr(p,' ')>q)*strchr(p,' ')='/';
						if((q=strchr(p,','))!=NULL)*q='/';
						if(strncasecmp(rnode->mailer,p,strlen(p)))write_log("hydra remote mailer: %s",p);
						put_flags((char *) rxbuf,h_flags,rxoptions,1024);
						if (txwindow || rxwindow)
							write_log("hydra link options: %s [%d/%d]",rxbuf,
								txwindow,rxwindow);
						else
							write_log("hydra link options: %s",rxbuf);
					}

					chattimer = (rxoptions & HOPT_DEVICE) ? 0L : -2L;

					txoptions = rxoptions;
					rxstate = HRX_FINFO;
				}

				txpkt(0,HPKT_INITACK);
				break;

				/*---------------------------------------------------*/
			case HPKT_INITACK:
				if (txstate == HTX_INIT || txstate == HTX_INITACK) {
					braindead = h_timer_set(H_BRAINDEAD);
					txtimer = h_timer_reset();
					txretries = 0;
					txstate = HTX_RINIT;
				}
				break;

				/*---------------------------------------------------*/
			case HPKT_FINFO:
				if (rxstate == HRX_FINFO) {
					braindead = h_timer_set(H_BRAINDEAD);
					if (!rxbuf[0]) {
/* 						write_log("HR: End of batch"); */
						qpreset(0);
						rxpos = 0L;
						rxstate = HRX_DONE;
						batchesdone++;
					} else {
						
						sscanf((char *) rxbuf,"%08lx%08x%*08x%*08x%08x",
							   &rxftime, &rxfsize, &count);

						if(!recvf.allf && count) recvf.allf=count;

						p=(char *) (rxbuf+40);
						if(strlen(p)+41<rxpktlen)
							p=(char *)(rxbuf+41+strlen(p));
						switch(rxopen(p, rxftime, rxfsize,
									  &rxfd)) {
						case FOP_SKIP:
							rxpos=H_SKIP;
							break;
						case FOP_SUSPEND:
							rxpos=H_SUSPEND;
							break;
						case FOP_CONT:
						case FOP_OK:
							rxoffset = rxpos = ftell(rxfd);
							rxstart = 0L;
							rxtimer = h_timer_reset();
							rxretries = 0;
							rxlastsync = 0L;
							rxsyncid = 0L;
							hydra_status(false);
							rxstate = HRX_DATA;
							break;
						}
						qpfrecv();
					}
				} else if (rxstate == HRX_DONE) //{
					rxpos = (!rxbuf[0]) ? 0L : H_SUSPEND;
//					hydra_status(false);
//				}
				STORE32(txbufin, rxpos);
				txpkt(4,HPKT_FINFOACK);
				break;

				/*---------------------------------------------------*/
			case HPKT_FINFOACK:
				if (txstate == HTX_FINFO || txstate == HTX_FINFOACK) {
					braindead = h_timer_set(H_BRAINDEAD);
					txretries = 0;
					if (!txfd) {
						txtimer = h_timer_set(H_IDLE);
						txstate = HTX_REND;
					}
					else {
						txtimer = h_timer_reset();
						txpos = FETCH32(rxbuf);
						if (txpos >= 0L) {
							txoffset = txpos;
							txlastack = txpos;
							hydra_status(true);
							if(txpos > 0L) {
								if(fseek(txfd,txpos,SEEK_SET) < 0L) {
									txclose(&txfd, FOP_ERROR);
									txpos = H_SUSPEND;
									txstate = HTX_EOF;
									break;
								}
								sendf.soff=txpos;
							}
							txstate = HTX_XDATA;
						}
						else {
							switch(txpos) {
							case H_SKIP:
								txclose(&txfd,FOP_SKIP);
								return XFER_SKIP;
							case H_SUSPEND:
								txclose(&txfd,FOP_SUSPEND);
								return XFER_SUSPEND;
							default:
								txclose(&txfd,FOP_ERROR);
								return XFER_ABORT;
							}
						}
					}
				}
				break;

				/*---------------------------------------------------*/
			case HPKT_DATA:
				if (rxstate == HRX_DATA) {
					int gotpos=FETCH32(rxbuf);
					if(rxstatus) {
						rxpos=((rxstatus==RX_SUSPEND)?H_SUSPEND:H_SKIP);
						rxclose(&rxfd,(rxstatus==RX_SUSPEND)?FOP_SUSPEND:FOP_SKIP);
						rxretries=1;
						rxsyncid++;
						STORE32(txbufin,rxpos);
						STORE32(txbufin+4,0L);
						STORE32(txbufin+8,rxsyncid);
						txpkt(12,HPKT_RPOS);
						rxtimer=h_timer_set(timeout);
						break;
					}
					if(gotpos!=rxpos||gotpos<0L) {
						if (gotpos <= rxlastsync) {
							rxtimer = h_timer_reset();
							rxretries = 0;
						}
						rxlastsync = FETCH32(rxbuf);

						if (!h_timer_running(rxtimer) ||
							h_timer_expired(rxtimer)) {
							if (rxretries > 4) {
								if (txstate < HTX_REND &&
									!originator && !hdxlink) {
									hdxlink = true;
									rxretries = 0;
								}
							}
							if (++rxretries > H_RETRIES) {
								write_log("HR: too many errors");
								txstate = HTX_DONE;
								res = XFER_ABORT;
								break;
							}
							if (rxretries == 1)
								rxsyncid++;

							rxblklen /= 2;
							i = rxblklen;
							if      (i <=  64) i =   64;
							else if (i <= 128) i =  128;
							else if (i <= 256) i =  256;
							else if (i <= 512) i =  512;
							else if (i <=1024) i = 1024;
							else if (i <=2048) i = 2048;
							else if (i <=4096) i = 4096;
							else if (i <=8192) i = 8192;              
							else 		   i = 16384;

							if (hydra_modifier == 8 && i > 8192)
								i = 8192;
							else if (hydra_modifier == 4 && i > 4096)
								i = 4096;
							else if (hydra_modifier == 1 && i > 1024)
								i = 1024;
							
							sline("HR: bad pkt at %ld - Retry %u (newblklen=%u)",
								  rxpos,rxretries,i);
							DEBUG(('H',1,"HR: bad pkt at %ld - Retry %u (newblklen=%u)",
								  rxpos,rxretries,i));
							STORE32(txbufin, rxpos);
							STORE32(txbufin+4, i);
							STORE32(txbufin+8, rxsyncid);
							txpkt(3 * 4,HPKT_RPOS);
							rxtimer = h_timer_set(timeout);
						}
					}
					else { char tmp[255];
						braindead = h_timer_set(H_BRAINDEAD);
						rxpktlen -= 4;
						rxblklen = rxpktlen;
						snprintf(tmp, 255, "%s/tmp/%s", ccs, recvf.fname);
						if (stat(tmp, &statf) && errno == ENOENT)
						{
						 rxclose(&rxfd, FOP_SKIP);
						 rxpos = H_SKIP;
						 rxretries = 1;
						 rxsyncid++;
						 STORE32(txbufin, rxpos);
						 txpkt(4,HPKT_RPOS);
						 rxtimer = h_timer_set(timeout);
						 break;
						}
						if (fwrite(rxbuf + 4,rxpktlen,1,rxfd) != 1) {
							sline("HR: file write error");
							DEBUG(('H',1,"HR: file write error"));
							rxclose(&rxfd, FOP_ERROR);
							rxpos = H_SUSPEND;
							rxretries = 1;
							rxsyncid++;
							STORE32(txbufin, rxpos);
							STORE32(txbufin+4, 0L);
							STORE32(txbufin+8, rxsyncid);
							txpkt(3 * 4,HPKT_RPOS);
							rxtimer = h_timer_set(timeout);
							if (errno==ENOSPC) return XFER_ABORT;
							break;
						}
						rxretries = 0;
						rxtimer = h_timer_reset();
						rxlastsync = rxpos;
 						rxpos += rxpktlen; 
						if (rxwindow) {
							STORE32(txbufin, rxpos);
							txpkt(4,HPKT_DATAACK);
						}
						if (!rxstart)
							rxstart = time(NULL) -
								((rxpktlen * 10) / effbaud);
						hydra_status(false);
					}/*badpkt*/
				}/*rxstate==HRX_DATA*/
				break;

				/*---------------------------------------------------*/
			case HPKT_DATAACK:
				if (txstate == HTX_XDATA || txstate == HTX_DATAACK ||
					txstate == HTX_XWAIT ||
					txstate == HTX_EOF || txstate == HTX_EOFACK) {
					if (txwindow && FETCH32(rxbuf) > txlastack) {
						txlastack = FETCH32(rxbuf);
						if (txstate == HTX_DATAACK &&
							(txpos < (txlastack + txwindow))) {
							txstate = HTX_XDATA;
							txretries = 0;
							txtimer = h_timer_reset();
						}
					}
				}
				break;

				/*---------------------------------------------------*/
			case HPKT_RPOS:
				if (txstate == HTX_XDATA || txstate == HTX_DATAACK ||
					txstate == HTX_XWAIT ||
					txstate == HTX_EOF || txstate == HTX_EOFACK) {
					if (FETCH32(rxbuf+8) != txsyncid) {
						txsyncid = FETCH32(rxbuf+8);
						txretries = 1;
						txtimer = h_timer_reset();
						txpos = FETCH32(rxbuf);
						if (txpos < 0L) {
							if (txfd) {
								sline("hydra: %s %s",txpos==H_SUSPEND?"suspending":(txpos==H_SKIP?"skipping":"strange skipping"),sendf.fname);
								DEBUG(('H',1,"hydra: %s %s",txpos==H_SUSPEND?"suspending":(txpos==H_SKIP?"skipping":"strange skipping"),sendf.fname));
								txclose(&txfd, txpos==H_SUSPEND?FOP_SUSPEND:FOP_SKIP);
								txstate = HTX_EOF;
							}
							txpos = (txpos==H_SUSPEND?H_SUSPEND:H_SKIP);
							break;
						}

						if (txblklen > FETCH32(rxbuf+4))
							txblklen = (word) FETCH32(rxbuf+4);
						else
							txblklen >>= 1;
						if      (txblklen <=  64) txblklen =   64;
						else if (txblklen <= 128) txblklen =  128;
						else if (txblklen <= 256) txblklen =  256;
						else if (txblklen <= 512) txblklen =  512;
						else if (txblklen <=1024) txblklen = 1024;
						else if (txblklen <=4096) txblklen = 4096;
						else if (txblklen <=8192) txblklen = 8192;
						else                      txblklen = 16384;
						
						if (hydra_modifier == 8 && txblklen > 8192)
							txblklen = 8192;
						else if (hydra_modifier == 4 && txblklen > 4096)
							txblklen = 4096;
						else if (hydra_modifier == 1 && txblklen > 1024)
							txblklen = 1024;

						txgoodbytes = 0;
						txgoodneeded += 1024;
						if (txgoodneeded > 8192)
							txgoodneeded = 8192;

						hydra_status(true);
						sline("hydra: Resending from offset %ld (newblklen=%u)",txpos,txblklen);
						DEBUG(('H',1,"hydra: Resending from offset %ld (newblklen=%u)",txpos,txblklen));
						if (fseek(txfd,txpos,SEEK_SET) < 0L) {
							txclose(&txfd, FOP_ERROR);
							txpos = H_SUSPEND;
							txstate = HTX_EOF;
							break;
						}

						if (txstate != HTX_XWAIT)
							txstate = HTX_XDATA;
					}
					else {
						if (++txretries > H_RETRIES) {
							write_log("hydra: too many errors");
							txstate = HTX_DONE;
							res = XFER_ABORT;
							break;
						}
					}
				}
				break;

				/*---------------------------------------------------*/
			case HPKT_EOF:
				if (rxstate == HRX_DATA) {
					if (FETCH32(rxbuf) < 0L) {
						rxclose(&rxfd, FOP_SKIP);
						rxstate = HRX_FINFO;
						braindead = h_timer_set(H_BRAINDEAD);
					}
					else if (FETCH32(rxbuf) != rxpos) {
						if (FETCH32(rxbuf) <= rxlastsync) {
							rxtimer = h_timer_reset();
							rxretries = 0;
						}
						rxlastsync = FETCH32(rxbuf);

						if (!h_timer_running(rxtimer) ||
							h_timer_expired(rxtimer)) {
							if (++rxretries > H_RETRIES) {
								write_log("HR: too many errors");
								txstate = HTX_DONE;
								res = XFER_ABORT;
								break;
							}
							if (rxretries == 1)
								rxsyncid++;

							rxblklen /= 2;
							i = rxblklen;
							if      (i <=  64) i =   64;
							else if (i <= 128) i =  128;
							else if (i <= 256) i =  256;
							else if (i <= 512) i =  512;
							else if (i <=1024) i = 1024;
							else if (i <=4096) i = 4096;
							else if (i <=8192) i = 8192;
							else               i = 16384;

							if (hydra_modifier == 8 && i > 8192)
								i = 8192;
							else if (hydra_modifier == 4 && i > 4096)
								i = 4096;
							else if (hydra_modifier == 1 && i > 1024)
								i = 1024;

							sline("HR: Bad EOF at %ld - Retry %u (newblklen=%u)",rxpos,rxretries,i);
							DEBUG(('H',1,"HR: Bad EOF at %ld - Retry %u (newblklen=%u)",rxpos,rxretries,i));
							STORE32(txbufin, rxpos);
							STORE32(txbufin+4, i);
							STORE32(txbufin+8, rxsyncid);
							txpkt(3 * 4,HPKT_RPOS);
							rxtimer = h_timer_set(timeout);
						}
					}
					else {
						rxfsize = rxpos;
						rxclose(&rxfd, FOP_OK);
						hydra_status(false);
						rxstate = HRX_FINFO;
						braindead = h_timer_set(H_BRAINDEAD);
					}/*skip/badeof/eof*/
				}/*rxstate==HRX_DATA*/

				if (rxstate == HRX_FINFO)
					txpkt(0,HPKT_EOFACK);
				break;

				/*---------------------------------------------------*/
			case HPKT_EOFACK:
				if (txstate == HTX_EOF || txstate == HTX_EOFACK) {
					braindead = h_timer_set(H_BRAINDEAD);
					if(txfd) {
						txclose(&txfd, FOP_OK);
						return (XFER_OK);
					} else return (txpos==H_SUSPEND?XFER_SUSPEND:XFER_SKIP);
				}
				break;

				/*---------------------------------------------------*/
			case HPKT_IDLE:
				if (txstate == HTX_XWAIT) {
					hdxlink = false;
					txtimer = h_timer_reset();
					txretries = 0;
					txstate = HTX_XDATA;
				}
				else if (txstate >= HTX_FINFO && txstate < HTX_REND)
					braindead = h_timer_set(H_BRAINDEAD);
				break;

				/*---------------------------------------------------*/
			case HPKT_END:
				/* special for chat, other side wants to quit */
 				if (chattimer > 0L && txstate == HTX_REND) {
 					chattimer = -3L;
 					break;
 				}

				if (txstate == HTX_END || txstate == HTX_ENDACK) {
					txpkt(0,HPKT_END);
					txpkt(0,HPKT_END);
					txpkt(0,HPKT_END);
/* 					write_log("Hydra done"); */
					txstate = HTX_DONE;
					res = XFER_OK;
				}
				break;

				/*---------------------------------------------------*/
			case HPKT_DEVDATA:
				if (devrxid != FETCH32(rxbuf)) {
					hydra_devrecv();
					devrxid = FETCH32(rxbuf);
				}
				STORE32(txbufin, devrxid);
				txpkt(4,HPKT_DEVDACK);
				break;

				/*---------------------------------------------------*/
			case HPKT_DEVDACK:
				if (devtxstate && (devtxid == FETCH32(rxbuf))) {
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
			switch (txstate) {
				/*---------------------------------------------------*/
			case HTX_START:
			case HTX_SWAIT:
				if (rxstate == HRX_FINFO) {
					txtimer = h_timer_reset();
					txretries = 0;
					txstate = HTX_INIT;
				}
				break;

				/*---------------------------------------------------*/
			case HTX_RINIT:
				if (rxstate == HRX_FINFO) {
					txtimer = h_timer_reset();
					txretries = 0;
					txstate = HTX_FINFO;
				}
				break;

				/*---------------------------------------------------*/
			case HTX_XDATA:
				if (rxstate && hdxlink) {
					write_log("HydraMsg: %s",hdxmsg);
					hydra_devsend("MSG",(byte *) hdxmsg,(int) strlen(hdxmsg));

					txtimer = h_timer_set(H_IDLE);
					txstate = HTX_XWAIT;
				}
				break;

				/*---------------------------------------------------*/
			case HTX_XWAIT:
				if (!rxstate) {
					txtimer = h_timer_reset();
					txretries = 0;
					txstate = HTX_XDATA;
				}
				break;

				/*---------------------------------------------------*/
			case HTX_REND:
				if (!rxstate && !devtxstate) {
                                /* special for chat, braindead will protect */
  					if (chattimer > 0L) break;
  					if (chattimer == 0L) chattimer = -3L;

					txtimer = h_timer_reset();
					txretries = 0;
					txstate = HTX_END;
				}
				break;

				/*---------------------------------------------------*/
			}/*switch(txstate)*/
		}/*while(txstate&&pkttype)*/
	} while (txstate);

	txclose(&txfd, (res==XFER_OK)?FOP_OK:FOP_ERROR);
	rxclose(&rxfd, (res==XFER_OK)?FOP_OK:FOP_ERROR);

	if (res == XFER_ABORT) {             /* ABORT!!!!!!!!!!!!!!!!! */
		if(!CARRIER()) 
			CANCEL();
		PURGE();
	}

	return (res);
}/*hydra()*/

/* end of hydra.c */
