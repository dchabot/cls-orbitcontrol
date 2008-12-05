# Templates/Makefile.leaf
# 	Template leaf node Makefile
#

PGMS=${ARCH}/orbitcontroller.obj
INCLUDE_DIR=../include
# C source names, if any, go here -- minus the .c
C_PIECES+=AdcReaderThread 
C_PIECES+=OrbitControllerUtils 
C_PIECES+=OrbitControllerIrq
C_PIECES+=DataHandler
C_PIECES+=PSController

C_FILES=$(C_PIECES:%=%.c)
C_O_FILES=$(C_PIECES:%=${ARCH}/%.o)

# C++ source names, if any, go here -- minus the .cc
CC_PIECES=VmeCrate VmeModule
CC_FILES=$(CC_PIECES:%=%.cc)
CC_O_FILES=$(CC_PIECES:%=${ARCH}/%.o)


H_PIECES=${INCLUDE_DIR}/bpmDefs
H_PIECES+=${INCLUDE_DIR}/AdcReaderThread ${INCLUDE_DIR}/OrbitController
H_PIECES+=${INCLUDE_DIR}/DataHandler ${INCLUDE_DIR}/PSController
H_PIECES+=${INCLUDE_DIR}/VmeCrate ${INCLUDE_DIR}/VmeModule
H_FILES=$(H_PIECES:%=%.h)

# Assembly source names, if any, go here -- minus the .S
S_PIECES=
S_FILES=$(S_PIECES:%=%.S)
S_O_FILES=$(S_FILES:%.S=${ARCH}/%.o)

SRCS=$(C_FILES) $(CC_FILES) $(H_FILES) $(S_FILES)
OBJS=$(C_O_FILES) $(CC_O_FILES) $(S_O_FILES)

# List of RTEMS managers to be included in the application goes here.
# Use:
#     MANAGERS=all
# to include all RTEMS managers in the application.
MANAGERS=all

include $(RTEMS_MAKEFILE_PATH)/Makefile.inc

include $(RTEMS_CUSTOM)
include $(RTEMS_ROOT)/make/leaf.cfg

#
# (OPTIONAL) Add local stuff here using +=
#
CFLAGS+= -I ../include -I $(RTEMS_MAKEFILE_PATH)/lib/include
CXXFLAGS += ${CFLAGS}


ifndef RTEMS_SITE_INSTALLDIR
	RTEMS_SITE_INSTALLDIR = /home/chabotd/ioc1009-109
endif

#
# Add your list of files to delete here.  The config files
#  already know how to delete some stuff, so you may want
#  to just run 'make clean' first to see what gets missed.
#  'make clobber' already includes 'make clean'
#

#CLEAN_ADDITIONS += xxx-your-debris-goes-here
CLOBBER_ADDITIONS +=

all:	${ARCH} $(SRCS) $(PGMS)

$(filter %.obj, $(PGMS)): ${OBJS}
	$(LD) -o $(PGMS) -r ${OBJS}

install:  all
	$(INSTALL_VARIANT) -m 644 $(PGMS) $(RTEMS_MAKEFILE_PATH)/lib
	$(INSTALL_VARIANT) -m 644 $(PGMS) $(RTEMS_SITE_INSTALLDIR)
# copy to tftp server bootfile location
#	scp $(PGMS) epics@srv-unix:$(RTEMS_SITE_INSTALLDIR)
	

