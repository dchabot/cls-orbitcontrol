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
dbLoadRecords("db/Bpms.db","bpmChangeEvent=13")
dbLoadRecords("db/BpmController.db")

dbLoadRecords("db/Ocm2404-05.db","isInCorrection=1")
dbLoadRecords("db/Ocm2406-01.db","isInCorrection=1")
dbLoadRecords("db/Ocm2406-03.db","isInCorrection=1")
dbLoadRecords("db/Ocm2408-01.db","isInCorrection=1")
#dbLoadRecords("db/OcmChicane2408-01.db","isInCorrection=1")
### NOTE: responseX|YSize is numOcm*numBpm (presently,24*48)
dbLoadRecords("db/OcmController.db","responseXSize=1152,responseYSize=1152,numBpm=48")

iocInit()
