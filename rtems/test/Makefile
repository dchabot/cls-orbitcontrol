# Templates/Makefile.leaf
# 	Template leaf node Makefile
#

PGMS=${ARCH}/tests.obj

# C source names, if any, go here -- minus the .c
C_PIECES=readoutTest irqTest mmapTest delayTest  adcAcquireTest SpawnTask adcFifoTest spinTest
C_FILES=$(C_PIECES:%=%.c)
C_O_FILES=$(C_PIECES:%=${ARCH}/%.o)

# C++ source names, if any, go here -- minus the .cc
CC_PIECES=
CC_FILES=$(CC_PIECES:%=%.cc)
CC_O_FILES=$(CC_PIECES:%=${ARCH}/%.o)

H_FILES= tests.h

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
CFLAGS+= -I ../../include 

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
#	$(AR) ruv o-optimize/libtest.a ${OBJS}
#	$(RANLIB) o-optimize/libtest.a

ifndef RTEMS_SITE_INSTALLDIR
RTEMS_SITE_INSTALLDIR = /export/tftpboot/epics/IOC2400-104
endif

install:  all
	    $(INSTALL_VARIANT) -m 644 $(PGMS) $(RTEMS_MAKEFILE_PATH)/lib

# copy to tftp server bootfile location
installremote: all install
	scp $(OBJNAME) epics@css1021-102:$(RTEMS_SITE_INSTALLDIR)


