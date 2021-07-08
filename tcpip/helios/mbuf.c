/*
 * Copyright (c) 1982, 1986, 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 *	@(#)uipc_mbuf.c	7.4.1.2 (Berkeley) 2/8/88
 */

#include "../machine/pte.h"

#include "param.h"
#include "dir.h"
#include "user.h"
#include "proc.h"
#include "cmap.h"
#include "map.h"
#include "mbuf.h"
#include "vm.h"
#include "kernel.h"
#include "syslog.h"
#include "domain.h"
#include "protosw.h"

#ifdef __HELIOS
static int mbutl_hwm;
struct  mbuf *mbutl;           /* virtual address of net free mem */   
struct  mbstat mbstat;                                                  
int     nmbclusters;                                                    
struct  mbuf *mfree, *mclfree;                                          
char    mclrefcnt[NMBCLUSTERS + 1];                                     
int     m_want; 
#endif

mbinit()
{
	int s;

#if CLBYTES < 4096
#define NCL_INIT	(4096/CLBYTES)
#else
#define NCL_INIT	1
#endif
#ifdef __HELIOS
	/* get ALL mbuf memory here for now */
	mbutl = (struct mbuf *)Malloc(NMBCLUSTERS*MCLBYTES+MCLBYTES);
	/* adjust mbutl to a 1024 byte boundary	*/
	mbutl = (struct mbuf *)((((int)mbutl) + MCLBYTES - 1) &~ MCLOFSET);
	mbutl_hwm = 0;
#endif
	s = splimp();
	if (m_clalloc(NCL_INIT, MPG_MBUFS, M_DONTWAIT) == 0)
		goto bad;
	if (m_clalloc(NCL_INIT, MPG_CLUSTERS, M_DONTWAIT) == 0)
		goto bad;
	splx(s);
	return;
bad:
	panic("mbinit");
}

/*
 * Must be called at splimp.
 */
/* ARGSUSED */
caddr_t
m_clalloc(
	  register int ncl,
	  int how,
	  int canwait )
{
#ifndef __HELIOS
	int npg;
#endif
	int mbx;
	register struct mbuf *m;
	register int i;
	static int logged;
	
#ifdef __HELIOS
/*IOdebug("m_clalloc(%d,%d,%d) mbutl %x hwm %d",ncl,how,canwait,mbutl,mbutl_hwm);*/
	mbx = mbutl_hwm;
	if( mbx+ncl >= NMBCLUSTERS )
	{
		if (logged == 0) {
			logged++;
			log(LOG_ERR, "mbuf map full\n");
		}
		return (0);
	}
	mbutl_hwm += ncl;
	m = cltom(mbx * NBPG / MCLBYTES);
/*IOdebug("mbutl_hwm %d %",mbutl_hwm);*/
#else
	npg = ncl * CLSIZE;
	mbx = rmalloc(mbmap, (long)npg);
	if (mbx == 0) {
		if (logged == 0) {
			logged++;
			log(LOG_ERR, "mbuf map full\n");
		}
		return (0);
	}
	m = cltom(mbx * NBPG / MCLBYTES);
	if (memall(&Mbmap[mbx], npg, proc, CSYS) == 0) {
		rmfree(mbmap, (long)npg, (long)mbx);
		return (0);
	}
	vmaccess(&Mbmap[mbx], (caddr_t)m, npg);
#endif
	switch (how) {

	case MPG_CLUSTERS:
		ncl = ncl * CLBYTES / MCLBYTES;
		for (i = 0; i < ncl; i++) {
			m->m_off = 0;
			m->m_next = mclfree;
			mclfree = m;
			m += MCLBYTES / sizeof (*m);
			mbstat.m_clfree++;
		}
		mbstat.m_clusters += ncl;
		break;

	case MPG_MBUFS:
		for (i = ncl * CLBYTES / sizeof (*m); i > 0; i--) {
			m->m_off = 0;
			m->m_type = MT_DATA;
			mbstat.m_mtypes[MT_DATA]++;
			mbstat.m_mbufs++;
			(void) m_free(m);
			m++;
		}
		break;

	case MPG_SPACE:
		mbstat.m_space++;
		break;
	}
	return ((caddr_t)m);
}

m_pgfree(
	 caddr_t addr,
	 int n )
{

#ifdef lint
	addr = addr; n = n;
#endif
}

/*
 * Must be called at splimp.
 */
m_expand(int canwait)
{
	register struct domain *dp;
	register struct protosw *pr;
	int tries;

	for (tries = 0;; ) {
		if (m_clalloc(1, MPG_MBUFS, canwait))
			return (1);
		if (canwait == 0 || tries++)
			return (0);

		/* ask protocols to free space */
		for (dp = domains; dp; dp = dp->dom_next)
			for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW;
			    pr++)
				if (pr->pr_drain)
					(*pr->pr_drain)();
		mbstat.m_drain++;
	}
}

/* NEED SOME WAY TO RELEASE SPACE */

/*
 * Space allocation routines.
 * These are also available as macros
 * for critical paths.
 */
struct mbuf *
m_get(
      int canwait,
      int type )
{
	register struct mbuf *m;

	MGET(m, canwait, type);
	return (m);
}

struct mbuf *
m_getclr(
	 int canwait,
	 int type )
{
	register struct mbuf *m;

	MGET(m, canwait, type);
	if (m == 0)
		return (0);
	bzero(mtod(m, caddr_t), MLEN);
	return (m);
}

struct mbuf *
m_free(struct mbuf *m)
{
	register struct mbuf *n;

	MFREE(m, n);
	return (n);
}

/*
 * Get more mbufs; called from MGET macro if mfree list is empty.
 * Must be called at splimp.
 */
/*ARGSUSED*/
struct mbuf *
m_more(
       int canwait,
       int type )
{
	register struct mbuf *m;

	while (m_expand(canwait) == 0) {
		if (canwait == M_WAIT) {
			mbstat.m_wait++;
			m_want++;
			sleep((caddr_t)&mfree, PZERO - 1);
		} else {
			mbstat.m_drops++;
			return (NULL);
		}
	}
#define m_more(x,y) (panic("m_more"), (struct mbuf *)0)
	MGET(m, canwait, type);
#undef m_more
	return (m);
}

m_freem(register struct mbuf *m)
{
	register struct mbuf *n;
	register int s;

	if (m == NULL)
		return;
	s = splimp();
	do {
		MFREE(m, n);
	} while (m = n);
	splx(s);
}

/*
 * Mbuffer utility routines.
 */

/*
 * Make a copy of an mbuf chain starting "off" bytes from the beginning,
 * continuing for "len" bytes.  If len is M_COPYALL, copy to end of mbuf.
 * Should get M_WAIT/M_DONTWAIT from caller.
 */
struct mbuf *
m_copy(
       register struct mbuf *m,
       int off,
       register int len )
{
	register struct mbuf *n, **np;
	struct mbuf *top, *p;

	if (len == 0)
		return (0);
	if (off < 0 || len < 0)
		panic("m_copy");
	while (off > 0) {
		if (m == 0)
			panic("m_copy");
		if (off < m->m_len)
			break;
		off -= m->m_len;
		m = m->m_next;
	}
	np = &top;
	top = 0;
	while (len > 0) {
		if (m == 0) {
			if (len != M_COPYALL)
				panic("m_copy");
			break;
		}
		MGET(n, M_DONTWAIT, m->m_type);
		*np = n;
		if (n == 0)
			goto nospace;
		n->m_len = MIN(len, m->m_len - off);
		if (m->m_off > MMAXOFF) {
			p = mtod(m, struct mbuf *);
			n->m_off = ((long)p - (long)n) + off;
			mclrefcnt[mtocl(p)]++;
		} else
			bcopy(mtod(m, caddr_t)+off, mtod(n, caddr_t),
			    (unsigned)n->m_len);
		if (len != M_COPYALL)
			len -= n->m_len;
		off = 0;
		m = m->m_next;
		np = &n->m_next;
	}
	return (top);
nospace:
	m_freem(top);
	return (0);
}

m_cat(
      register struct mbuf * m,
      register struct mbuf * n )
{
	while (m->m_next)
		m = m->m_next;
	while (n) {
		if (m->m_off >= MMAXOFF ||
		    m->m_off + m->m_len + n->m_len > MMAXOFF) {
			/* just join the two chains */
			m->m_next = n;
			return;
		}
		/* splat the data from one into the other */
		bcopy(mtod(n, caddr_t), mtod(m, caddr_t) + m->m_len,
		    (u_int)n->m_len);
		m->m_len += n->m_len;
		n = m_free(n);
	}
}

m_adj(
	struct mbuf *mp,
	register int len )
{
	register struct mbuf *m;
	register count;

	if ((m = mp) == NULL)
		return;
	if (len >= 0) {
		while (m != NULL && len > 0) {
			if (m->m_len <= len) {
				len -= m->m_len;
				m->m_len = 0;
				m = m->m_next;
			} else {
				m->m_len -= len;
				m->m_off += len;
				break;
			}
		}
	} else {
		/*
		 * Trim from tail.  Scan the mbuf chain,
		 * calculating its length and finding the last mbuf.
		 * If the adjustment only affects this mbuf, then just
		 * adjust and return.  Otherwise, rescan and truncate
		 * after the remaining size.
		 */
		len = -len;
		count = 0;
		for (;;) {
			count += m->m_len;
			if (m->m_next == (struct mbuf *)0)
				break;
			m = m->m_next;
		}
		if (m->m_len >= len) {
			m->m_len -= len;
			return;
		}
		count -= len;
		/*
		 * Correct length for chain is "count".
		 * Find the mbuf with last data, adjust its length,
		 * and toss data from remaining mbufs on chain.
		 */
		for (m = mp; m; m = m->m_next) {
			if (m->m_len >= count) {
				m->m_len = count;
				break;
			}
			count -= m->m_len;
		}
		while (m = m->m_next)
			m->m_len = 0;
	}
}

/*
 * Rearange an mbuf chain so that len bytes are contiguous
 * and in the data area of an mbuf (so that mtod and dtom
 * will work for a structure of size len).  Returns the resulting
 * mbuf chain on success, frees it and returns null on failure.
 * If there is room, it will add up to MPULL_EXTRA bytes to the
 * contiguous region in an attempt to avoid being called next time.
 */
struct mbuf *
m_pullup(
	register struct mbuf *n,
	int len )
{
	register struct mbuf *m;
	register int count;
	int space;

	if (n->m_off + len <= MMAXOFF && n->m_next) {
		m = n;
		n = n->m_next;
		len -= m->m_len;
	} else {
		if (len > MLEN)
			goto bad;
		MGET(m, M_DONTWAIT, n->m_type);
		if (m == 0)
			goto bad;
		m->m_len = 0;
	}
	space = (int)( MMAXOFF - m->m_off);
	do {
		count = MIN(MIN(space - m->m_len, len + MPULL_EXTRA), n->m_len);
		bcopy(mtod(n, caddr_t), mtod(m, caddr_t)+m->m_len,
		  (unsigned)count);
		len -= count;
		m->m_len += count;
		n->m_len -= count;
		if (n->m_len)
			n->m_off += count;
		else
			n = m_free(n);
	} while (len > 0 && n);
	if (len > 0) {
		(void) m_free(m);
		goto bad;
	}
	m->m_next = n;
	return (m);
bad:
	m_freem(n);
	return (0);
}

#ifdef __HELIOS

#ifdef MBUF_INFO

static char *typenames[] =
{
"free","data","header","socket","pcb","rtable",
"htable","atable","soname","zombie","soopts",
"ftable","rights","ifaddr"
};

int mbuf_info()
{
	int i;
	int total = mbstat.m_mbufs + mbstat.m_clusters*4;
	int free = mbstat.m_mtypes[MT_FREE] + mbstat.m_clfree*4;
	int usage = ((total-free)*100)/total;
	IOdebug("mbuf clusters %d hwm %d",NMBCLUSTERS,mbutl_hwm);
	IOdebug("mbufs %d %",mbstat.m_mbufs);
	IOdebug("clusters %d %",mbstat.m_clusters);
	IOdebug("space %d %",mbstat.m_space);
	IOdebug("clfree %d",mbstat.m_clfree);
	IOdebug("drops %d %",mbstat.m_drops);
	IOdebug("wait %d %",mbstat.m_wait);
	IOdebug("drain %d %",mbstat.m_drain);
	IOdebug("usage %d%%",usage);
	for( i = 0; i <14; i++ ) 
	{
		if( mbstat.m_mtypes[i] != 0 )
			IOdebug("%s %d %",typenames[i],mbstat.m_mtypes[i]);
	}
	IOdebug("");
	IOdebug("Heap: free %d size %d",Malloc(-1),Malloc(-3));
}

#endif /* MBUF_INFO */

#endif
