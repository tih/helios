/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Mike Olson.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)lruhash.c	5.1 (Berkeley) 1/23/91";
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include "lrucache.h"

#define HASH(l, pgno)	(pgno % l->lru_csize)

/*
 *  LRUHASHGET -- Look up a block in the LRU cache by page number.
 *
 *	Parameters:
 *		l -- LRU cache
 *		pgno -- number of the logical page to get
 *
 *	Returns:
 *		(CACHE_ENT *) pointer to the associated hash table entry
 *		(if any), or NULL (if none).
 */

CACHE_ENT *
lruhashget(l, pgno)
	LRUCACHE *l;
	int pgno;
{
	int hashind;
	CACHE_ENT *ce;

	hashind = HASH(l, pgno);

	/* walk the chain */
	for (ce = l->lru_cache[hashind];
	     ce != (CACHE_ENT *) NULL;
	     ce = ce->c_chain) {
		if (ce->c_pgno == pgno)
			return (ce);
	}

	return ((CACHE_ENT *) NULL);
}

/*
 *  LRUHASHPUT -- Add an entry for a given page to the cache hash table.
 *
 *	This routine assumes that the given page does not yet have an entry
 *	in the table.
 *
 *	Parameters:
 *		l -- LRU cache
 *		pgno -- logical page number for this entry
 *		lruent -- LRU list entry at which hash table entry should
 *			  point
 *
 *	Returns:
 *		(CACHE_ENT *) pointer to the new hash table entry on success,
 *		or NULL on failure.
 *
 *	Side Effects:
 *		Allocates memory which should be freed when the hash table
 *		entry is removed.
 */

CACHE_ENT *
lruhashput(l, pgno, lruent)
	LRUCACHE *l;
	int pgno;
	LRU_ENT *lruent;
{
	int hashind;
	CACHE_ENT *ce;

	if ((ce = (CACHE_ENT *) malloc((unsigned) sizeof(CACHE_ENT)))
	    == (CACHE_ENT *) NULL)
		return ((CACHE_ENT *) NULL);

	hashind = HASH(l, pgno);

	ce->c_pgno = pgno;
	ce->c_lruent = lruent;
	ce->c_chain = l->lru_cache[hashind];
	l->lru_cache[hashind] = ce;

	return (ce);
}

/*
 *  LRUHASHDEL -- Delete the entry for a given page from the LRU cache
 *		  hash table.
 *
 *	Parameters:
 *		l -- LRU cache
 *		pgno -- page number for which we should delete htable entry
 *
 *	Returns:
 *		Zero on success, -1 on failure.
 *
 *	Side Effects:
 *		Releases the memory occupied by the hash table entry.
 */

int
lruhashdel(l, pgno)
	LRUCACHE *l;
	int pgno;
{
	CACHE_ENT *ce;
	CACHE_ENT *sce;
	int hashind;

	sce = (CACHE_ENT *) NULL;
	hashind = HASH(l, pgno);

	/* find the entry we want to delete */
	for (ce = l->lru_cache[hashind];
	     ce != (CACHE_ENT *) NULL;
	     ce = ce->c_chain) {
		if (ce->c_pgno == pgno)
			break;
		sce = ce;
	}

	if (ce == (CACHE_ENT *) NULL)
		return (-1);

	/* remove it from the chain */
	if (sce == (CACHE_ENT *) NULL)
		l->lru_cache[hashind] = ce->c_chain;
	else
		sce->c_chain = ce->c_chain;

	/* release it */
	free ((char *) ce);

	/* success */
	return (0);
}
