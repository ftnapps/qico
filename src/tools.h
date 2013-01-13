/*
 * $Id: tools.h,v 1.12 2005/08/10 19:45:50 mitry Exp $
 *
 * $Log: tools.h,v $
 * Revision 1.12  2005/08/10 19:45:50  mitry
 * Added param to qscandir() to return full path with file name
 *
 * Revision 1.11  2005/05/17 18:17:42  mitry
 * Removed system basename() usage.
 * Added qbasename() implementation.
 *
 * Revision 1.10  2005/05/16 11:20:13  mitry
 * Updated function prototypes. Changed code a bit.
 *
 * Revision 1.9  2005/05/06 20:48:55  mitry
 * Misc code cleanup
 *
 * Revision 1.8  2005/04/14 18:04:26  mitry
 * Changed scandir() to qscandir()
 *
 * Revision 1.7  2005/04/05 09:33:41  mitry
 * Update write_debug_log() prototype
 *
 * Revision 1.6  2005/03/31 19:40:38  mitry
 * Update function prototypes and it's duplication
 *
 */

#ifndef __TOOLS_H__
#define __TOOLS_H__

#define EXT_OK		0
#define EXT_DID_WORK	1
#define EXT_YOURSELF	2

#define C_INT     1
#define C_STR     2
#define C_ADDR    3
#define C_ADRSTR  4
#define C_PATH    5
#define C_YESNO   6
#define C_ADDRL   7
#define C_ADRSTRL 8
#define C_STRL    9
#define C_OCT     10

typedef struct _cfgitem_t {
	slist_t			*condition;
	union {
		int		v_int;
		char		*v_char;
		falist_t	*v_al;
		faslist_t	*v_fasl;
		slist_t		*v_sl;
	} value;
	struct _cfgitem_t	*next;
} cfgitem_t;

typedef struct {
	char		*keyword;
	int		type, flags;
	cfgitem_t	*items;
	char		*def_val;
} cfgstr_t;

extern	char *sigs[];

void	recode_to_remote(char *);
void	recode_to_local(char *);
char	*mappath(const char *);
char	*mapname(char *, char *, size_t);
char	*qbasename(const char *);
int	hexdcd(char, char);
char	*qver(int);
unsigned
long	sequencer(void);
off_t	filesize(const char *);
int	lunlink(const char *);
int	lockpid(const char *);
int	islocked(const char *);

FILE	*mdfopen(char *, const char *);
int	fexist(const char *);
int	mkdirs(char *);
void	rmdirs(char *);
int	qalphasort(const void *, const void *);
int	qscandir(const char *, char ***, int,
		int (*)(const char *), int (*)(const void *, const void *));
int	fmatchcase(const char *, char ***);
char	*fnc(char *);
int	isdos83name(char *);
size_t	getfreespace(const char *);
void	to_dev_null(void);
int	randper(int, int);
int	execsh(const char *);
int	execnowait(const char *, const char *, const char *, const char *);
void	qsleep(int);


/* config.c */
int		cfgi(int);
char		*cfgs(int);
slist_t		*cfgsl(int);
faslist_t	*cfgfasl(int);
falist_t	*cfgal(int);
int		readconfig(const char *);
void		rereadconfig(int);
void		killconfig(void);
#ifdef NEED_DEBUG
void		dumpconfig(void);
#endif


/* log.c */
extern	void (*log_callback)(const char *str);

int	log_init(const char *, const char *);
void	vwrite_log(const char *, int);
void	write_log(const char *, ...);

#ifdef NEED_DEBUG
extern	int facilities_levels[256];

void	parse_log_levels(void);
void	write_debug_log(unsigned char, int, const char *, ...);

#ifdef __GNUC__
#define DEBUG(all) __DEBUG all
#define __DEBUG(F,L,A...) do { if(facilities_levels[(unsigned char)(F)]>=(L)) write_debug_log((unsigned char)(F),(L),##A); } while(0)
#else
#define DEBUG(all) write_debug_log all
#endif
#else
#define DEBUG(all)
#endif

void	log_done(void);

int	chatlog_init(const char *, const ftnaddr_t *, int);
void	chatlog_write(const char *, int);
void	chatlog_done(void);


/* main.c */
RETSIGTYPE	sigerr(int);
void		stopit(int);

/* daemon.c */
void	daemon_mode(void);

/* flagexp.y */
int	flagexp(slist_t *, int);

#endif
