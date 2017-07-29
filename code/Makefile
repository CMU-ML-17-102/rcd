.PHONY: all clean lib tests run_tests
.SECONDARY: %.cpp %.h %.cc

##################################################################
#Configurations
##################################################################
CONFIG ?= opt
USEMPI ?= 0
USEICE ?= 0
USEPBUF ?= 0

ifeq ($(CONFIG), dbg)
USEOPENMP = 0
DEBUG = 1
else ifeq ($(CONFIG), opt)
USEOPENMP = 1
DEBUG = 0
else 
$(error Invalid configuration)
endif

##################################################################
#Directories
##################################################################
SRCDIR = src
SRC = $(shell find $(SRCDIR)/ -type f -name '*.cpp') 
ICEOUTDIR = gen/ice
ICE = $(shell find $(SRCDIR)/ -type f -name '*.ice')
PBUFOUTDIR = gen/proto
PBUF = $(shell find $(SRCDIR)/ -type f -name '*.proto')

MAINSRCDIR = src_main
MAINSRC = $(wildcard $(MAINSRCDIR)/*.cpp)
TESTSRCDIR = src_test
TESTSRC = $(wildcard $(TESTSRCDIR)/*.cpp)

OBJDIR = obj/$(CONFIG)
OBJ = $(SRC:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
#ICE Files
ifeq ($(USEICE),1)
ICECPP = $(ICE:$(SRCDIR)/%.ice=$(ICEOUTDIR)/%.cpp)
ICEH = $(ICE:$(SRCDIR)/%.ice=$(ICEOUTDIR)/%.h)
ICEOBJ = $(ICE:$(SRCDIR)/%.ice=$(OBJDIR)/%.ico)
else
ICEOBJ = 
endif
#Proto Files
ifeq ($(USEPBUF),1)
PBUFCPP = $(PBUF:$(SRCDIR)/%.proto=$(PBUFOUTDIR)/%.pb.cc)
PBUFH = $(PBUF:$(SRCDIR)/%.proto=$(PBUFOUTDIR)/%.pb.h)
PBUFOBJ = $(PBUF:$(SRCDIR)/%.proto=$(OBJDIR)/%.pbo)
else
PBUFOBJ = 
endif

LIBDIR = lib/$(CONFIG)
LIBTARGET = $(LIBDIR)/librcd.a

BINDIR = bin/$(CONFIG)
BINTARGET = $(MAINSRC:$(MAINSRCDIR)/%.cpp=$(BINDIR)/%)

TESTTARGETDIR = $(BINDIR)
TESTTARGET = $(TESTSRC:$(TESTSRCDIR)/%.cpp=$(TESTTARGETDIR)/%)
TESTSUCCESSFLAG = $(TESTSRC:$(TESTSRCDIR)/%.cpp=$(TESTTARGETDIR)/%.sf)

##################################################################
#Flags
##################################################################
ifeq ($(USEOPENMP),1)
OMPFLAG = -fopenmp -DUSE_OPENMP
OMPLFLAG = -fopenmp
else
OMPFLAG = -Wno-unknown-pragmas
OMPLFLAG = 
endif

ifeq ($(USEICE),1)
ICEFLAG = -DUSE_ICE -I$(ICEOUTDIR)
ICELFLAG =  -lIce -lIceUtil -pthread 
else
ICEFLAG = 
ICELFLAG = 
endif

ifeq ($(USEPBUF),1)
PBUFFLAG = -DUSE_PBUF -I$(PBUFOUTDIR)
PBUFLFLAG = -lprotobuf
else
PBUFFLAG = 
PBUFLFLAG = 
endif

ifeq ($(DEBUG),1)
OFLAG = -g -DDEBUG -O0
else
OFLAG = -O3
endif

ifeq ($(USEMPI),1)
CPP = mpic++
else
CPP = g++
endif

CFLAG = -pg -rdynamic -Wall -Wno-reorder -I. -I$(SRCDIR) -I$(ICEOUTDIR) -I./ext/include -MMD -MP -std=c++0x -include common.h $(OFLAG) $(OMPFLAG) $(ICEFLAG) $(PBUFFLAG)
LFLAG = -lstdc++ $(OMPLFLAG) $(ICELFLAG) $(PBUFLFLAG)

all: $(BINTARGET)

tests: $(TESTTARGET)

run_tests: $(TESTSUCCESSFLAG)

$(OBJDIR):
	mkdir -p $(OBJDIR)
	
$(LIBDIR):
	mkdir -p $(LIBDIR)

$(BINDIR):
	mkdir -p $(BINDIR) 

$(ICEOUTDIR)/%.h $(ICEOUTDIR)/%.cpp : $(SRCDIR)/%.ice
	@mkdir -p "$(@D)"
	slice2cpp $< --output-dir "$(@D)"
	
$(PBUFOUTDIR)/%.pb.h : $(SRCDIR)/%.proto
	@mkdir -p "$(@D)" 
	protoc -I src $< --cpp_out=$(PBUFOUTDIR)		

$(PBUFOUTDIR)/%.pb.cc : $(PBUFOUTDIR)/%.pb.h 

$(OBJDIR)/%.ico : $(ICEOUTDIR)/%.cpp 	
	@mkdir -p "$(@D)"
	$(CPP) $(CFLAG) -I"$(<D)" -c $< -o $@

$(OBJDIR)/%.pbo : $(PBUFOUTDIR)/%.pb.cc			
	@mkdir -p "$(@D)"	
	$(CPP) $(CFLAG) -I"$(<D)" -c $< -o $@
			
$(OBJDIR)/%.o : $(SRCDIR)/%.cpp 	
	@mkdir -p "$(@D)"
	$(CPP) $(CFLAG) -c $< -o $@

-include $(OBJ:%.o=%.d)

$(OBJDIR)/environments/ParameterClientIceWrapper.o : $(ICEH)
ifeq ($(USEPBUF),1)
$(OBJDIR)/problems/SVMCodec.o : $(PBUFOUTDIR)/problems/SVMProtos.pb.h
endif

$(LIBTARGET): $(OBJ) $(ICEOBJ) $(PBUFOBJ) | $(LIBDIR)
	mkdir -p $(LIBDIR)
	ar crs $(LIBTARGET) $(OBJ) $(ICEOBJ) $(PBUFOBJ)

$(BINDIR)/%: $(MAINSRCDIR)/%.cpp $(LIBTARGET) | $(BINDIR)
	mkdir -p $(BINDIR)
	$(CPP) $(CFLAG) $< $(LIBTARGET) $(LFLAG) -o $@

$(TESTTARGETDIR)/%: $(TESTSRCDIR)/%.cpp $(LIBTARGET) | $(BINDIR)
	mkdir -p $(BINDIR)
	$(CPP) $(CFLAG) -include $(TESTSRCDIR)/common_test.h $< $(LIBTARGET) $(LFLAG) -o $@

$(TESTTARGETDIR)/%.sf: $(TESTTARGETDIR)/%
	$<
	touch $@

clean:	
	rm -rf $(BINDIR)
	rm -rf $(OBJDIR)
	rm -rf $(LIBDIR)
	rm -rf $(TESTTARGETDIR)

clean_all:
	rm -rf gen  
	rm -rf obj
	rm -rf bin
	rm -rf lib

rebuild: clean all

