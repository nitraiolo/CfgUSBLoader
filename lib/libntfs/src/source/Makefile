#---------------------------------------------------------------------------------
# Clear the implicit built in rules
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=<path to>devkitPPC")
endif

ifeq ($(PLATFORM),wii)
include $(DEVKITPPC)/wii_rules
endif

ifeq ($(PLATFORM),cube)
include $(DEVKITPPC)/gamecube_rules
endif

#---------------------------------------------------------------------------------
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------
BUILD		?=	wii_release
SOURCES		:=	.
INCLUDES	:=	../include
LIBDIR		:=	../lib

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------

#CFLAGS		=	-ffast-math -O3 -pipe -mrvl -mcpu=750 -meabi -mhard-float -Wall $(MACHDEP) $(INCLUDE) -DGEKKO -DHAVE_CONFIG_H -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE
#CFLAGS		=	-ffast-math -Os -pipe -mrvl -mcpu=750 -meabi -mhard-float -Wall $(MACHDEP) $(INCLUDE) -DGEKKO -DHAVE_CONFIG_H -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE
CFLAGS		=	-Os -Wall -ffast-math -pipe $(MACHDEP) $(INCLUDE) -DHAVE_CONFIG_H
CXXFLAGS	=	$(CFLAGS)
ASFLAGS		:=	-g
export NTFSBIN	:= $(LIBDIR)/$(PLATFORM)/libntfs.a

ifeq ($(BUILD),cube_debug)
CFLAGS += -DDEBUG
CXXFLAGS += -DDEBUG
endif
ifeq ($(BUILD),wii_debug)
CFLAGS += -DDEBUG
CXXFLAGS += -DDEBUG
endif

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------
LIBS		:=

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS		:=

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))


export OFILES	:=	$(addsuffix .o,$(BINFILES)) \
			$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD) \
			-I$(LIBOGC_INC)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib) \
			-L$(LIBOGC_LIB)

.PHONY: $(BUILD) clean

#---------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr wii_debug wii_release cube_debug cube_release $(LIBDIR)

all: $(NTFSBIN)

install_ogc:
	cp ../include/ntfs.h $(DEVKITPRO)/libogc/include
	cp ../lib/wii/libntfs.a $(DEVKITPRO)/libogc/lib/wii
	cp ../lib/cube/libntfs.a $(DEVKITPRO)/libogc/lib/cube

wii-install:
	cp ../include/ntfs.h $(DEVKITPRO)/libogc/include
	cp ../lib/wii/libntfs.a $(DEVKITPRO)/libogc/lib/wii

DKV			:= $(shell $(DEVKITPPC)/bin/$(CC) --version | sed 's/^.*(devkitPPC release \([0-9]*\)).*$$/\1/;q')
DEST_INC	:=	../../include
DEST_LIB	:=	../../lib$(DKV)

install:
	mkdir -p $(DEST_INC) $(DEST_LIB)
	cp ../include/ntfs.h $(DEST_INC)
	cp ../include/ntfs_frag.h $(DEST_INC)
	cp ../lib/wii/libntfs.a $(DEST_LIB)

#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(NTFSBIN): $(OFILES) $(LIBDIR)/$(PLATFORM)
	@rm -f "../$(NTFSBIN)"
	@$(AR) rcs "../$(NTFSBIN)" $(OFILES)
	@echo built ... $(notdir $@)

$(LIBDIR)/$(PLATFORM):
	mkdir -p ../$(LIBDIR)/$(PLATFORM)

-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------

