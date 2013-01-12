/**********************************************************
 * Queue operations
 **********************************************************/
/*
 * $Id: queue.c,v 1.12 2005/08/18 16:11:22 mitry Exp $
 *
 * $Log: queue.c,v $
 * Revision 1.12  2005/08/18 16:11:22  mitry
 * Extended debug message
 *
 * Revision 1.11  2005/06/10 20:49:32  mitry
 * Updated q_each() to make qico to call a node if we have
 * empty FLOw file(s) or netmail packet(s) with callable
 * flavour.
 *
 * Revision 1.10  2005/05/16 11:17:30  mitry
 * Updated function prototypes. Changed code a bit.
 *
 * Revision 1.9  2005/05/11 16:44:40  mitry
 * Changed a bit a call of outbound_rescan()
 *
 * Revision 1.8  2005/05/06 20:46:23  mitry
 * Empty FLOw file in outbound now can make a qico to poll node during mail hours.
 * Misc code cleanup.
 *
 * Revision 1.7  2005/03/31 19:40:38  mitry
 * Update function prototypes and it's duplication
 *
 */

#include "headers.h"
#include "qipc.h"

#define _Z(x) ((qitem_t *)x)
int q_cmp(const void *q1, const void *q2)
{
	if(_Z(q1)->addr.z!=_Z(q2)->addr.z)return _Z(q1)->addr.z-_Z(q2)->addr.z;
	if(_Z(q1)->addr.n!=_Z(q2)->addr.n)return _Z(q1)->addr.n-_Z(q2)->addr.n;
	if(_Z(q1)->addr.f!=_Z(q2)->addr.f)return _Z(q1)->addr.f-_Z(q2)->addr.f;
	return _Z(q1)->addr.p-_Z(q2)->addr.p;
}


qitem_t *q_find(const ftnaddr_t *fa)
{
	qitem_t	*i;
	int	r = 1;

	for ( i = q_queue; i && ( r = q_cmp( &i->addr, fa )) < 0 ; i = i->next );
	return r ? NULL : i;
}


qitem_t *q_add(const ftnaddr_t *fa)
{
	qitem_t	**i, *q;
	int	r = 1;

	for( i = &q_queue; *i && ( r = q_cmp( &(*i)->addr, fa )) < 0 ; i = &((*i)->next ));

	if ( !r )
		return *i;

	q = xcalloc( 1, sizeof( qitem_t ));
	q->next = *i;
	q->canpoll = 0;
	*i = q;
	addr_cpy( &q->addr, fa );

	return q;
}


int q_recountflo(const char *name, off_t *size, time_t *mtime, int rslow)
{
	FILE		*f;
	struct stat	sb;
	char		s[MAX_STRING + 5], *p, *m;
	off_t		total = 0;
	int		fcount = 1;

	DEBUG(('Q',4,"scan lo '%s'",name));

	if ( !stat( name, &sb )) {
		if ( sb.st_mtime != *mtime || rslow ) {
			*mtime = sb.st_mtime;
			f = fopen( name, "rt" );
			if ( f ) {
				fcount = 0;
				while( fgets( s, MAX_STRING - 1, f )) {
                                        DEBUG(('Q',5,"scan lo -> '%s'",s));
					if ( *s == '~' )
						continue;

					p = strrchr( s, '\r' );
					if ( p )
						*p = '\0';
					p = strrchr( s, '\n' );
					if ( p )
						*p = '\0';
					p = s;
					if ( *p == '^' || *p == '#' )
						p++;
					m = mappath( p );
					if ( *m )
						p = m;
					if ( !stat( p, &sb ))
						total += sb.st_size;
					fcount++;
                                        DEBUG(('Q',5,"%10lu '%s'",(unsigned long) sb.st_size,p));
				}
				fclose( f );
			} else {
				write_log( "can't open %s: %s", name, strerror( errno ));
			}
			*size = total;
		} else {
			fcount = sb.st_size;
		}
	} else {
		*mtime = 0;
		*size = 0;
	}

	return fcount;
}


void q_recountbox(const char *name, off_t *size, time_t *mtime, int rslow)
{
	DIR		*d;
	struct dirent	*de;
	struct stat	sb;
	char		p[MAX_PATH + 5];
	off_t		total = 0;

	if ( !stat( name, &sb )
		&& (( sb.st_mode & S_IFMT ) == S_IFDIR
			|| ( sb.st_mode & S_IFMT ) == S_IFLNK ))
	{
		if ( sb.st_mtime != *mtime || rslow ) {
			DEBUG(('Q',4,"scan box '%s'",name));
			*mtime = sb.st_mtime;
			d = opendir( name );
			if ( d ) {
				while(( de = readdir( d ))) {
					if ( de->d_name[0] == '.' )
						continue;
					snprintf( p, MAX_PATH, "%s/%s", name, de->d_name );
					if ( !stat( p, &sb ) && S_ISREG( sb.st_mode )) {
						DEBUG(('Q',6,"add file '%s'",p));
						total += sb.st_size;
					}
				}
				closedir( d );
			} else
				write_log("can't open %s: %s",name,strerror(errno));

			*size = total;
		}
	} else {
		DEBUG(('Q',4,"box '%s' lost",name));
		*mtime = 0;
		*size = 0;
	}
}


void q_each(const char *fname, const ftnaddr_t *fa, int type, int flavor, int rslow)
{
	qitem_t		*q;
	struct stat	sb;
	char		*callflavs = cfgs( CFG_CALLONFLAVORS );
	int		f = ( 1 << (flavor - 1)), canpoll = 0;
	int		cflav = 0;
	
	DEBUG(('Q',4,"q_each: lo '%s'",fname));

	cflav = (f & Q_CANPOLL) ? (strchr( callflavs, tolower( Q_CHARS[flavor - 1] ))
		|| strchr( callflavs, toupper( Q_CHARS[flavor - 1] ))) : 0; 

	q = q_add( fa );
	q->touched = 1;
	q->what |= type;

	switch( type ) {
	case T_REQ:
		if ( !stat( fname, &sb ))
			q->reqs = sb.st_size;
		break;

	case T_NETMAIL:
		if ( !stat( fname, &sb )) {
			q->pkts = sb.st_size;
			canpoll = 1;
		}
		break;
	
	case T_ARCMAIL:
		canpoll = !q_recountflo( fname, &q->sizes[flavor-1], &q->times[flavor-1], rslow );
		break;
	}

	q->canpoll |= ( cflav && canpoll && (f & Q_CANPOLL));
	q->flv |= f;
	DEBUG(('Q',4,"q_each: canpoll %d, q->canpoll %d, q->flv 0x%X(0x%X)",canpoll,q->canpoll,q->flv,f));
}


off_t q_sum(const qitem_t *q)
{
	int i;
	off_t total = 0;

	for( i = 0; i < 6; i++ )
		if ( q->times[i] )
			total += q->sizes[i];
	return total;
}


void rescan_boxes(int rslow)
{
	faslist_t *i;
	qitem_t *q;
	DIR *d;
	struct dirent *de;
	FTNADDR_T(a);
	char *p,rev[40],flv;
	int len,n;
	DEBUG(('Q',3,"rescan_boxes"));
	for(i=cfgfasl(CFG_FILEBOX);i;i=i->next) {
		q=q_add(&i->addr);
		q_recountbox(i->str,&q->sizes[4],&q->times[4],rslow);
		if(q->sizes[4]!=0) {
			q->touched=1;
			q->flv|=Q_HOLD;
			q->what|=T_ARCMAIL;
		}
	}
	if(cfgs(CFG_LONGBOXPATH)) {
		if((d=opendir(ccs))!=0) {
			while((de=readdir(d))) {
				if ( de->d_name[0] == '.' )
					continue;
				n=sscanf(de->d_name,"%d.%d.%d.%d.%c",&a.z,&a.n,&a.f,&a.p,&flv);
				if(n)DEBUG(('Q',4,"found lbox '%s', parse: %d:%d/%d.%d (%c) rc=%d",de->d_name,a.z,a.n,a.f,a.p,n==5?flv:'*',n));
				if(n==4||n==5) {
					if(n==4)snprintf(rev,40,"%d.%d.%d.%d",a.z,a.n,a.f,a.p);
					    else snprintf(rev,40,"%d.%d.%d.%d.%c",a.z,a.n,a.f,a.p,flv);
					if(!strcmp(de->d_name,rev)) {
						len=strlen(cfgs(CFG_LONGBOXPATH))+2+strlen(de->d_name);
						p=xmalloc(len);
						snprintf(p,len,"%s/%s",ccs,de->d_name);
						q=q_add(&a);
						q_recountbox(p,&q->sizes[5],&q->times[5],rslow);
						if(q->sizes[5]!=0) {
							q->touched=1;
							q->what|=T_ARCMAIL;
							cfgs(CFG_DEFBOXFLV);
							if(n==4)flv=(ccs&&*ccs)?(*ccs):'h';
							switch(tolower(flv)) {
							    case 'h': q->flv|=Q_HOLD;break;
							    case 'n':
							    case 'f': q->flv|=Q_NORM;break;
							    case 'd': q->flv|=Q_DIR;break;
							    case 'c': q->flv|=Q_CRASH;break;
							    case 'i': q->flv|=Q_IMM;break;
							    default : write_log("unknown longbox flavour '%c' for dir %s",flv,de->d_name);
							}
						}
						xfree(p);
					} else DEBUG(('Q',1,"strange: find: '%s', calc: '%s'",de->d_name,rev));
				} else if(*de->d_name!='.')DEBUG(('Q',2,"bad longbox name %s",de->d_name));
			}
			closedir(d);
		} else write_log("can't open %s: %s",ccs,strerror(errno));
	}
}


int q_rescan(qitem_t **curr, int rslow)
{
    sts_t sts;
    qitem_t *q, **p;
    time_t rescan_time;

    DEBUG(('Q',2,"rescan queue (%d)",rslow));
    rescan_time = time( NULL );

    for( q = q_queue; q; q = q->next ) {
        q->flv &= Q_DIAL;
        q->what = q->touched = q->canpoll = 0;
    }

    if ( outbound_rescan( q_each, rslow ) == 0 )
        return 0;

    rescan_boxes( rslow );
    p = &q_queue;
    qqreset();
    while(( q = *p )) {
        if ( !q->touched ) {
            *p = q->next;
            if ( q == *curr )
                *curr = *p;
            xfree( q );
        } else {
            outbound_getstatus( &q->addr, &sts );
            q->flv |= sts.flags;
            q->try = sts.try;
            if ( sts.htime > time(NULL) )
                q->onhold = sts.htime;
            else
                q->flv &= ~Q_ANYWAIT;

            qpqueue( &q->addr, q->pkts, q_sum( q ) + q->reqs, q->try, q->flv );
            p = &( (*p)->next );
        }
    }

    if ( !*curr )
        *curr = q_queue;

    rescan_time = time( NULL ) - rescan_time;

    DEBUG(('Q',2,"rescan done in %d seconds", rescan_time));
    return 1;
}


void qsendqueue(void)
{
	qitem_t *q;

	qqreset();
	
	for( q = q_queue; q ; q = q->next )
		qpqueue( &q->addr, q->pkts, q_sum( q ) + q->reqs, q->try, q->flv );
}
