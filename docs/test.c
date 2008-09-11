#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>


typedef struct {
	int32_t 	 crateID;
	uint32_t vmeBaseAddr;
	uint32_t modOffset;
	uint32_t pwrSupChan;
	int32_t  pwrSupData;
} DioInfo;


int main(void) {
	char *cp = NULL;
	char input[]="2 0x00700000 8 12 -123";
	//int i;
	DioInfo dioInfo = {0};

	/* begin */
/*	for(cp=strtok(input," "),i=0; cp; cp=strtok(NULL, " "),i++) {
		printf("data[%d] = %s\n",i,cp);
	}
*/	
	dioInfo.crateID = strtol(cp=strtok(input," "),NULL,10);
	dioInfo.vmeBaseAddr = strtoul(cp=strtok(NULL," "),NULL,16);
	dioInfo.modOffset = strtoul(cp=strtok(NULL, " "),NULL,10);
	dioInfo.pwrSupChan = strtoul(cp=strtok(NULL," "),NULL,10);
	dioInfo.pwrSupData = strtol(cp=strtok(NULL," "),NULL,10);

	printf("dioInfo.crateID = %d\n",dioInfo.crateID);
	printf("dioInfo.vmeBaseAddr = %#x\n",dioInfo.vmeBaseAddr);
	printf("dioInfo.modOffset = %#x\n",dioInfo.modOffset);
	printf("dioInfo.pwrSupChan = %#x\n",dioInfo.pwrSupChan);
	printf("dioInfo.pwrSupData = %d\n",dioInfo.pwrSupData);
	
	return 0;
}
