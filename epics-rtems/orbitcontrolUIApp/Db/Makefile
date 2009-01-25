TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#----------------------------------------------------
#  Optimization of db files using dbst (DEFAULT: NO)
#DB_OPT = YES

################################################################################
############## HEY, YOU!! THIS IS REALLY IMPORTANT, SO READ IT!! ###############
### This variable is used to switch between Experimental (testing on 10.51 vlan)
### and Production (10.50 vlan) record names. Be careful !!!
################################################################################
IS_EXPERIMENT_CONFIG=true

#----------------------------------------------------
# Create and install (or just install)
# databases, templates, substitutions like this
DB += Bpms.db
DB += BpmController.db
DB += OcmController.db
DB += OrbitController.db
# For control (setpoint) of individual OCM pwr supplies
DB += Ocm2408-01.db Ocm2404-05.db Ocm2406-01.db Ocm2406-03.db 
DB += OcmChicane2408-01.db

#DB += dbSubExample.db
#DB += user.substitutions
#DB += userHost.substitutions

#----------------------------------------------------
# If <anyname>.db template is not named <anyname>*.template add
# <anyname>_TEMPLATE = <templatename>

ifeq "${IS_EXPERIMENT_CONFIG}" "true"
	OCM_TEMPLATES += Ocm_exp.template
	BPM_TEMPLATES += Bpms_exp.template
else
	OCM_TEMPLATES += Ocm.template
	BPM_TEMPLATES += Bpms.template
endif


include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

vpath %.template

%.template: ../%.template
	cp $< $@
	chmod 664 $@

../O.Common/Ocm2408-01.db: ../Ocm2408-01.sub $(OCM_TEMPLATES)
	msi -S $< $(OCM_TEMPLATES) > $@

../O.Common/Ocm2404-05.db: ../Ocm2404-05.sub $(OCM_TEMPLATES)
	msi -S $< $(OCM_TEMPLATES) > $@

../O.Common/Ocm2406-01.db: ../Ocm2406-01.sub $(OCM_TEMPLATES)
	msi -S $< $(OCM_TEMPLATES) > $@

../O.Common/Ocm2406-03.db: ../Ocm2406-03.sub $(OCM_TEMPLATES)
	msi -S $< $(OCM_TEMPLATES) > $@

../O.Common/OcmChicane2408-01.db: ../OcmChicane2408-01.sub $(OCM_TEMPLATES)
	msi -S $< $(OCM_TEMPLATES) > $@
	
../O.Common/Bpms.db: ../Bpms.sub $(BPM_TEMPLATES)
	msi -S $< $(BPM_TEMPLATES) > $@
