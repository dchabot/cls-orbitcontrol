## Example RTEMS startup script

## You may have to change 1009-109 to something else
## everywhere it appears in this file

#< envPaths

## Register all support components
dbLoadDatabase "dbd/1009-109.dbd"
1009_109_registerRecordDeviceDriver pdbbase

## Load record instances
dbLoadTemplate "db/user.substitutions"
dbLoadRecords "db/dbSubExample.db", "user=chabotd"

## Set this to see messages from mySub
#var mySubDebug 1

## Run this to trace the stages of iocInit
#traceIocInit

iocInit

## Start any sequence programs
#seq sncExample, "user=chabotd"
