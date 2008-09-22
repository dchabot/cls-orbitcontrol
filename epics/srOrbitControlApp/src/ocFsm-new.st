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

int i;
%%unsigned short portNum=0;

%%char *hostName=0;
%%char *tmp=0;
int sockFD;
%%struct sockaddr_in server;
%%struct hostent *host;

double data[108];
assign data to "bpm:mux";

int n;
int isConnected;
int numBytesPerBpmArray;
int numChannels;

/* start */
ss PublishBpmValues {
	state init {
		when(){
			n=0;
			%%numChannels = sizeof(data)/sizeof(data[0]);
			%%numBytesPerBpmArray = sizeof(double)*numChannels;
			hostName = macValueGet("hostName");
			tmp = macValueGet("portNum");
			%%portNum = (unsigned short)strtoul(tmp,NULL,0);
			if(hostName==0 || portNum==0) {
				fprintf(stderr, "Error: hostName=%p, portNum=%#hx\n",hostName,portNum);
				exit(1);
			}
			printf("ocFsm: using hostName=%s, portNum=%hd\n",hostName,portNum);
		}state getSocket
	}

	state getSocket {
		entry {
			printf("ocFsm.st: state getSocket{}...\n");
		}
		when() {
			if ((sockFD = socket (AF_INET, SOCK_STREAM, 0)) < 0 ) {
				perror("socket() failed");
				exit(1);
			}
			/*hostName = "ioc2400-104";*/
			host = gethostbyname(hostName);
			if (host == NULL) {
				perror("gethostbyname()");
				exit(3);
			}

			%%server.sin_family = host->h_addrtype;
			%%memcpy (&server.sin_addr, host->h_addr_list[0], host->h_length);
			%%server.sin_port = htons((short)portNum);
		} state tcpConnect /* end when() */
	}

	state tcpConnect { /* FIXME loop here 'til socket is connected */
		entry {
			printf("In entry{}, state tcpConnect{}...\n");
		}
		when() {
			for(;;) {
				%%isConnected = connect(sockFD, (struct sockaddr *)&server, sizeof server);
				if (isConnected < 0 ) {
					printf("Waiting to connect...\n");
					sleep(1.0);
					continue;
				}
				else { /* yes! we're connected */
					printf("We're connected to %s...\n",inet_ntoa(server.sin_addr));
					break;
				}
			}
		} state readSocket /* end when() */
	}

	state readSocket {
		entry {
			%%memset(data,0,numBytesPerBpmArray);
			%%n = read(sockFD, data, numBytesPerBpmArray);
		}

		when(n == numBytesPerBpmArray) {  /*success*/
			printf("ocFsm.st : updating BPM demuxing record...");
		} state updateBpmPvs

		when(n < numBytesPerBpmArray) { /*failure*/
			printf("ocFsm.st : read returned %d,was expecting %d\n",n,numBytesPerBpmArray);
			printf("\t\tAttempting to reconnect...\n\n");
			%%close(sockFD);
		} state getSocket

	}

	state updateBpmPvs {
		when( ) {
			/*
			 * Just push data[] directly to bpm:mux PV:
			 *
			 * the values in data[] will be automagically demux'd
			 * to each BPM record via the waveform-->subArray relationship
			 */
			pvPut(data);
			printf("DONE\n");
		} state readSocket
	}
} /*end state-set PublishBPMValues */

