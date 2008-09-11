#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <string.h>
#include <errno.h>

#include "DaqServerUtils.h"


char* NetAddrToString(unsigned long addr, char* buf) {
	sprintf(buf, "%d.%d.%d.%d",	(int)(addr >> 24) & 0xff,
            					(int)(addr >> 16) & 0xff,
            					(int)(addr >>  8) & 0xff,
            					(int)addr & 0xff);
	return buf;
}

/*
 * Move to the next ifreq structure
 * Made difficult by the fact that addresses larger than the structure
 * size may be returned from the kernel.
 */
struct ifreq* ifreqNext ( struct ifreq *pifreq ) {
    size_t size = ifreq_size ( pifreq );
    
    if ( size < sizeof ( *pifreq ) ) {
    	size = sizeof ( *pifreq );
    }
    return ( struct ifreq * )( size + ( char * ) pifreq );
}


unsigned int getNetwork (int s) {
    char ibuf[200];
    struct ifconf ifc;
    struct ifreq *ifr;
    unsigned int ipaddr, net;

    ifc.ifc_len = sizeof ibuf;
    ifc.ifc_buf = ibuf;
    if (ioctl (s, SIOCGIFCONF, (char *)&ifc) < 0) {
    	syslog(LOG_INFO,"SIOCGIFCONF ioctl failed !!!");
    	return(0);
    }

    for (ifr = ifc.ifc_req;
    	((char *)ifr < ((char *)ifc.ifc_req + ifc.ifc_len));
    	ifr = ifreqNext(ifr) ) {
     
    	if ( ifr->ifr_addr.sa_family != AF_INET ) { continue; }

        struct sockaddr_in *sp = (struct sockaddr_in *)&ifr->ifr_addr;
        ipaddr = ntohl (sp->sin_addr.s_addr);

        if (ipaddr != ((127 << 24) | 1 )) {
        	net = ipaddr & 0xFFFF0000;
            return net;
        }
    }
    syslog(LOG_INFO,"NO NETWORK !!!");
    return 0;
}

/* Sets up a basic TCP/IP server and waits for an "acceptable" client (same subnet as server).
 * Returns the connected socket-descriptor
 */
int ServerConnect(const char* aName, uint16_t aPort, const uint32_t aNetMask) {
	int fd = -1;
	int clientFD = -1;
	struct sockaddr_in sockaddr = {0};
	unsigned long addr;
	socklen_t len = sizeof(sockaddr);
	extern int errno;
	unsigned int myNetwork=0;
	int i = 1;
	
	for(;;) {
		/*
		 * Create socket
		 * Don't bother looking up the protocol (getprotoent)
		 */
		if ((fd = socket (PF_INET, SOCK_STREAM, 0)) < 0) {
			syslog(LOG_INFO,"%s: Couldn't create socket: %s\n", aName,strerror(errno));
			return -1;
		}
	
		/* SO_REUSEADDR: rules applied in a bind(2) should allow for reuse of local addrs */
		if(setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&i,sizeof(i)) < 0) {
			syslog(LOG_INFO, "%s: couldn't apply SO_REUSEADDR: %s\n",aName,strerror(errno));
		}
		
		/*
		 * Bind name to socket
		 * Don't bother looking up the port number (getservbyname)
		 */
		sockaddr.sin_family = AF_INET;
		sockaddr.sin_addr.s_addr = htonl (INADDR_ANY);
		sockaddr.sin_port = htons (aPort);
		
		memset (sockaddr.sin_zero, 0, sizeof sockaddr.sin_zero);
		
		if (bind (fd, (struct sockaddr *)&sockaddr, sizeof (sockaddr)) < 0) {
			syslog(LOG_INFO,"%s: Can't bind name to socket: %s\n",aName, strerror(errno));
			return -1;
		}
	
		/* get net-info (allowing for minimal security...) */
		myNetwork = getNetwork(fd);
		
		if (listen (fd, 1) != 0) {
			syslog(LOG_INFO,"%s: Can't listen on socket: %s\n",aName, strerror(errno));
			return -1;
		}


		/* Wait for a client to connect */
		if ( (clientFD = accept (fd, (struct sockaddr *)&sockaddr, &len)) < 0 ) {
			syslog(LOG_INFO,"%s: Can't accept connection on socket: %s\n",aName,strerror(errno));
			return -1;
		}
		/* Make sure the connection is from an acceptable client */
		addr = ntohl(sockaddr.sin_addr.s_addr);
		if(/*(addr & aNetMask) == myNetwork*/1) {
			char buf[32] = {0};
			
			syslog(LOG_INFO, "%s: accepted connection from %s\n",aName,NetAddrToString(addr,buf));
			/* Close the original socket */
			close(fd);
			return clientFD;
		}
		else {
			char buf[32] = {0};
			
			syslog(LOG_INFO, "%s: won't accept connection from %s\n",aName,NetAddrToString(addr, buf));
			/* Close the original socket */
			close(fd);
			fd = -1;
			close(clientFD);
			clientFD = -1;
			/* just to be explicit... */
			continue;
		}
	} /* end for(;;) */
}
