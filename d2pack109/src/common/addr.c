/*
 * Copyright (C) 1999,2000,2001  Ross Combs (rocombs@cs.nmsu.edu)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#define ADDR_INTERNAL_ACCESS
#include "common/setup_before.h"
#ifdef HAVE_STDDEF_H
# include <stddef.h>
#else
# ifndef NULL
#  define NULL ((void *)0)
# endif
#endif
#ifdef STDC_HEADERS
# include <stdlib.h>
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# endif
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#ifdef HAVE_MEMORY_H
# include <memory.h>
#endif
#include "compat/memset.h"
#include "compat/memcpy.h"
#include "compat/strrchr.h"
#include "compat/strdup.h"
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#include "compat/socket.h"
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#include "compat/netinet_in.h"
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#include "compat/inet_aton.h"
#include "compat/inet_ntoa.h"
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif
#include "compat/psock.h"
#include "common/eventlog.h"
#include "common/list.h"
#include "common/util.h"
#include "common/addr.h"
#include "common/setup_after.h"


static char const * netaddr_num_to_addr_str(unsigned int netipaddr, unsigned int netmask);


#define HACK_SIZE 4

/* both arguments are in host byte order */
extern char const * addr_num_to_addr_str(unsigned int ipaddr, unsigned short port)
{
    static unsigned int curr=0;
    static char         temp[HACK_SIZE][64];
    struct sockaddr_in  tsa;
    
    curr = (curr+1)%HACK_SIZE;
    
    memset(&tsa,0,sizeof(tsa));
    tsa.sin_family = PSOCK_AF_INET;
    tsa.sin_port = htons((unsigned short)0);
    tsa.sin_addr.s_addr = htonl(ipaddr);
    sprintf(temp[curr],"%.32s:%hu",inet_ntoa(tsa.sin_addr),port);
    
    return temp[curr];
}


/* ipaddr is in host byte order */
extern char const * addr_num_to_ip_str(unsigned int ipaddr)
{
    static unsigned int curr=0;
    static char         temp[HACK_SIZE][64];
    struct sockaddr_in  tsa;
    
    curr = (curr+1)%HACK_SIZE;
    
    memset(&tsa,0,sizeof(tsa));
    tsa.sin_family = PSOCK_AF_INET;
    tsa.sin_port = htons((unsigned short)0);
    tsa.sin_addr.s_addr = htonl(ipaddr);
    sprintf(temp[curr],"%.32s",inet_ntoa(tsa.sin_addr));
    
    return temp[curr];
}


static char const * netaddr_num_to_addr_str(unsigned int netipaddr, unsigned int netmask)
{
    static unsigned int curr=0;
    static char         temp[HACK_SIZE][64];
    struct sockaddr_in  tsa;
    
    curr = (curr+1)%HACK_SIZE;
    
    memset(&tsa,0,sizeof(tsa));
    tsa.sin_family = PSOCK_AF_INET;
    tsa.sin_port = htons((unsigned short)0);
    tsa.sin_addr.s_addr = htonl(netipaddr);
    sprintf(temp[curr],"%.32s/0x%08x",inet_ntoa(tsa.sin_addr),netmask);
    
    return temp[curr];
}



extern char const * host_lookup(char const * hoststr, unsigned int * ipaddr)
{
    struct sockaddr_in tsa;
#ifdef HAVE_GETHOSTBYNAME
    struct hostent *   hp;
#endif
    
    if (!hoststr)
    {
	eventlog(eventlog_level_error,"host_lookup","got NULL hoststr");
	return NULL;
    }
    if (!ipaddr)
    {
	eventlog(eventlog_level_error,"host_lookup","got NULL ipaddr");
	return NULL;
    }
    
    memset(&tsa,0,sizeof(tsa));
    tsa.sin_family = PSOCK_AF_INET;
    tsa.sin_port = htons(0);
    
#ifdef HAVE_GETHOSTBYNAME
#ifdef WIN32
    psock_init();
#endif
    hp = gethostbyname(hoststr);
    if (!hp || !hp->h_addr_list)
#endif
    {
	if (inet_aton(hoststr,&tsa.sin_addr))
	{
	    *ipaddr = ntohl(tsa.sin_addr.s_addr);
	    return hoststr; /* We could call gethostbyaddr() on tsa to try and get the
			       official hostname but most systems would have already found
			       it when sending a dotted-quad to gethostbyname().  This is
			       good enough when that fails. */
	}
	eventlog(eventlog_level_error,"host_lookup","could not lookup host \"%s\"",hoststr);
	return NULL;
    }
    
#ifdef HAVE_GETHOSTBYNAME
    memcpy(&tsa.sin_addr,(void *)hp->h_addr_list[0],sizeof(struct in_addr)); /* avoid warning */
    *ipaddr = ntohl(tsa.sin_addr.s_addr);
    if (hp->h_name)
	return hp->h_name;
    return hoststr;
#endif
}


#ifdef USE_CHECK_ALLOC
extern t_addr * addr_create_num_real(unsigned int ipaddr, unsigned short port, char const * fn, unsigned int ln)
#else
extern t_addr * addr_create_num(unsigned int ipaddr, unsigned short port)
#endif
{
    t_addr * temp;
    
#ifdef USE_CHECK_ALLOC
    if (!(temp = check_malloc_real(sizeof(t_addr),fn,ln)))
#else
    if (!(temp = malloc(sizeof(t_addr))))
#endif
    {
	eventlog(eventlog_level_error,"addr_create_num","unable to allocate memory for addr");
	return NULL;
    }
    
    if (!(temp->str = strdup(addr_num_to_addr_str(ipaddr,port))))
    {
	eventlog(eventlog_level_error,"addr_create_num","could not allocate memory for str");
	free(temp);
	return NULL;
    }
    temp->str    = NULL;
    temp->ip     = ipaddr;
    temp->port   = port;
    temp->data.p = NULL;
    
    return temp;
}


#ifdef USE_CHECK_ALLOC
extern t_addr * addr_create_str_real(char const * str, unsigned int defipaddr, unsigned short defport, char const * fn, unsigned int ln)
#else
extern t_addr * addr_create_str(char const * str, unsigned int defipaddr, unsigned short defport)
#endif
{
    char *             tstr;
    t_addr *           temp;
    unsigned int       ipaddr;
    unsigned short     port;
    char const *       hoststr;
    char *             portstr;
    char const *       hostname;
    
    if (!str)
    {
	eventlog(eventlog_level_error,"addr_create_str","got NULL str");
	return NULL;
    }
    
    if (!(tstr = strdup(str)))
    {
	eventlog(eventlog_level_error,"addr_create_str","could not allocate memory for str");
	return NULL;
    }
    
    if ((portstr = strrchr(tstr,':')))
    {
	char * protstr;
	
	*portstr = '\0';
	portstr++;
	
	if ((protstr = strrchr(portstr,'/')))
	{
	    *protstr = '\0';
	    protstr++;
	}
	
	if (portstr[0]!='\0')
	{
	    if (str_to_ushort(portstr,&port)<0)
	    {
#ifdef HAVE_GETSERVBYNAME
		struct servent * sp;
		
		if (!(sp = getservbyname(portstr,protstr?protstr:"tcp")))
#endif
		{
		    eventlog(eventlog_level_error,"addr_create_str","could not convert \"%s\" to a port number",portstr);
		    free(tstr);
		    return NULL;
		}
#ifdef HAVE_GETSERVBYNAME
		port = ntohs(sp->s_port);
#endif
	    }
	}
	else
	    port = defport;
    }
    else
	port = defport;
    
    if (tstr[0]!='\0')
	hoststr = tstr;
    else
    {
	struct sockaddr_in tsa;
	
	tsa.sin_addr.s_addr = htonl(defipaddr);
	hoststr = inet_ntoa(tsa.sin_addr);
    }
    
    if (!(hostname = host_lookup(hoststr,&ipaddr)))
    {
	eventlog(eventlog_level_error,"addr_create_str","could not lookup host \"%s\"",hoststr);
	free(tstr);
	return NULL;
    }
    
#ifdef USE_CHECK_ALLOC
    if (!(temp = check_malloc_real(sizeof(t_addr),fn,ln)))
#else
    if (!(temp = malloc(sizeof(t_addr))))
#endif
    {
	eventlog(eventlog_level_error,"addr_create_str","unable to allocate memory for addr");
	free(tstr);
	return NULL;
    }
    
    if (!(temp->str = strdup(hostname)))
    {
	eventlog(eventlog_level_error,"addr_create_str","could not allocate memory for str");
	free(temp);
	free(tstr);
	return NULL;
    }
    free(tstr);
    
    temp->ip     = ipaddr;
    temp->port   = port;
    temp->data.p = NULL;
    
    return temp;
}


extern int addr_destroy(t_addr const * addr)
{
    if (!addr)
    {
	eventlog(eventlog_level_error,"addr_destroy","got NULL addr");
	return -1;
    }
    
    if (addr->str)
	free((void *)addr->str); /* avoid warning */
    free((void *)addr); /* avoid warning */
    
    return 0;
}


/* hostname or IP */
extern char * addr_get_host_str(t_addr const * addr, char * str, unsigned int len)
{
    if (!addr)
    {
	eventlog(eventlog_level_error,"addr_get_host_str","got NULL addr");
	return NULL;
    }
    if (!str)
    {
	eventlog(eventlog_level_error,"addr_get_host_str","got NULL str");
	return NULL;
    }
    if (len<2)
    {
	eventlog(eventlog_level_error,"addr_get_host_str","str too short");
	return NULL;
    }
    
    if (!addr->str)
    {
	eventlog(eventlog_level_error,"addr_get_host_str","addr has NULL str");
	return NULL;
    }
    
    strncpy(str,addr->str,len-1);
    str[len-1] = '\0';
    
    return str;
}


/* IP:port */
extern char * addr_get_addr_str(t_addr const * addr, char * str, unsigned int len)
{
    if (!addr)
    {
	eventlog(eventlog_level_error,"addr_get_addr_str","got NULL addr");
	return NULL;
    }
    if (!str)
    {
	eventlog(eventlog_level_error,"addr_get_addr_str","got NULL str");
	return NULL;
    }
    if (len<2)
    {
	eventlog(eventlog_level_error,"addr_get_addr_str","str too short");
	return NULL;
    }
    
    strncpy(str,addr_num_to_addr_str(addr->ip,addr->port),len-1);
    str[len-1] = '\0';
    
    return str;
}


extern unsigned int addr_get_ip(t_addr const * addr)
{
    if (!addr)
    {
	eventlog(eventlog_level_error,"addr_get_ip","got NULL addr");
	return 0;
    }
    
    return addr->ip;
}


extern unsigned short addr_get_port(t_addr const * addr)
{
    if (!addr)
    {
	eventlog(eventlog_level_error,"addr_get_port","got NULL addr");
	return 0;
    }
    
    return addr->port;
}


extern int addr_set_data(t_addr * addr, t_addr_data data)
{
    if (!addr)
    {
	eventlog(eventlog_level_error,"addr_set_data","got NULL addr");
	return -1;
    }
    
    addr->data = data;
    return 0;
}


extern t_addr_data addr_get_data(t_addr const * addr)
{
    t_addr_data tdata;
    
    if (!addr)
    {
	eventlog(eventlog_level_error,"addr_get_data","got NULL addr");
	tdata.p = NULL;
	return tdata;
    }
    
    return addr->data;
}


extern t_netaddr * netaddr_create_str(char const * netstr)
{
    t_netaddr *  netaddr;
    char *       temp;
    char const * netipstr;
    char const * netmaskstr;
    unsigned int netip;
    unsigned int netmask;
    
    if (!netstr)
    {
	eventlog(eventlog_level_error,"netaddr_create_str","unable to allocate memory for netaddr");
	return NULL;
    }
    
    if (!(temp = strdup(netstr)))
    {
	eventlog(eventlog_level_error,"netaddr_create_str","could not allocate memory for temp");
	return NULL;
    }
    if (!(netipstr = strtok(temp,"/")))
    {
	free(temp);
	return NULL;
    }
    if (!(netmaskstr = strtok(NULL,"/")))
    {
	free(temp);
	return NULL;
    }
    
    if (!(netaddr = malloc(sizeof(t_netaddr))))
    {
	eventlog(eventlog_level_error,"netaddr_create_str","could not allocate memory for netaddr");
	free(temp);
	return NULL;
    }
    
    /* FIXME: call getnetbyname() first, then host_lookup() */
    if (!host_lookup(netipstr,&netip))
    {
	eventlog(eventlog_level_error,"netaddr_create_str","could not lookup net");
	free(netaddr);
	free(temp);
	return NULL;
    }
    netaddr->ip = netip;
    
    if (str_to_uint(netmaskstr,&netmask)<0)
    {
	struct sockaddr_in tsa;
	
	if (inet_aton(netmaskstr,&tsa.sin_addr))
	    netmask = ntohl(tsa.sin_addr.s_addr);
	else
	{
	    eventlog(eventlog_level_error,"netaddr_create_str","could not convert mask");
	    free(netaddr);
	    free(temp);
	    return NULL;
	}
    }
    else
    {
	if (netmask>32)
	{
	    eventlog(eventlog_level_error,"netaddr_create_str","network bits must be less than or equal to 32 (%u)",netmask);
	    free(netaddr);
	    free(temp);
	    return NULL;
	}
	/* for example, 8 -> 11111111000000000000000000000000 */
	if (netmask!=0)
	    netmask = ~((1<<(32-netmask))-1);
    }
    netaddr->mask = netmask;
    
    free(temp);		// [zap-zero] 20020731 - (hopefully) fixed memory leak
    
    return netaddr;
}


extern int netaddr_destroy(t_netaddr const * netaddr)
{
    if (!netaddr)
    {
	eventlog(eventlog_level_error,"netaddr_destroy","got NULL netaddr");
	return -1;
    }
    
    free((void *)netaddr); /* avoid warning */
    
    return 0;
}


extern char * netaddr_get_addr_str(t_netaddr const * netaddr, char * str, unsigned int len)
{
    if (!netaddr)
    {
	eventlog(eventlog_level_error,"netaddr_get_addr_str","got NULL netaddr");
	return NULL;
    }
    if (!str)
    {
	eventlog(eventlog_level_error,"netaddr_get_addr_str","got NULL str");
	return NULL;
    }
    if (len<2)
    {
	eventlog(eventlog_level_error,"netaddr_get_addr_str","str too short");
	return NULL;
    }
    
    strncpy(str,netaddr_num_to_addr_str(netaddr->ip,netaddr->mask),len-1); /* FIXME: format nicely with x.x.x.x/bitcount */
    str[len-1] = '\0';
    
    return str;
}


extern int netaddr_contains_addr_num(t_netaddr const * netaddr, unsigned int ipaddr)
{
    if (!netaddr)
    {
	eventlog(eventlog_level_error,"netaddr_contains_addr_num","got NULL netaddr");
	return -1;
    }
    
    return (ipaddr&netaddr->mask)==netaddr->ip;
}


extern int addrlist_append(t_addrlist * addrlist, char const * str, unsigned int defipaddr, unsigned short defport)
{
    t_addr *     addr;
    char *       tstr;
    char *       tok;
    
    if (!str)
    {
	eventlog(eventlog_level_error,"addrlist_append","got NULL str");
	return -1;
    }

    if (!addrlist) {
	eventlog(eventlog_level_error,"addrlist_append","got NULL addrlist");
	return -1;
    }
        
    if (!(tstr = strdup(str)))
    {
	eventlog(eventlog_level_error,"addrlist_append","could not allocate memory for tstr");
	return -1;
    }
    
    for (tok=strtok(tstr,","); tok; tok=strtok(NULL,",")) /* strtok modifies the string it is passed */
    {
	if (!(addr = addr_create_str(tok,defipaddr,defport)))
	{
	    eventlog(eventlog_level_error,"addrlist_append","could not create addr");
	    free(tstr);
	    return -1;
	}
	if (list_append_data(addrlist,addr)<0)
	{
	    eventlog(eventlog_level_error,"addrlist_append","could not add item to list");
	    addr_destroy(addr);
	    free(tstr);
	    return -1;
	}
    }
    
    free(tstr);
    
    return 0;
}

extern t_addrlist * addrlist_create(char const * str, unsigned int defipaddr, unsigned short defport)
{
    t_addrlist * addrlist;

    if (!str)
    {
	eventlog(eventlog_level_error,"addrlist_create","got NULL str");
	return NULL;
    }
    
    if (!(addrlist = list_create()))
    {
	eventlog(eventlog_level_error,"addrlist_create","could not create list");
	return NULL;
    }

    if (addrlist_append(addrlist,str,defipaddr,defport)<0) {
	eventlog(eventlog_level_error,"addrlist_create","could not append to newly created addrlist");
	list_destroy(addrlist);
	return NULL;
    }

    return addrlist;
}

extern int addrlist_destroy(t_addrlist * addrlist)
{
    t_elem * curr;
    t_addr * addr;
    
    if (!addrlist)
    {
	eventlog(eventlog_level_error,"addrlist_destroy","got NULL addrlist");
	return -1;
    }
    
    LIST_TRAVERSE(addrlist,curr)
    {
        if (!(addr = elem_get_data(curr)))
            eventlog(eventlog_level_error,"addrlist_destroy","found NULL addr in list");
        else
            addr_destroy(addr);
        list_remove_elem(addrlist,curr);
    }
    
    return list_destroy(addrlist);
}


extern int addrlist_get_length(t_addrlist const * addrlist)
{
    return list_get_length(addrlist);
}
