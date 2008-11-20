#!../../bin/linux-x86/srOrbitControl

## You may have to change rtemsOrbCor to something else
## everywhere it appears in this file

< envPaths

cd ${TOP}

## Register all support components
dbLoadDatabase("dbd/srOrbitControl.dbd",0,0)
srOrbitControl_registerRecordDeviceDriver(pdbbase)

### connect to the RTEMS DioWriteServer... ##################
drvAsynIPPortConfigure("L1","ioc1009-109:24743 TCP")

## set "End Of String" characters here: #####################
asynOctetSetOutputEos("L1",0,"\n")

## Asyn debugging messages... ###############################
#asynSetTraceMask("L1",0,0x9)
#asynSetTraceIOMask("L1",0,0x2)

### connect to the RTEMS BPM samples/avg setpoint control... ##################
drvAsynIPPortConfigure("L2","ioc1009-109:24750 TCP")

## set "End Of String" characters here: #####################
asynOctetSetOutputEos("L2",0,"\n")

## Asyn debugging messages... ###############################
#asynSetTraceMask("L2",0,0x9)
#asynSetTraceIOMask("L2",0,0x2)

## Load record instances ####################################
dbLoadRecords("db/SrOC2404-05.db", "PORT=L1,ADDR=0")
dbLoadRecords("db/SrOC2406-01.db", "PORT=L1,ADDR=0")
dbLoadRecords("db/SrOC2406-03.db", "PORT=L1,ADDR=0")
dbLoadRecords("db/SrOC2408-01.db", "PORT=L1,ADDR=0")
dbLoadRecords("db/SrChicane2408-01.db", "PORT=L1,ADDR=0")

### contains the mux'd bpm fbk data ######################
dbLoadRecords("db/BpmArray.db")
dbLoadRecords("db/BpmArrayTest.db")
### contains the bpm fbk records #########################
dbLoadRecords("db/SRBpms.db")
### contains the mux'd OCM setpoint record (waveform) #### 
dbLoadRecords("db/OcmArray.db")
dbLoadRecords("db/OcmSetpoint.db")
### orbit RMS info ##########################################
#dbLoadRecords("db/SrOrbitRms.db", "clsName=SrBPMs")
dbLoadRecords("db/SamplesPerAvg.db","PORT=L2,ADDR=0")

cd ${TOP}/iocBoot/${IOC}
iocInit()

## Start any sequence programs
seq ocFsm "hostName=ioc1009-109,bpmPort=24742,ocmPort=24745"
