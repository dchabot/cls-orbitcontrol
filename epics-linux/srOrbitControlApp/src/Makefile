TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# build an ioc application

USR_CFLAGS += -g
USR_CXXFLAGS += -g
USR_LDFLAGS += -g

STATIC_BUILD=YES
SHARED_LIBRARIES=NO

PROD_IOC = srOrbitControl
# <name>.dbd will be created from <name>Include.dbd
DBD += srOrbitControl.dbd


# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
srOrbitControl_SRCS += srOrbitControl_registerRecordDeviceDriver.cpp
srOrbitControl_SRCS_DEFAULT += srOrbitControlMain.cpp
srOrbitControl_SRCS_vxWorks += -nil-

# custom device support:
srOrbitControl_SRCS += devSupBPMWaveform.c
srOrbitControl_SRCS += devSupOCMSetpoint.c
srOrbitControl_SRCS += devSupBPMSubArray.c
srOrbitControl_SRCS += devSupBPMSamplesSetpoint.c

#The following adds support from base/src/vxWorks
#srOrbitControl_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

#add a definition for each support application used by this application
#srOrbitControl_LIBS += xxx

#NOTES:
#      1)It is not possible to build sncExample both as a component of srOrbitControl
#        and standalone. You must choose only one.
#      2)To build sncExample SNCSEQ must be defined in <top>/configure/RELEASE

#The following builds sncExample as a component of srOrbitControl
#srOrbitControl_SRCS += ocFsm.st
#srOrbitControl_LIBS += seq pv

#srOrbitControl_LIBS += asyn
#asyn_DIR += $(EPICS_MODULES)/support/asyn/4-2-1/lib/$(T_A)
#srOrbitControl_LIBS += calc
#calc_DIR += $(EPICS_MODULES)/support/calc/2-5/lib/$(T_A)
#srOrbitControl_LIBS += sscan
#sscan_DIR += $(EPICS_MODULES)/support/sscan/2-5-1/lib/$(T_A)
#srOrbitControl_LIBS += genSub
#genSub_DIR += $(EPICS_MODULES)/support/genSub/1-6/lib/$(T_A)

srOrbitControl_LIBS += $(EPICS_BASE_IOC_LIBS)

#The following builds sncExample as a standalone application
#PROD_HOST += sncExample
#sncExample_SNCFLAGS += +m
#sncExample_SRCS += sncExample.stt
#sncExample_LIBS += seq pv
#sncExample_LIBS += $(EPICS_BASE_HOST_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

