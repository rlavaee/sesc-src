#the following line has to be before the include
SIGNATURE=libll
include	../Makefile.defs
##############################################################################
.PHONY: all clean

default: all

all: $(OBJ)/main.o $(OBJ)/libll.a 

##############################################################################
#                Objects
##############################################################################
OBJS	:=MIPSInstruction.o GFlow.o ExecutionFlow.o Events.o ThreadContext.o \
	HeapManager.o 

##############################################################################
#                             Change Rules                                   # 
##############################################################################
# eclean is called by clean
eclean:
	-@rm -f $(OBJ)/libll.a
	-@rm -f $(OBJ)/main.o

size:
	-@wc *.cpp *.h *.hpp

_MAKEFILE_COMMON_SUBSHELL=1

include $(SRC_DIR)/Makefile.common

ifneq ($(MAKECMDGOALS),distclean)
ifneq ($(MAKECMDGOALS),clean)
-include $(DOTDEPEND)
endif
endif
# DO NOT DELETE
