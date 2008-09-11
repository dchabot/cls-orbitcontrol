#ifndef DAQSERVERUTILS_H_
#define DAQSERVERUTILS_H_

#include <stdint.h>
#include <sys/socket.h>

#define ifreq_size(pifreq) (pifreq->ifr_addr.sa_len + sizeof(pifreq->ifr_name))

char* NetAddrToString(unsigned long addr, char* buf);
struct ifreq* ifreqNext ( struct ifreq *pifreq );
unsigned int getNetwork (int s);
int ServerConnect(const char* aName, uint16_t aPort, const uint32_t aNetMask);

#endif /*DAQSERVERUTILS_H_*/
