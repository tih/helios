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
static char sccsid[] = "@(#)insert.c	5.2 (Berkeley) 2/18/91";
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include "db.h"
#include "btree.h"

/*
 *  _BT_INSERT -- Insert a new user datum in the btree.
 *
 *	This routine is called by bt_put, the public interface, once the
 *	location for the new item is known.  We do the work here, and
 *	handle splits if necessary.
 *
 *	Parameters:
 *		t -- btree in which to do the insertion.
 *		item -- BTITEM describing location of new datum
 *		key -- key to insert
 *		data -- data associated with key
 *		flag -- magic cookie passed recursively to bt_put if we
 *			have to do a split
 *
 *	Returns:
 *		RET_SUCCESS, RET_ERROR.
 */

#include <stdio.h>

int
_bt_insert(t, item, key, data, flag)
	BTREE_P t;
	BTITEM *item;
	DBT *key;
	DBT *data;
	int flag;
{
	index_t index;
	BTHEADER *h;
	DATUM *d;
	int nbytes;
	int status;
	pgno_t pgno;
	int keysize, datasize;
	int bigkey, bigdata;

	if (_bt_getpage(t, item->bti_pgno) == RET_ERROR)
		return (RET_ERROR);
	h = t->bt_curpage;

	if (TOOBIG(t, data->size)) {
		bigdata = TRUE;
		datasize = sizeof(pgno_t);
	} else {
		bigdata = FALSE;
		datasize = data->size;
	}

	if (TOOBIG(t, key->size)) {
		bigkey = TRUE;
		keysize = sizeof(pgno_t);
	} else {
		bigkey = FALSE;
		keysize = key->size;
	}

	nbytes = keysize + datasize + (2*sizeof(size_t) + sizeof(u_long));
	nbytes = (int) LONGALIGN(nbytes) + sizeof(index_t);

	/* if there's not enough room here, split the page */
	if ((h->h_upper - h->h_lower) < nbytes) {
		if (_bt_split(t) == RET_ERROR)
			return (RET_ERROR);

		/* okay, try again (empty the stack first, though) */
		while (_bt_pop((BTREE_P) t) != P_NONE)
			continue;

		return (bt_put((BTREE) t, key, data, flag));
	}

	/* put together a leaf page datum from the key/data pair */
	index = item->bti_index;
	nbytes = keysize + datasize + (2*sizeof(size_t) + sizeof(u_long));

	if ((d = (DATUM *) malloc((unsigned) nbytes)) == (DATUM *) NULL)
		return (RET_ERROR);

	d->d_ksize = keysize;
	d->d_dsize = datasize;
	d->d_flags = 0;

	if (bigkey) {
		if (_bt_indirect(t, key, &pgno) == RET_ERROR)
			return (RET_ERROR);
		(void) bcopy((char *) &pgno, (char *)&(d->d_bytes[0]), sizeof(pgno));
		d->d_flags |= D_BIGKEY;
		if (_bt_getpage(t, item->bti_pgno) == RET_ERROR)
			return (RET_ERROR);
	} else {
		if (d->d_ksize > 0) {
			(void) bcopy((char *) key->data,
				      (char *) &(d->d_bytes[0]),
				      (int) d->d_ksize);
		}
	}

	if (bigdata) {
		if (_bt_indirect(t, data, &pgno) == RET_ERROR)
			return (RET_ERROR);
		(void) bcopy((char *) &pgno,
			     (char *)&(d->d_bytes[keysize]),
			     sizeof(pgno));
		d->d_flags |= D_BIGDATA;
		if (_bt_getpage(t, item->bti_pgno) == RET_ERROR)
			return (RET_ERROR);
	} else {
		if (d->d_dsize > 0) {
			(void) bcopy((char *) data->data,
				      (char *) &(d->d_bytes[keysize]),
				      (int) d->d_dsize);
		}
	}

	/* do the insertion */
	status = _bt_insertat(t, (u_char *) d, index);

	(void) free((char *) d);

	return (status);
}

/*
 *  _BT_INSERTI -- Insert IDATUM on current page in the btree.
 *
 *	This routine handles insertions to internal pages after splits
 *	lower in the tree.  On entry, t->bt_curpage is the page to get
 *	the new IDATUM.  We are also given pgno, the page number of the
 *	IDATUM that is immediately left of the new IDATUM's position.
 *	This guarantees that the IDATUM for the right half of the page
 *	after a split goes next to the IDATUM for its left half.
 *
 *	Parameters:
 *		t -- tree in which to do insertion.
 *		id -- new IDATUM to insert
 *		pgno -- page number of IDATUM left of id's position
 *
 *	Returns:
 *		RET_SUCCESS, RET_ERROR.
 */

int
_bt_inserti(t, id, pgno)
	BTREE_P t;
	IDATUM *id;
	pgno_t pgno;
{
	BTHEADER *h = t->bt_curpage;
	index_t next, i;
	IDATUM *idx;
	u_char *key;
	pgno_t chain;
	int free_key;
	int ignore;

	if (id->i_flags & D_BIGKEY) {
		free_key = TRUE;
		bcopy((char *)&(id->i_bytes[0]), (char *) &chain, sizeof(chain));
		if (_bt_getbig(t, chain, &key, &ignore) == RET_ERROR)
			return (RET_ERROR);
	} else {
		free_key = FALSE;
		key = (u_char *)&(id->i_bytes[0]);
	}
	i = _bt_binsrch(t, key);

	next = (index_t) NEXTINDEX(h);
	while (i < next && _bt_cmp(t, key, i) >= 0)
		i++;

	if (free_key)
		(void) free(key);

	/* okay, now we're close; find adjacent IDATUM */
	for (;;) {
		idx = (IDATUM *) GETDATUM(h,i);
		if (idx->i_pgno == pgno) {
			i++;
			break;
		}
		--i;
	}

	/* correctly positioned, do the insertion */
	return (_bt_insertat(t, (u_char *) id, i));
}

/*
 *  _BT_INSERTAT -- Insert a datum at a given location on the current page.
 *
 *	This routine does insertions on both leaf and internal pages.
 *
 *	Parameters:
 *		t -- tree in which to do insertion.
 *		p -- DATUM or IDATUM to insert.
 *		index -- index in line pointer array to put this item.
 *
 *	Returns:
 *		RET_SUCCESS, RET_ERROR.
 *
 *	Side Effects:
 *		Will rearrange line pointers to make space for the new
 *		entry.  This means that any scans currently active are
 *		invalid after this.
 *
 *	Warnings:
 *		There must be sufficient room for the new item on the page.
 */

int
_bt_insertat(t, p, index)
	BTREE_P t;
	u_char *p;
	index_t index;
{
	IDATUM *id = (IDATUM *) p;
	DATUM *d = (DATUM *) p;
	BTHEADER *h;
	CURSOR *c;
	index_t nxtindex;
	u_char *src, *dest;
	int nbytes;

	/* insertion may confuse an active scan.  fix it. */
	c = &(t->bt_cursor);
	if (t->bt_flags & BTF_SEQINIT && t->bt_curpage->h_pgno == c->c_pgno)
		if (_bt_fixscan(t, index, d, INSERT) == RET_ERROR)
			return (RET_ERROR);

	h = t->bt_curpage;
	nxtindex = (index_t) NEXTINDEX(h);

	/*
	 *  If we're inserting at the middle of the line pointer array,
	 *  copy pointers that will follow the new one up on the page.
	 */

	if (index < nxtindex) {
		src = (u_char *) &(h->h_linp[index]);
		dest = (u_char *) &(h->h_linp[index + 1]);
		nbytes = (int) (h->h_lower - (src - ((u_char *) h)))
			 + sizeof(h->h_linp[0]);
		(void) bcopy((char *)src, (char *)dest, nbytes);
	}

	/* compute size and copy data to page */
	if (h->h_flags & F_LEAF) {
		nbytes = d->d_ksize + d->d_dsize
			 + (2*sizeof(size_t) + sizeof(u_long));
	} else {
		nbytes = id->i_size + (sizeof(size_t) + sizeof(pgno_t)
				+ sizeof(u_long));
	}
	dest = (((u_char *) h) + h->h_upper) - LONGALIGN(nbytes);
	(void) bcopy((char *) p, (char *)dest, nbytes);

	/* update statistics */
	dest -= (int) h;
	h->h_linp[index] = (index_t) dest;
	h->h_upper = (index_t) dest;
	h->h_lower += sizeof(index_t);

	/* we're done */
	h->h_flags |= F_DIRTY;

	return (RET_SUCCESS);
}
