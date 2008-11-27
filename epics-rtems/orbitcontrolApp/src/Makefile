TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE


#=============================
# Build the IOC application

USR_CFLAGS += -g
USR_CXXFLAGS += -g
USR_LDFLAGS += -g

PROD_IOC_RTEMS = orbitcontrol
# orbitcontrol.dbd will be created and installed
DBD += orbitcontrol.dbd

# orbitcontrol_registerRecordDeviceDriver.cpp derives from orbitcontrol.dbd
orbitcontrol_SRCS += orbitcontrol_registerRecordDeviceDriver.cpp

# custom device support:
orbitcontrol_SRCS += devSupBPMWaveform.c
orbitcontrol_SRCS += devSupOCMSetpoint.c
orbitcontrol_SRCS += devSupBPMSubArray.c
orbitcontrol_SRCS += devSupOCMWaveform.c

# Build the main IOC entry point on workstation OSs.
orbitcontrol_SRCS_DEFAULT += orbitcontrolMain.cpp
orbitcontrol_SRCS_vxWorks += -nil-


# Finally link to the EPICS Base libraries
orbitcontrol_LIBS += $(EPICS_BASE_IOC_LIBS)

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

