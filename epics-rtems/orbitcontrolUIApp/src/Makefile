TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE


#=============================
# Build the IOC application

USR_CFLAGS_RTEMS += -g -I ../../../../rtems/include
USR_CXXFLAGS_RTEMS += ${USR_CFLAGS_RTEMS}
USR_LDFLAGS_RTEMS += -g -Wl,-Map,linkmap

PROD_IOC_RTEMS = orbitcontrolUI
# orbitcontrolUI.dbd will be created and installed
DBD += orbitcontrolUI.dbd

# orbitcontrolUI_registerRecordDeviceDriver.cpp derives from orbitcontrolUI.dbd
orbitcontrolUI_SRCS += orbitcontrolUI_registerRecordDeviceDriver.cpp

# custom device support:
orbitcontrolUI_SRCS += devSupOrbitControllerMbbio.cc
orbitcontrolUI_SRCS += devSupBPMBinaryOut.cc
orbitcontrolUI_SRCS += devSupBPMAnalogOut.cc
orbitcontrolUI_SRCS += devSupBPMAnalogIn.cc
orbitcontrolUI_SRCS += devSupBPMSamplesSetpoint.cc
orbitcontrolUI_SRCS += devSupOCMLongOut.cc
orbitcontrolUI_SRCS += devSupOCMWaveform.cc
orbitcontrolUI_SRCS += devSupOCMBinaryOut.cc
orbitcontrolUI_SRCS += devSupOCMAnalogOut.cc

# Build the main IOC entry point on workstation OSs.
orbitcontrolUI_SRCS_DEFAULT += orbitcontrolUIMain.cpp
orbitcontrolUI_SRCS_vxWorks += -nil-


# Finally link to the EPICS Base libraries
orbitcontrolUI_LIBS += $(EPICS_BASE_IOC_LIBS)

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

