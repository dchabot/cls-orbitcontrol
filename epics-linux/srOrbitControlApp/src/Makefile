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

#STATIC_BUILD=YES
SHARED_LIBRARIES=YES

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

