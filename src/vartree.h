/**********************************************************
 * File: vartree.h
 * Created at Mon Nov 12 14:48:45 2001 by lev // lev@serebryakov.spb.ru
 * 
 * $Id: vartree.h,v 1.1 2002/03/16 15:59:39 lev Exp $
 **********************************************************/
#ifndef __VARTREE_H__
#define __VARTREE_H__

#define UNIVAR_TYPE_NULL	'n'			/* Tree node without its own value */
#define UNIVAR_TYPE_CHAR	'c'			/* One character -- typically flag */
#define UNIVAR_TYPE_STRING	's'			/* ASCIIZ String WITHOUT length */
#define UNIVAR_TYPE_BIN		'b'			/* Binary string with length */
#define UNIVAR_TYPE_INT		'd'			/* 32 bit signed integer */
#define UNIVAR_TYPE_ADDR	'a'			/* 4D FTN address -- four 16 bit integers */
#define UNIVAR_TYPE_OBJ		'o'			/* Object -- fixed sequence of variables of different types */
#define UNIVAR_TYPE_LIST	'l'			/* List -- grow sequence of varibales of same type */

/* Full ID -- number of components + all components */
typedef struct _uniid {
	INT32 l;
	INT32 *id;
} uniid_t;

/* List */
typedef struct _varlist {
	char type;
	int length;
	struct _unival *head;
	struct _unival *tail;
} varlist_t;

/* Object */
typedef struct _varobj {
	char *type;
	/* Really -- array */
	struct _unival *components;
} varobj_t;

/* Any value -- DOESN'T STORE TYPE! */
typedef struct _unival {
	enum _unival_value {
		CHAR	c;
		CHAR	*s;
		struct _unival_bin_val {
			UINT32	l;
			CHAR	*d;
		}		b;
		INT32	i;
		ftnaddr_t a;
        struct _varlist l;
		struct _varobj o;
	} v;
	/* To link them in list */
	struct _unival *n;
	struct _unival *p;
} unival_t;

typedef struct _treenode {
	INT32 id;
	char type;
	unival_t *v;
	/* List */
	struct _treenode *childs;
	/* Pointers */
	struct _treenode *next;
	struct _treenode *prev;
	/* Any data for callbacks */
	void *data;
} treenode_t;

treenode_t *vt_node_Create(INT32 id);
int vt_node_Destroy(treenode_t *node);
int vt_node_DestroyTree(treenode_t *root);
int vt_node_AddChildren(treenode_t *parent, treenode_t *node);
int vt_node_GetChildren(treenode_t *parent, INT32 id);
int vt_node_GetNode(treenode_t *root, uniid_t *id);


#endif/*__VARTREE_H__*/
