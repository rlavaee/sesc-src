#the following line has to be before the include
SIGNATURE=libpower
include	../Makefile.defs

DOTDEPENDEXTRA1=$$\(OBJ\)/libpower.a: $$\(OBJ\)/libcacti.a $$\(OBJ\)/libwattch.a $$\(OBJ\)/libpanalyzer.a $$\(OBJ\)/liborion.a
DOTDEPENDEXTRA2=-include $$\(OBJ\)/.depend.libcacti
DOTDEPENDEXTRA3=-include $$\(OBJ\)/.depend.libwattch
DOTDEPENDEXTRA4=-include $$\(OBJ\)/.depend.libpanalyzer
DOTDEPENDEXTRA5=-include $$\(OBJ\)/.depend.liborion

##############################################################################
#              Definitions
##############################################################################
WATTCHENV=$(OBJ)/libwattch.a
PANALENV=$(OBJ)/libpanalyzer.a
CACTIENV=$(OBJ)/libcacti.a
ORIONENV=$(OBJ)/liborion.a
SRCENV=$(OBJ)/libcore.a
SUCENV=$(OBJ)/libsuc.a
MINTENV=$(MINT_O)
ALLENVS = $(WATTCHENV) $(PANALENV) $(CACTIENV) $(ORIONENV) $(SRCENV) $(SUCENV) $(MINTENV)

POWER_SUBDIRS:= cacti wattch orion
ifdef SESC_THERM
POWER_SUBDIRS+=panalyzer
endif

##############################################################################
#                Objects
##############################################################################
OBJS = powermain.o cacmain.o

##############################################################################
#                             Change Rules                                   # 
##############################################################################
.PHONY: all clean $(POWER_SUBDIRS)

all	:$(POWER_SUBDIRS) $(patsubst %.o,$(OBJ)/%.o,$(OBJS))

depend:
	-@for a in $(POWER_SUBDIRS) ; 			\
	do						\
	$(MAKE) --no-print-directory -C $$a depend;	\
	done 

$(POWER_SUBDIRS): $(DOTDEPEND)
	@echo "Compiling " $@;			
	$(MAKE) --no-print-directory -C $@ all;	

wattch: cacti
panalyzer: cacti
orion: wattch

$(ALLENVS): $(POWER_SUBDIRS)

eclean:
	-@rm -f cactify wattchify panalyze
	-@for a in $(POWER_SUBDIRS) ; 			\
	do 						\
	echo "Cleaning " $$a ; 				\
	$(MAKE) --no-print-directory -C $$a clean ;	\
	done

# DO NOT DELETE
_MAKEFILE_COMMON_SUBSHELL=1
include $(SRC_DIR)/Makefile.common

ifneq ($(MAKECMDGOALS),distclean)
ifneq ($(MAKECMDGOALS),clean)
-include $(DOTDEPEND) 
endif
endif


