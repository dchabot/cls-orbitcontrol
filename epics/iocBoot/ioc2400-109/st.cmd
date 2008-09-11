#!../../bin/linux-x86/rtemsOrbCor -d

## You may have to change rtemsOrbCor to something else
## everywhere it appears in this file

< envPaths

cd ${TOP}

## Register all support components
dbLoadDatabase("dbd/rtemsOrbCor.dbd",0,0)
rtemsOrbCor_registerRecordDeviceDriver(pdbbase)

### connect to the RTEMS DioWriteServer... ##################
drvAsynIPPortConfigure("L1","ioc2400-104:24743 TCP")

## set "End Of String" characters here: #####################
asynOctetSetOutputEos("L1",0,"\n")

## Asyn debugging messages... ###############################
#asynSetTraceMask("L1",0,0x9)
#asynSetTraceIOMask("L1",0,0x2)

## Load record instances ####################################
dbLoadRecords("db/SrOC2404-05.db", "PORT=L1,ADDR=0")
dbLoadRecords("db/SrOC2406-01.db", "PORT=L1,ADDR=0")
dbLoadRecords("db/SrOC2406-03.db", "PORT=L1,ADDR=0")
dbLoadRecords("db/SrOC2408-01.db", "PORT=L1,ADDR=0")
dbLoadRecords("db/SrChicane2408-01.db", "PORT=L1,ADDR=0")

### this DB contains the bpm values #########################
dbLoadRecords("db/SrBpmOrbit.db")
### orbit RMS info ##########################################
#dbLoadRecords("db/SrOrbitRms.db", "clsName=SrBPMs")


cd ${TOP}/iocBoot/${IOC}
iocInit()

## Start any sequence programs
seq ocFsm "hostName=ioc2400-104,portNum=24742"

