#
#  $Id: $
#
# Templates/Makefile.lib
#       Template library Makefile
#

LIBDIR= $(RTEMS_MAKEFILE_PATH)/lib
# xxx- your library names goes here
LIBNAME= libutils
OBJNAME= $(LIBNAME).obj
LIB=${ARCH}/${LIBNAME}.a

# C and C++ source names, if any, go here -- minus the .c or .cc
C_PIECES= utils spinDelay findTscFreq

C_FILES=$(C_PIECES:%=%.c)
C_O_FILES=$(C_PIECES:%=${ARCH}/%.o)

CC_PIECES= 
CC_FILES=$(CC_PIECES:%=%.cc)
CC_O_FILES=$(CC_PIECES:%=${ARCH}/%.o)

H_FILES=

# Assembly source names, if any, go here -- minus the .S
S_PIECES=
S_FILES=$(S_PIECES:%=%.S)
S_O_FILES=$(S_FILES:%.S=${ARCH}/%.o)

SRCS=$(C_FILES) $(CC_FILES) $(H_FILES) $(S_FILES)
OBJS=$(C_O_FILES) $(CC_O_FILES) $(S_O_FILES)


include $(RTEMS_MAKEFILE_PATH)/Makefile.inc
include $(RTEMS_CUSTOM)
include $(RTEMS_ROOT)/make/lib.cfg

#
# Add local stuff here using +=
#
INCLUDE_DIR=${LIBDIR}/include
#CFLAGS += -I ${INCLUDE_DIR}

#
# Add your list of files to delete here.  The config files
#  already know how to delete some stuff, so you may want
#  to just run 'make clean' first to see what gets missed.
#  'make clobber' already includes 'make clean'
#

CLEAN_ADDITIONS += $(OBJNAME)
CLOBBER_ADDITIONS += Depends-o-optimize

ifndef RTEMS_SITE_INSTALLDIR
RTEMS_SITE_INSTALLDIR = /export/tftpboot/epics/IOC0000-032
endif

all:	${ARCH} $(SRCS) $(LIB)

$(LIB): ${OBJS}
	$(make-library)
	$(LD) -o $(OBJNAME) -r --whole-archive $(LIB)
# Install the library, appending _g or _p as appropriate.
# for include files, just use $(INSTALL_CHANGE)
install:  all
	test -d "${INCLUDE_DIR}" || mkdir ${INCLUDE_DIR}
#	$(CP) ${H_FILES} ${INCLUDE_DIR}
#	$(CP) $(LIB) $(OBJNAME) ../../lib
	$(INSTALL_VARIANT) -m 644 $(OBJNAME) $(LIBDIR)
	$(INSTALL_VARIANT) -m 644 $(LIB) $(LIBDIR)
# copy to tftp server bootfile location
installremote: all install
	    scp $(OBJNAME) epics@srv-unix:$(RTEMS_SITE_INSTALLDIR)


