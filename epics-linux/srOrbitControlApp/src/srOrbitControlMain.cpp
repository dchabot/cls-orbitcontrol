/* rtemsOrbCorMain.cpp */
/* Author:  Marty Kraimer Date:    17MAR2000 */
/*          R.Berg      Date:       25JUL2006   
                    added -d deamon code as default*/



#include <stddef.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
                                                                                        
#include "epicsThread.h"
#include "iocsh.h"

int main(int argc,char *argv[])
{
    int daemon = 0;
        if(argc>=2 && strcmp(argv[1],"-d") == 0) {
           argc--;
           argv++;
           daemon++;
        }
        if(argc>=2) {
           iocsh(argv[1]);
           epicsThreadSleep(.2);
        }
        if( daemon) {
           for(;;)
                epicsThreadSleep(60.0);
        }
        else
           iocsh(NULL);
        return(0);
}

