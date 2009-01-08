############## load our EPICS app module #########################
epicsAppHdl=ld("orbitcontrolUI.obj")

############## at least osdTimeRegister() is req'd in 3.14.10 ####
osdTimeRegister()

# read in envPaths file
< envPaths

dbLoadDatabase("dbd/orbitcontrolUI.dbd")

orbitcontrolUI_registerRecordDeviceDriver(pdbbase)

## Load record instances ####################################
dbLoadRecords("db/SRBpms.db","event=13,phase=1")
dbLoadRecords("db/SamplesPerAvg.db")

dbLoadRecords("db/SrOC2404-05.db")
dbLoadRecords("db/SrOC2406-01.db")
dbLoadRecords("db/SrOC2406-03.db")
dbLoadRecords("db/SrOC2408-01.db")
dbLoadRecords("db/SrChicane2408-01.db")

iocInit()
