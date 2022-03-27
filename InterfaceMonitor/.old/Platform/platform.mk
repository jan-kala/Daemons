MAC_OS_X := 1

BIN_EXT :=

LIB_PREFIX := lib

LIB_EXT := .a

ifeq ($(origin CXX),default)
CXX := g++
endif

ifeq ($(origin CC),default)
CC := gcc
endif

AR := ar

RM := rm

CP := cp

MV := mv

TOUCH := touch

MKDIR := mkdir

INSTALL_SCRIPT := install.sh

UNINSTALL_SCRIPT := uninstall.sh
MACOS_SDK_HOME := /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk

# Apple Silicon (arm64) specific build flags
GLOBAL_FLAGS := -arch arm64 -target arm64-apple-macos11 -isysroot $(MACOS_SDK_HOME)


PCAPPLUSPLUS_HOME := /Users/jan.kala/Repos/school/Anotator/PcapPlusPlus/PcapPlusPlus
