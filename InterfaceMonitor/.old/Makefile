include ./Platform/platform.mk
include /usr/local/etc/PcapPlusPlus.mk

# SOURCES := $(wildcard *.cpp)
SOURCES := $(wildcard main.cpp)
OBJS_FILENAMES := $(patsubst %.cpp,Obj/%.o,$(SOURCES))

CXX_FLAGS = -std=c++11

# Tady se asi musi pridat protobuf
Obj/%.o: %.cpp
	@echo Building file: $<
	@$(CXX) $(CXX_FLAGS) $(PCAPPP_BUILD_FLAGS) -c $(PCAPPP_INCLUDES) -fmessage-length=0 -MMD -MP -MF"$(@:Obj/%.o=Obj/%.d)" -MT"$(@:Obj/%.o=Obj/%.d)" -o "$@" "$<" -v

UNAME := $(shell uname)
CUR_TARGET := $(notdir $(shell pwd))

.SILENT:

all: InterfaceMonitor

start:
	@echo ==== Building target: $(CUR_TARGET) ====

create-directories:
	@$(MKDIR) -p Obj
	@$(MKDIR) -p Bin

PCAPPP_LIBS_DIR := -L/usr/local/lib

InterfaceMonitor: start create-directories $(OBJS_FILENAMES)
	@$(CXX) $(PCAPPP_BUILD_FLAGS) $(PCAPPP_LIBS_DIR)  -o "./Bin/InterfaceMonitor$(BIN_EXT)" $(OBJS_FILENAMES) $(PCAPPP_LIBS)
	@$(PCAPPP_POST_BUILD)
	@echo Finished successfully building: $(CUR_TARGET)
	@echo ' '

Protobuf:
	protoc -I=../ProtobufMessages/Src --cpp_out=../ProtobufMessages/Out/cpp ../ProtobufMessages/Src/HTTPMessage.proto

clean:
	@$(RM) -rf ./Obj/*
	@$(RM) -rf ./Bin/*
	@echo Clean finished: $(CUR_TARGET)

run:
	Bin/InterfaceMonitor$(BIN_EXT)

collect:
	Bin/InterfaceMonitor$(BIN_EXT) > data/interface.csv

# Only if we want to compile the libs as well #
dependents:
	@echo $(PCAPPLUSPLUS_HOME)
	@cd $(PCAPPLUSPLUS_HOME) && $(MAKE) libs
# ------------------------------------------- #