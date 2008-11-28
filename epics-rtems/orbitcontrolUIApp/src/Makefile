TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE


#=============================
# Build the IOC application

USR_CFLAGS_RTEMS += -g -I ../../../../rtems/include
USR_CXXFLAGS_RTEMS += -g -I ../../../../rtems/include
USR_LDFLAGS_RTEMS += -g

PROD_IOC_RTEMS = orbitcontrolUI
# orbitcontrolUI.dbd will be created and installed
DBD += orbitcontrolUI.dbd

# orbitcontrolUI_registerRecordDeviceDriver.cpp derives from orbitcontrolUI.dbd
orbitcontrolUI_SRCS += orbitcontrolUI_registerRecordDeviceDriver.cpp

# custom device support:
orbitcontrolUI_SRCS += devSupBPMWaveform.c
orbitcontrolUI_SRCS += devSupOCMSetpoint.c
orbitcontrolUI_SRCS += devSupBPMSubArray.c
orbitcontrolUI_SRCS += devSupOCMWaveform.c
orbitcontrolUI_SRCS += devSupBPMSamplesSetpoint.c

# Build the main IOC entry point on workstation OSs.
orbitcontrolUI_SRCS_DEFAULT += orbitcontrolUIMain.cpp
orbitcontrolUI_SRCS_vxWorks += -nil-


# Finally link to the EPICS Base libraries
orbitcontrolUI_LIBS += $(EPICS_BASE_IOC_LIBS)

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

