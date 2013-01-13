/**********************************************************
 * protocol definitions
 * $Id: mailer.h,v 1.12 2005/05/16 11:17:30 mitry Exp $
 **********************************************************/
#ifndef __MAILER_H__
#define __MAILER_H__
#include "qcconst.h"

#define P_NONE		0x0000	/* 0 0000 0000 */
#define P_NCP		0x0001	/* 0 0000 0001 */
#ifdef WITH_BINKP
#define P_BINKP		0x0001	/* 0 0000 0001 */
#endif
#define P_ZMODEM	0x0002	/* 0 0000 0010 */
#define P_ZEDZAP	0x0004	/* 0 0000 0100 */
#define P_DIRZAP	0x0008	/* 0 0000 1000 */
#define P_HYDRA		0x0010	/* 0 0001 0000 */
#define P_JANUS		0x0020	/* 0 0010 0000 */
#ifdef HYDRA8K16K
#define P_HYDRA4	0x0040	/* 0 0100 0000 */
#define P_HYDRA8	0x0080	/* 0 1000 0000 */
#define P_HYDRA16	0x0100	/* 1 0000 0000 */
#endif
#define P_MASK		0x01FF	/* 1 1111 1111 */


#define S_OK      0
#define S_NODIAL  1
#define S_REDIAL  2
#define S_BUSY    3
#define S_FAILURE 4
#define S_MASK    7
#define S_HOLDR   8
#define S_HOLDX   16
#define S_HOLDA   32
#define S_ADDTRY  64
#define S_ANYHOLD (S_HOLDR|S_HOLDX|S_HOLDA)

#define FOP_OK      0
#define FOP_CONT    1
#define FOP_SKIP    2
#define FOP_ERROR   3
#define FOP_SUSPEND 4

#define SESSION_AUTO    0
#define SESSION_EMSI    1
#define SESSION_BINKP   2

/* session mode flags */
#define SM_INBOUND	0
#define SM_OUTBOUND	1

/* force_call flags */
#define FC_NORMAL	0
#define FC_IMMED	1
#define FC_ANY		2

/* FREQs flags */
#define FR_NOTHANDLED	(-1)
#define FR_NOTAVAILABLE	0
#define FR_AVAILABLE	1

#define RX_SKIP		1
#define RX_SUSPEND	2

#define HUP_NONE	0
#define HUP_LINE	1
#define HUP_OPERATOR	2
#define HUP_SESLIMIT	3
#define HUP_CPS		4

#define M_STAT \
    ( tty_gothup == HUP_NONE     ? "ok"              :                \
    ( tty_gothup == HUP_LINE     ? "carrier lost"    :                \
    ( tty_gothup == HUP_OPERATOR ? "hangup"          :                \
    ( tty_gothup == HUP_SESLIMIT ? "session limit"   :                \
    ( tty_gothup == HUP_CPS      ? "low cps"         : "unknown" )))))

#define TIM_CHAT	90

#define MSG_NEVER	"Never"


/* ls_z*.c */
int	zmodem_receive(char *, int);
int	zmodem_send(slist_t *, slist_t *);
int	zmodem_sendfile(char *, char *, unsigned long *, unsigned long *);
int	zmodem_sendinit(int);
int	zmodem_senddone(void);


/* emsi.c */
void	emsi_makedat(ftnaddr_t *, unsigned long, unsigned long,
		int, char *, falist_t *, int);
int	emsi_init(int);
int	emsi_send(void);
int	emsi_recv(int, ninfo_t *);


/* protfm.c */
int	rxopen(char *, time_t, off_t, FILE **);
int	rxclose(FILE **, int);
FILE	*txopen(char *, char *);
int	txclose(FILE **, int);
void	log_rinfo(ninfo_t *);
void	check_cps(void);

extern	FILE *txfd;
extern	FILE *rxfd;
extern	volatile long txpos;
extern	volatile long rxpos;
extern	word txblklen;
extern	word rxblklen;
extern	byte *txbuf;
extern	byte *rxbuf;
extern	unsigned effbaud;
extern	byte *rxbufptr;
extern	int txstate;
extern	int rxstate;
extern	byte *rxbufmax;
extern	long txstart;
extern	word txmaxblklen;
extern	word timeout;
extern	byte txlastc;
/* for chat */
void	chatinit(int);
void	c_devrecv(unsigned char *, unsigned);
void	getevt(void);


/* session.c */
extern	int (*receive_callback)(char *);
int	receivecb(char *);
void	makeflist(flist_t **, ftnaddr_t *, int);
void	flkill(flist_t **, int);
void	flexecute(flist_t *);
void	addflist(flist_t **, char *, char *, char, off_t, FILE *, int);
void	simulate_send(ftnaddr_t *);
int	session(int, int, ftnaddr_t *, int);


/* freq.c */
int	freq_ifextrp(slist_t *);
int	freq_recv(char *);
int	is_freq_available(void);	/* Can we handle FREQs now/ever */


/* call.c */
int	do_call(ftnaddr_t *, char *, char *);
int	force_call(ftnaddr_t *, int, int);


/* perl.c */
#ifdef WITH_PERL
#define IFPerl(x) x
extern	unsigned short perl_flg;
int	perl_init(const char *, int);
void	perl_done(int);
void	perl_on_reload(int);
void	perl_on_std(int);
void	perl_on_log(char *);
int	perl_on_call(const ftnaddr_t *, const char *, const char *);
int	perl_on_session(char *);
void	perl_end_session(long, int);
int	perl_on_recv(void);
char	*perl_end_recv(int);
char	*perl_on_send(const char *);
void	perl_end_send(int);
#else
#define IFPerl(x)
#endif

#endif
