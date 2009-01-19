############## load our EPICS app module #########################
epicsAppHdl=ld("orbitcontrolUI.obj")

############## at least osdTimeRegister() is req'd in 3.14.10 ####
osdTimeRegister()

# read in envPaths file
< envPaths

dbLoadDatabase("dbd/orbitcontrolUI.dbd")

orbitcontrolUI_registerRecordDeviceDriver(pdbbase)

## Load record instances ####################################
dbLoadRecords("db/OrbitController.db","modeChangeEvent=10")
#dbLoadRecords("db/Bpms.db","bpmChangeEvent=13")
#dbLoadRecords("db/BpmSamplesPerAvg.db")

#dbLoadRecords("db/SrOC2404-05.db","isInCorrection=1")
#dbLoadRecords("db/SrOC2406-01.db","isInCorrection=1")
#dbLoadRecords("db/SrOC2406-03.db","isInCorrection=1")
#dbLoadRecords("db/SrOC2408-01.db","isInCorrection=1")
#dbLoadRecords("db/SrChicane2408-01.db","isInCorrection=1")

iocInit()
