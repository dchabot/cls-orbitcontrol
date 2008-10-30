program ocFsm

#include "bpmPvs_exp.h"

%%#include <string.h>
%%#include <math.h>

%%#include <stdio.h>
%%#include <string.h>
%%#include <stdlib.h>
%%#include <unistd.h>
%%#include <sys/types.h>
%%#include <sys/stat.h>
%%#include <fcntl.h>
%%#include <sys/socket.h>
%%#include <netinet/in.h>
%%#include <arpa/inet.h>
%%#include <netdb.h>

char *hostName;
char *tmp;

/* Begin BPM related variables */
int i;

%%unsigned short bpmPort=0;
int bpmSockFd;
%%struct sockaddr_in bpmServer;
%%struct hostent *bpmHost;

double bpmdata[108];
assign bpmdata to "bpm:mux";

int n;
int isBpmConnected;
int numBytesPerBpmArray;
int numBpmChannels;
/* End BPM related variables */

/* Begin OCM-related variables */
int ocmdata[48];
assign ocmdata to "ocm:array";
monitor ocmdata;
evflag ocmflag;
sync ocmdata ocmflag;

int j;
%%unsigned short ocmPort=0;
int ocmSockFd;
%%struct sockaddr_in ocmServer;
%%struct hostent *ocmHost;

int m;
int isOcmConnected;
int numBytesPerOcmArray;
int numOcmChannels;
/* End OCM-related variables */


/* start */

ss PublishBpmValues {
	state init {
		when(delay(1.0)){
			n=0;
			%%numBpmChannels = sizeof(bpmdata)/sizeof(bpmdata[0]);
			%%numBytesPerBpmArray = sizeof(double)*numBpmChannels;
			printf("numBpmChannels=%d\tnumBytesPerBpmArray=%d\n",numBpmChannels,numBytesPerBpmArray);
			tmp = macValueGet("bpmPort");
			%%bpmPort = (unsigned short)strtoul(tmp,NULL,0);
			if(bpmPort==0) {
				fprintf(stderr, "Error: bpmPort is NULL\n");
				exit(1);
			}
			printf("ocFsm: using bpmPort=%hd\n",bpmPort);
		} state getBpmSocket
	}

	state getBpmSocket {
		entry {
			printf("ocFsm.st: state getBpmSocket{}...\n");
		}
		when() {
			if ((bpmSockFd = socket (AF_INET, SOCK_STREAM, 0)) < 0 ) {
				perror("socket() failed");
				exit(1);
			}
			hostName = macValueGet("hostName");
			bpmHost = gethostbyname(hostName);
			if (bpmHost == NULL) {
				perror("gethostbyname()");
				exit(3);
			}

			%%bpmServer.sin_family = bpmHost->h_addrtype;
			%%memcpy (&bpmServer.sin_addr, bpmHost->h_addr_list[0], bpmHost->h_length);
			%%bpmServer.sin_port = htons((short)bpmPort);
		} state tcpConnect /* end when()*/
	}

	state tcpConnect {  /*FIXME loop here 'til socket is connected*/
		entry {
			printf("In entry{}, state tcpConnect{}...\n");
		}
		when() {
			for(;;) {
				%%isBpmConnected = connect(bpmSockFd, (struct sockaddr *)&bpmServer, sizeof bpmServer);
				if (isBpmConnected < 0 ) {
					printf("BPM: Waiting to connect...\n");
					sleep(1.0);
					continue;
				}
				else {  /*yes! we're connected
*/					printf("BPM: We're connected to %s...\n",inet_ntoa(bpmServer.sin_addr));
					break;
				}
			}
		} state readSocket  /*end when()*/
	}

	state readSocket {
		entry {
			%%memset(bpmdata,0,numBytesPerBpmArray);
			/* block-waiting here for updates */
			%%n = read(bpmSockFd, bpmdata, numBytesPerBpmArray);
		}

		when(n == numBytesPerBpmArray) {  /*success*/
			/*printf("ocFsm.st: updating BPM demuxing record...");*/
		} state updateBpmPvs

		when(n < numBytesPerBpmArray) { /*failure*/
			printf("ocFsm.st: read returned %d,was expecting %d\n",n,numBytesPerBpmArray);
			printf("\t\tAttempting to reconnect...\n\n");
			%%close(bpmSockFd);
		} state getBpmSocket

	}

	state updateBpmPvs {
		when( ) {

			 /* Just push bpmdata[] directly to bpm:mux PV:
			  *
			  * the values in bpmdata[] will be automagically demux'd
			  * to each BPM record via the waveform-->subArray relationship
			  */
			pvPut(bpmdata);
			//printf("DONE\n");
		} state readSocket
	}
} /*end state-set PublishBPMValues */


ss PublishOCMValues {
	state init {
		when(){
			n=0;
			%%numOcmChannels = sizeof(ocmdata)/sizeof(ocmdata[0]);
			%%numBytesPerOcmArray = sizeof(int)*numOcmChannels;
			printf("numOcmChannels=%d\tnumBytesPerOcmArray=%d\n",numOcmChannels,numBytesPerOcmArray);
			tmp = macValueGet("ocmPort");
			%%ocmPort = (unsigned short)strtoul(tmp,NULL,0);
			if(ocmPort==0) {
				fprintf(stderr, "Error: ocmPort is NULL!!\n");
				exit(1);
			}
			printf("ocFsm: using ocmPort=%hd\n",ocmPort);
		}state getOcmSocket
	}

	state getOcmSocket {
		entry {
			printf("ocFsm.st: state getOcmSocket{}...\n");
		}
		when() {
			if ((ocmSockFd = socket (AF_INET, SOCK_STREAM, 0)) < 0 ) {
				perror("socket() failed");
				exit(1);
			}
			hostName = macValueGet("hostName");
			ocmHost = gethostbyname(hostName);
			if (ocmHost == NULL) {
				perror("gethostbyname()");
				exit(3);
			}

			%%ocmServer.sin_family = ocmHost->h_addrtype;
			%%memcpy (&ocmServer.sin_addr, ocmHost->h_addr_list[0], ocmHost->h_length);
			%%ocmServer.sin_port = htons((short)ocmPort);
		} state tcpConnect /* end when() */
	}

	state tcpConnect { /* FIXME loop here 'til socket is connected */
		entry { printf("OCM: In entry{}, state tcpConnect{}...\n"); }

		when() {
			for(;;) {
				%%isOcmConnected = connect(ocmSockFd, (struct sockaddr *)&ocmServer, sizeof ocmServer);
				if (isOcmConnected < 0 ) {
					printf("OCM: Waiting to connect...\n");
					sleep(1.0);
					continue;
				}
				else { /* yes! we're connected */
					printf("OCM: We're connected to %s...\n",inet_ntoa(ocmServer.sin_addr));
					break;
				}
			}
		} state waitForOcmUpdate /* end when() */

		exit { efTestAndClear(ocmflag); }
	}

	state waitForOcmUpdate {
		when(efTestAndClear(ocmflag)) {
			printf("ocFsm.st: OCM setpoint array was updated\n");
		} state writeSocket
	}

	state writeSocket {
		entry {
			%%m = write(ocmSockFd, ocmdata, numBytesPerOcmArray);
		}

		when(m == numBytesPerOcmArray) {  /*success*/
			printf("ocFsm.st : wrote %d bytes to OCM setpoint Server...\n",m);
		} state waitForOcmUpdate

		when(n < numBytesPerOcmArray) { /*failure*/
			printf("ocFsm.st : OCM write returned %d,was expecting %d\n",m,numBytesPerOcmArray);
			printf("\t\tAttempting to reconnect...\n\n");
			%%close(ocmSockFd);
		} state getOcmSocket

	}

}


