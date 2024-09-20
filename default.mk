.SECONDARY: # prevent removal globally.
.SUFFIXES: # prevent built-in implicit rules being applied.
.DELETE_ON_ERROR: # delete the target file if the recipe fails \
			beginning to change the file.

WORK_HOME = /home/lup/rubbish/make_asist
BIN = $(WORK_HOME)/bin
INCLUDE = $(WORK_HOME)/include
SRC = $(WORK_HOME)/src
TEST = $(WORK_HOME)/test
DEPS = $(patsubst %.c, %.d, $(notdir $(wildcard $(SRC)/*.c $(TEST)/*.c))) $(patsubst %.cpp, %.d, $(notdir $(wildcard $(SRC)/*.cpp $(TEST)/*.cpp)))
CLEAN = $(BIN)

INCLUDE_MK = 
TEST_SCRIPT = 

# Define vpath for directory searching.
vpath %.h $(INCLUDE)
vpath %.c $(SRC):$(TEST)
vpath %.cpp $(SRC):$(TEST)
vpath %.o $(BIN)
vpath % $(BIN)
# End define vpath.

# Define flags.
CC = gcc
CFLAGS = -Wall -c -g -I$(INCLUDE) 
CXX = g++
PTHREADFLAG = -pthread
CXXFLAGS = -c -g -I$(INCLUDE) $(PTHREADFLAG)
CPPFLAGS = -I$(INCLUDE) -MM
LD = gcc
LDXX = g++
LFLAGS = $(PTHREADFLAG)
# End define flags.

# Define phony tagets.
.PHONY: all clean test
# End define phony targets.

# Define default target.
.DEFAULT_GOAL := all
OBJS =

# Include makefile specifiying `OBJS` and explicit rules.
-include $(INCLUDE_MK)
# Include dependency makefiles.
-include $(addprefix $(BIN)/,$(DEPS))

# Add $(OBJS) to prerequisites of all.
# Shouldn't modify `all` manually!
all : $(OBJS)
# End define default target.

# Define implicit rules.
# Makefile should't be remade automatically.
Makefile makefile : ;

# Generate .d files for .o files. Use '-MT' flag to specify the target
# (with absolute path, and .d files should also be added as target
# so that when head files change, .d files could be updated automatically)
$(BIN)/%.d : %.c;
	$(CC) $(CPPFLAGS) $(realpath $<) -MF $@ -MT '$(@:.d=.o) $@'

$(BIN)/%.d : %.cpp;
	$(CXX) $(CPPFLAGS) $(realpath $<) -MF $@ -MT '$(@:.d=.o) $@'

# Generate .o files from .c files.
$(BIN)/%.o : %.c
	$(CC) $(CFLAGS) $(sort $(realpath $(filter %.c, $^))) -o $@ 

# Generate .o files from .cpp files.
$(BIN)/%.o : %.cpp
	$(CXX) $(CXXFLAGS) $(sort $(realpath $(filter %.cpp, $^))) -o $@
# End define implicit rules.

# Define explicit rules.
# Would rather use static pattern rules than implicit rules to make `OBJS`.
$(OBJS) :
	$(LDXX) $(LFLAGS) $^ -o $@
# End define explicit rules.

clean :
	@if [ "$$(ls -A $(CLEAN))" ]; then \
		rm -rf $(addsuffix /*,$(CLEAN)); \
	fi

test :
	$(TEST_SCRIPT)

