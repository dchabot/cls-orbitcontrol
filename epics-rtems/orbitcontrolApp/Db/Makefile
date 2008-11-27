TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#----------------------------------------------------
#  Optimization of db files using dbst (DEFAULT: NO)
#DB_OPT = YES

#----------------------------------------------------
# Create and install (or just install)
# databases, templates, substitutions like this
# for bulk distribution of BPM avgs (automatic via subArray record-type) 
DB += BpmArray.db
DB += SRBpms.db
# for bulk distribution of OCM setpoints 
DB += OcmArray.db
# control of BPM mux/demux update rate
DB += SamplesPerAvg.db
# For control (setpoint) of individual OCM pwr supplies
DB += SrOC2408-01.db SrOC2404-05.db SrOC2406-01.db SrOC2406-03.db 
DB += SrChicane2408-01.db

#DB += dbSubExample.db
#DB += user.substitutions
#DB += userHost.substitutions

#----------------------------------------------------
# If <anyname>.db template is not named <anyname>*.template add
# <anyname>_TEMPLATE = <templatename>
BPM_TEMPLATES += SRBpms.template
SrOC_TEMPLATES += SrOC.template

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

vpath %.template

%.template: ../%.template
	cp $< $@
	chmod 664 $@

../O.Common/SrOC2408-01.db: ../SrOC2408-01_exp.sub $(SrOC_TEMPLATES)
	msi -S ../SrOC2408-01_exp.sub > ../O.Common/SrOC2408-01.db

../O.Common/SrOC2404-05.db: ../SrOC2404-05_exp.sub $(SrOC_TEMPLATES)
	msi -S ../SrOC2404-05_exp.sub > ../O.Common/SrOC2404-05.db

../O.Common/SrOC2406-01.db: ../SrOC2406-01_exp.sub $(SrOC_TEMPLATES)
	msi -S ../SrOC2406-01_exp.sub > ../O.Common/SrOC2406-01.db

../O.Common/SrOC2406-03.db: ../SrOC2406-03_exp.sub $(SrOC_TEMPLATES)
	msi -S ../SrOC2406-03_exp.sub > ../O.Common/SrOC2406-03.db

../O.Common/SrChicane2408-01.db: ../SrChicane2408-01_exp.sub $(SrOC_TEMPLATES)
	msi -S ../SrChicane2408-01_exp.sub > ../O.Common/SrChicane2408-01.db

../O.Common/SRBpms.db: ../SRBpms_exp.sub $(BPM_TEMPLATES)
	msi -S ../SRBpms_exp.sub > $@	 
	
