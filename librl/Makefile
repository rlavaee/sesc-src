#the following line has to be before the include
SIGNATURE=librl
include	../Makefile.defs
##############################################################################
.PHONY: all clean

default : all

all: $(OBJ)/mtst1.o $(OBJ)/librl.a

##############################################################################
#                Objects
##############################################################################
OBJS	:= CMAC.o RL.o Tiles.o State.o Enum.o



##############################################################################
#                             Change Rules                                   # 
##############################################################################
# eclean is called by clean
eclean:
	-@rm -f $(OBJ)/librl.a
	-@rm -f $(OBJ)/mtst1.o

size:
	-@wc *.cpp *.h

_MAKEFILE_COMMON_SUBSHELL=1

include $(SRC_DIR)/Makefile.common

ifneq ($(MAKECMDGOALS),distclean)
ifneq ($(MAKECMDGOALS),clean)
-include $(DOTDEPEND)
endif
endif
# DO NOT DELETE
