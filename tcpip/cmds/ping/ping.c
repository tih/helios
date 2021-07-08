/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifdef lint
char copyright[] =
"@(#) Copyright (c) 1987 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifdef lint
static char sccsid[] = "@(#)ping.c	4.10 (Berkeley) 10/10/88";
#endif /* not lint */

/*
 *			P I N G . C
 *
 * Using the InterNet Control Message Protocol (ICMP) "ECHO" facility,
 * measure round-trip-delays and packet loss across network paths.
 *
 * Author -
 *	Mike Muuss
 *	U. S. Army Ballistic Research Laboratory
 *	December, 1983
 * Modified at Uc Berkeley
 *
 * Status -
 *	Public Domain.  Distribution Unlimited.
 *
 * Bugs -
 *	More statistics could always be gathered.
 *	This program has to run SUID to ROOT to access the ICMP socket.
 */

#include <stdio.h>
#include <errno.h>
#include <sys/time.h>

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/file.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>

#include <stdlib.h>
#include <strings.h>
  
extern char * inet_ntoa( long );
extern long   inet_addr( const char * );
extern void   setlinebuf( FILE * );

  
#define	MAXWAIT		10	/* max time to wait for response, sec. */
#define	MAXPACKET	4096	/* max packet size */
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN	64
#endif

int	verbose;
u_char	packet[MAXPACKET];
int	options;
extern	int errno;

int s;			/* Socket file descriptor */
struct hostent *hp;	/* Pointer to host info */
struct timezone tz;	/* leftover */

struct sockaddr whereto;/* Who to ping */
int datalen;		/* How much data */

char usage[] = "Usage:  ping [-drv] host [data size] [npackets]\n";

char *hostname;
char hnamebuf[MAXHOSTNAMELEN];

int npackets;
int ntransmitted = 0;		/* sequence # for outbound packets = #sent */
int ident;

int nreceived = 0;		/* # of packets we got back */
int timing = 0;
long tmin = 999999999;
long tmax = 0;
long tsum = 0;			/* sum of all times, for doing average */
void finish( int a );
void catcher( int a );


/*
 *			I N _ C K S U M
 *
 * Checksum routine for Internet Protocol family headers (C Version)
 *
 */
int
in_cksum(addr, len)
u_short *addr;
int len;
{
	register int nleft = len;
	register u_short *w = addr;
	register u_short answer;
	register int sum = 0;
	u_short odd_byte = 0;

	/*
	 *  Our algorithm is simple, using a 32 bit accumulator (sum),
	 *  we add sequential 16 bit words to it, and at the end, fold
	 *  back all the carry bits from the top 16 bits into the lower
	 *  16 bits.
	 */
	while( nleft > 1 )  {
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if( nleft == 1 ) {
		*(u_char *)(&odd_byte) = *(u_char *)w;
		sum += odd_byte;
	}

	/*
	 * add back carry outs from top 16 bits to low 16 bits
	 */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return (answer);
}

/*
 * 			P I N G E R
 * 
 * Compose and transmit an ICMP ECHO REQUEST packet.  The IP packet
 * will be added on by the kernel.  The ID field is our UNIX process ID,
 * and the sequence number is an ascending integer.  The first 8 bytes
 * of the data portion are used to hold a UNIX "timeval" struct in VAX
 * byte-order, to compute the round-trip time.
 */
void
pinger()
{
	static u_char outpack[MAXPACKET];
	register struct icmp *icp = (struct icmp *) outpack;
	int i, cc;
	register struct timeval *tp = (struct timeval *) &outpack[8];
	register u_char *datap = &outpack[8+sizeof(struct timeval)];

	icp->icmp_type = ICMP_ECHO;
	icp->icmp_code = 0;
	icp->icmp_cksum = 0;
	icp->icmp_seq = ntransmitted++;
	icp->icmp_id = ident;		/* ID */

	cc = datalen+8;			/* skips ICMP portion */

	if (timing)
		gettimeofday( tp, &tz );

	for( i=8; i<datalen; i++)	/* skip 8 for time */
		*datap++ = i;

	/* Compute ICMP checksum here */
	icp->icmp_cksum = in_cksum( (u_short *)icp, cc );

	/* cc = sendto(s, msg, len, flags, to, tolen) */
	i = sendto( s, (char *)outpack, cc, 0, &whereto, sizeof(struct sockaddr) );

	if( i < 0 || i != cc )  {
		if( i<0 )  perror("sendto");
		printf("ping: wrote %s %d chars, ret=%d\n",
			hostname, cc, i );
		fflush(stdout);
	}
}

/*
 * 			C A T C H E R
 * 
 * This routine causes another PING to be transmitted, and then
 * schedules another SIGALRM for 1 second from now.
 * 
 * Bug -
 * 	Our sense of time will slowly skew (ie, packets will not be launched
 * 	exactly at 1-second intervals).  This does not affect the quality
 *	of the delay and loss statistics.
 */
void
catcher( int a )
{
	long waittime;


	/* IOdebug( "catcher: called" ); */
	
	pinger();
	
	if (npackets == 0 || ntransmitted < npackets)
	  {
	    alarm( 1 );
	  }
	else {
		if (nreceived) {
			waittime = 2 * tmax / 1000;
			if (waittime == 0)
				waittime = 1;
		} else
			waittime = MAXWAIT;
		
		signal(SIGALRM, finish);
		
		alarm((int)waittime);
	}
}

/*
 * 			P R _ T Y P E
 *
 * Convert an ICMP "type" field to a printable string.
 */
char *
pr_type( t )
register int t;
{
	static char *ttab[] = {
		"Echo Reply",
		"ICMP 1",
		"ICMP 2",
		"Dest Unreachable",
		"Source Quence",
		"Redirect",
		"ICMP 6",
		"ICMP 7",
		"Echo",
		"ICMP 9",
		"ICMP 10",
		"Time Exceeded",
		"Parameter Problem",
		"Timestamp",
		"Timestamp Reply",
		"Info Request",
		"Info Reply"
	};

	if( t < 0 || t > 16 )
		return("OUT-OF-RANGE");

	return(ttab[t]);
}

/*
 * 			T V S U B
 * 
 * Subtract 2 timeval structs:  out = out - in.
 * 
 * Out is assumed to be >= in.
 */
void
tvsub( out, in )
register struct timeval *out, *in;
{
	if( (out->tv_usec -= in->tv_usec) < 0 )
	  {
		out->tv_sec--;
		out->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}

/*
 *			P R _ P A C K
 *
 * Print out the packet, if it came from us.  This logic is necessary
 * because ALL readers of the ICMP socket get a copy of ALL ICMP packets
 * which arrive ('tis only fair).  This permits multiple copies of this
 * program to be run without having intermingled output (or statistics!).
 */
void
pr_pack( buf, cc, from )
  char *buf;
  int cc;
  struct sockaddr_in *from;
{
	struct ip *ip;
	register struct icmp *icp;
	register long *lp = (long *) packet;
	register int i;
	struct timeval tv;
	struct timeval *tp;
	int hlen;
	long	triptime;

	from->sin_addr.s_addr = ntohl( from->sin_addr.s_addr );
	gettimeofday( &tv, &tz );

	ip = (struct ip *) buf;
	hlen = ip->ip_hl << 2;
	if (cc < hlen + ICMP_MINLEN) {
		if (verbose)
			printf("packet too short (%d bytes) from %s\n", cc,
				inet_ntoa(ntohl(from->sin_addr.s_addr)));
		return;
	}
	cc -= hlen;
	icp = (struct icmp *)(buf + hlen);
	if( icp->icmp_type != ICMP_ECHOREPLY )  {
		if (verbose) {
			printf("%d bytes from %s: ", cc,
				inet_ntoa(ntohl(from->sin_addr.s_addr)));
			printf("icmp_type=%d (%s)\n",
				icp->icmp_type, pr_type(icp->icmp_type) );
			for( i=0; i<12; i++)
			    printf("x%2.2x: x%8.8lx\n", i*sizeof(long), *lp++ );
			printf("icmp_code=%d\n", icp->icmp_code );
		}
		return;
	}
	if( icp->icmp_id != ident )
		return;			/* 'Twas not our ECHO */

	tp = (struct timeval *)&icp->icmp_data[0];
	printf("%d bytes from %s: ", cc,
		inet_ntoa(ntohl(from->sin_addr.s_addr)));
	printf("icmp_seq=%d. ", icp->icmp_seq );
	if (timing) {
		tvsub( &tv, tp );
		triptime = tv.tv_sec*1000+(tv.tv_usec/1000);
		printf("time=%ld. ms\n", triptime );
		tsum += triptime;
		if( triptime < tmin )
			tmin = triptime;
		if( triptime > tmax )
			tmax = triptime;
	} else
		putchar('\n');
	nreceived++;
}



/*
 *			F I N I S H
 *
 * Print out statistics, and give up.
 * Heavily buffered STDIO is used here, so that all the statistics
 * will be written with 1 sys-write call.  This is nice when more
 * than one copy of the program is running on a terminal;  it prevents
 * the statistics output from becomming intermingled.
 */
void
finish( int a )
{
	printf("\n----%s PING Statistics----\n", hostname );
	printf("%d packets transmitted, ", ntransmitted );
	printf("%d packets received, ", nreceived );
	if (ntransmitted)
	    printf("%d%% packet loss",
		(int) (((ntransmitted-nreceived)*100) / ntransmitted ) );
	printf("\n");
	if (nreceived && timing)
	    printf("round-trip (ms)  min/avg/max = %ld/%ld/%ld\n",
		tmin,
		tsum / nreceived,
		tmax );
	fflush(stdout);
	exit(0);
}

/*
 * 			M A I N
 */
int
main(argc, argv)
  int	   argc;
  char *   argv[];
{
  struct sockaddr_in from;
  char **av = argv;
  char *toaddr = NULL;
  struct sockaddr_in *to = (struct sockaddr_in *) &whereto;
  char on = 1;
  struct protoent *proto;
  
  
  argc--, av++;
  
  while (argc > 0 && *av[0] == '-')
    {
      while (*++av[0]) switch (*av[0])
	{
	case 'd':
	  options |= SO_DEBUG;
	  break;
	case 'r':
	  options |= SO_DONTROUTE;
	  break;
	case 'v':
	  verbose++;
	  break;
	}
      argc--, av++;
    }
  
  if( argc < 1)
    {
      printf(usage);
      exit(1);
    }

  bzero( (char *)&whereto, sizeof(struct sockaddr) );
  
  to->sin_family = AF_INET;
  to->sin_addr.s_addr = inet_addr(av[0]);
  
  if (to->sin_addr.s_addr != -1)
    {
      strcpy(hnamebuf, av[0]);
      hostname = hnamebuf;
    }
  else
    {
      hp = gethostbyname(av[0]);

      if (!hp)
	{
	  fprintf(stderr, "ping: %s: ", av[0]);
	  herror((char *)NULL);
	  exit(1);
	}
      to->sin_family = hp->h_addrtype;
      bcopy(hp->h_addr, (caddr_t)&to->sin_addr, hp->h_length);
      hostname = hp->h_name;
      toaddr = inet_ntoa(to->sin_addr.s_addr);
    }
  
  if( argc >= 2 )
    datalen = atoi( av[1] );
  else
    datalen = 64-8;
  
  if (datalen > MAXPACKET)
    {
      fprintf(stderr, "ping: packet size too large\n");
      exit(1);
    }
  
  if (datalen >= sizeof(struct timeval))
    timing = 1;
  
  if (argc > 2)
    npackets = atoi(av[2]);
  
  ident = getpid() & 0xFFFF;
  
  if ((proto = getprotobyname("icmp")) == NULL)
    {
      fprintf(stderr, "icmp: unknown protocol\n");
      exit(10);
    }
  
  if ((s = socket(AF_INET, SOCK_RAW, proto->p_proto)) < 0)
    {
      perror("ping: socket");
      exit(5);
    }
  
  if (options & SO_DEBUG)
    setsockopt(s, SOL_SOCKET, SO_DEBUG, &on, sizeof(on));
  
  if (options & SO_DONTROUTE)
    setsockopt(s, SOL_SOCKET, SO_DONTROUTE, &on, sizeof(on));
  
  printf("PING %s", hostname);
  if (toaddr)
    printf(" (%s)", toaddr);
  
  printf(": %d data bytes\n", datalen);
  
  setlinebuf( stdout );
  
  /*setsigstacksize(10000);*/
  
  signal( SIGINT, finish );
  signal( SIGALRM, catcher);
  
  catcher( 1 );	/* start things going */
  
  for (;;)
    {
      int len = sizeof (packet);
      int fromlen = sizeof (from);
      int cc;

      /* IOdebug( "receive" ); */
      
      if ( (cc=recvfrom(s, (char *)packet, len, 0, (struct sockaddr *)&from, &fromlen)) < 0)
	{
	  if ( errno == EINTR )
	    continue;
	  perror("ping: recvfrom");
	  continue;
	}

      /* IOdebug( "printing" ); */
      
      pr_pack( (char *)packet, cc, &from );

      /* IOdebug( "printed" ); */
      
      if (npackets && nreceived >= npackets)
	finish( 1 );
    }
  
  /*NOTREACHED*/
} /* main */
