#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPro")
endif

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

ifeq ($(strip $(FEOSSDK)),)
$(error "Please set FEOSSDK in your environment. export FEOSSDK=<path to>FeOS/sdk")
endif

FEOSMK = $(FEOSSDK)/mk

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
#---------------------------------------------------------------------------------
TARGET        := FSS7
BUILD         := build
SOURCES       := source ../common_src
DATA          := data
INCLUDES      := include ../common_inc

CONF_DEFINES =
CONF_ARM7 = 1

include $(FEOSMK)/app.mk

install: all
	@mkdir -p $(FEOSDEST)/data/FeOS/arm7
	@cp $(TARGET).fx2 $(FEOSDEST)/data/FeOS/arm7/$(TARGET).fx2
