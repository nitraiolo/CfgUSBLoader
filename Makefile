#---------------------------------------------------------------------------------
# Clear the implicit built in rules
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=<path to>devkitPPC")
endif

include $(DEVKITPPC)/wii_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------
VERSION		:=	70r78
RELEASE		:=	release
# to override RELEASE use: make announce RELEASE=beta
ifeq ($(findstring compat,$(VERSION)),compat)
	RELEASE := release
else ifeq ($(findstring debug,$(VERSION)),debug)
	RELEASE := debug
else ifeq ($(findstring c,$(VERSION)),c)
	RELEASE := bugfix
else ifeq ($(findstring b,$(VERSION)),b)
	RELEASE := beta
else ifeq ($(findstring a,$(VERSION)),a)
	RELEASE := alpha
else ifeq ($(findstring t,$(VERSION)),t)
	RELEASE := test
else ifeq ($(findstring x,$(VERSION)),x)
	RELEASE := experimental
endif

BINBASE		:=	cfg$(VERSION)
TARGET		:=	$(BINBASE)
BUILD		:=	build
SOURCES		:=	source source/pngu source/libwbfs source/unzip
DATA		:=	data
INCLUDES	:=	
GETTEXT		:=	tools/gettext
UPLOAD		:=  tools/googlecode_upload.py
GAUTH		?=  -u username -w passwd
INSTDIR		:=  ../Cfg_USB_Loader_$(VERSION)

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------

DKV			:= $(shell $(DEVKITPPC)/bin/$(CC) --version | sed 's/^.*(devkitPPC release \([0-9]*\)).*$$/\1/;q')
PORTLIBS	:= $(DEVKITPRO)/portlibs/ppc

BUILD_222	:=	0
ifeq ($(BUILD_222),1)
	BUILD	:=	build_222
	TARGET	:=	$(BINBASE)-222
	BUILD_222_FLAG := -DBUILD_222=$(BUILD_222)
endif

BUILD_DEBUG	:=	0
ifeq ($(BUILD_DEBUG),1)
	BUILD	:=	build_dbg
	TARGET	:=	$(BINBASE)-dbg
	BUILD_DBG_FLAG := -DBUILD_DBG=3
endif
#"-g" tells the compiler to include support for the debugger
#"-Wall" tells it to warn us about all suspicious-looking code
DEBUG_OPT	= -Os
#DEBUG_OPT	= -g
BUILD_FLAGS = -DVERSION=$(VERSION) -DDEVKITPPCVER=$(DKV) -DCCOPT=\"$(DEBUG_OPT)\" $(BUILD_222_FLAG) $(BUILD_DBG_FLAG)
CFLAGS	= $(DEBUG_OPT) -Wall $(MACHDEP) $(INCLUDE) $(BUILD_FLAGS) $(DEFINES)
CXXFLAGS	=	$(CFLAGS)

# start address:
# Original:   0x80a00100
# NeoGammaR4: 0x80dfff00
# cfg 33-36:  0x80c00000
# cfg 37-49:  0x80b00000
# cfg 50-..:  0x80a80000
LDFLAGS	=	$(MACHDEP) -Wl,-Map,$(notdir $@).map,--section-start,.init=0x80a80000

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------
#LIBS	:=	-lmetaphrasis -lfreetype -lasnd -lpng -lz -lfat -lwiiuse -lbte -lmad -lmodplay -logc -lm -ltremor
LIBAESND=$(wildcard $(LIBOGC_LIB)/libaesnd.a)
ifneq ($(strip $(LIBAESND)),)
LAESND  := -laesnd
endif

LIBS	:=	-lgrrlib -lfat -lntfs -lext2fs -lpng -ljpeg -lwiiuse -lbte -lmad -lmodplay -lasnd -logc -lm -lz


#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
#LIBDIRS	:=	$(DEVKITPPC)/lib $(CURDIR) $(CURDIR)/lib/png $(CURDIR)/lib/freetype $(CURDIR)/lib/libfat
LIBDIRS	:=	$(DEVKITPPC)/lib $(CURDIR) $(CURDIR)/lib/png $(CURDIR)/lib/libfat $(CURDIR)/lib/libntfs $(CURDIR)/lib/libext2fs $(CURDIR)/lib/jpeg $(CURDIR)/lib/grrlib $(PORTLIBS)

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export PATH 	:=	$(CURDIR)/$(GETTEXT):$(PATH)
export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

#---------------------------------------------------------------------------------
# automatically build a list of object files for our project
#---------------------------------------------------------------------------------
CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
sFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.S)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif

export OFILES	:=	$(addsuffix .o,$(BINFILES)) \
					$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) \
					$(sFILES:.s=.o) $(SFILES:.S=.o)

#---------------------------------------------------------------------------------
# build a list of include paths
#---------------------------------------------------------------------------------
export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD) \
					-I$(LIBOGC_INC) -I$(LIBOGC_INC)/ogc -I$(LIBOGC_INC)/ogc/machine

#---------------------------------------------------------------------------------
# build a list of library paths
#---------------------------------------------------------------------------------
export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib$(DKV) -L$(dir)/lib) \
					-L$(LIBOGC_LIB)

export OUTPUT	:=	$(CURDIR)/$(TARGET)
.PHONY: $(BUILD) 222 debug lang all clean tags

#---------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@echo
	@echo $(CURDIR)
	@echo Building $(TARGET) $(BUILD_FLAGS)
	@grep _V_STR $(LIBOGC_INC)/ogc/libversion.h | cut -f2 -d'"'
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

222:
	@$(MAKE) --no-print-directory BUILD_222=1

debug:
	@$(MAKE) --no-print-directory BUILD_DEBUG=1

#---------------------------------------------------------------------------------
lang:
	@[ -d Languages ] || mkdir -p Languages
	@echo
	@echo "Building Language File ($@$(VERSION).pot)"
	@xgettext --sort-output --no-wrap --no-location -k -kgt -kgts -o Languages/$@$(VERSION).pot source/*.c

#---------------------------------------------------------------------------------
all: $(BUILD) 222 lang
	@echo Done All
#	@make --no-print-directory build
#	@make --no-print-directory BUILD_222=1
#	@make --no-print-directory lang

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(OUTPUT).elf $(OUTPUT).dol
	@rm -fr $(BUILD)_222 $(OUTPUT)-222.elf $(OUTPUT)-222.dol
	@rm -fr $(BUILD)_dbg $(OUTPUT)-dbg.elf $(OUTPUT)-dbg.dol

cleanall: clean
	@rm -fr *.dol *.elf


#---------------------------------------------------------------------------------
run: 
	wiiload $(OUTPUT).dol debug=0 simple=0 home=exit gui=coverflow3d gui_style=grid gui_antialias=4 gui_compress_covers=1 music=sd://mp3 button_2=random
#	wiiload $(OUTPUT).dol simple=0 device=usb cover_style=3d widescreen=1 covers_size=100,100 debug=1 music=sd://mp3 gui_font=Vera_Mono_12_Bold.png hide_game=RB7E admin_lock=1 wpreview_coords=10,100,200,0
# widescreen = [auto], 0, 1
# home = [reboot], exit, screenshot
# cover_style = [standard], 3d, disc

DATE=$(shell date +%F)
SIZE=$(shell stat -c %s $(BINBASE).dol 2>/dev/null)
SIZE_222=$(shell stat -c %s $(BINBASE)-222.dol 2>/dev/null)

define ANNOUNCE_SH
CHANGES=`sed -n '/cfg v'$(VERSION)' /,/cfg v/{/cfg v/!p}' README-CFG.txt`
echo "===== updates.txt: ====="
cat << END

release = $(VERSION) ($(RELEASE))
size = $(SIZE)
date = $(DATE)
url = http://cfg-loader.googlecode.com/files/$(BINBASE).dol
$$CHANGES

release = $(VERSION)-222 ($(RELEASE))
size = $(SIZE_222)
date = $(DATE)
url = http://cfg-loader.googlecode.com/files/$(BINBASE)-222.dol
Same as $(VERSION) but with different
default options:
  ios=222-mload

END
echo "===== forum ====="
cat << END

[b]cfg v$(VERSION) ($(RELEASE))[/b]
[url="http://cfg-loader.googlecode.com/files/$(BINBASE).dol"]$(BINBASE).dol[/url]
[url="http://cfg-loader.googlecode.com/files/$(BINBASE)-222.dol"]$(BINBASE)-222.dol[/url]
[url="http://cfg-loader.googlecode.com/files/lang$(VERSION).zip"]lang$(VERSION).zip[/url]
(or online update)
[pre]Changes:

cfg v$(VERSION) ($(RELEASE))
$$CHANGES
[/pre]

END
endef
export ANNOUNCE_SH

announce:
	@echo "$$ANNOUNCE_SH" | sh

DOLS := $(BINBASE)-222.dol $(BINBASE).dol
ELFS := $(BINBASE)-222.elf $(BINBASE).elf
BINS := $(ELFS) $(DOLS)

upload:
	for F in $(DOLS); do echo uploading $$F ; $(UPLOAD) $(GAUTH) -p cfg-loader -s $$F $$F ; done

upload_elf:
	for F in $(ELFS); do echo uploading $$F ; $(UPLOAD) $(GAUTH) -p cfg-loader -s $$F $$F ; done

upload_lang:
	echo uploading Languages/lang$(VERSION).zip
	$(UPLOAD) $(GAUTH) -p cfg-loader -s lang$(VERSION).zip Languages/lang$(VERSION).zip

upload_full:
	echo uploading ../Cfg_USB_Loader_$(VERSION).zip
	$(UPLOAD) $(GAUTH) -p cfg-loader -s Cfg_USB_Loader_$(VERSION).zip ../Cfg_USB_Loader_$(VERSION).zip


install:
	cp $(BINBASE).dol $(INSTDIR)/inSDRoot/apps/USBLoader/boot.dol
	-rm $(INSTDIR)/*.dol
	#cp $(BINBASE)-222.dol $(INSTDIR)/
	cp README-CFG.txt $(INSTDIR)/
	sed -i 's/<version>.*</<version>'$(VERSION)' (release)</' $(INSTDIR)/inSDRoot/apps/USBLoader/meta.xml
	sed -i 's/<release_date>.*</<release_date>'`date +%Y%m%d000000`'</' $(INSTDIR)/inSDRoot/apps/USBLoader/meta.xml
	cp Languages/*.lang $(INSTDIR)/inSDRoot/usb-loader/languages
	-rm $(INSTDIR)/inSDRoot/usb-loader/languages/*_miss.lang
	cp Languages/lang.pot $(INSTDIR)/inSDRoot/usb-loader/languages

tags:
	cd source ; ctags -R .

#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).dol: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)

#---------------------------------------------------------------------------------
# This rule links in binary data with the .jpg extension
#---------------------------------------------------------------------------------
%.jpg.o	:	%.jpg
	@echo $(notdir $<)
	@$(bin2o)

%.png.o	:	%.png
	@echo $(notdir $<)
	@$(bin2o)

%.ttf.o : %.ttf
	@echo $(notdir $<)
	@$(bin2o)

%.wav.o : %.wav
	@echo $(notdir $<)
	@$(bin2o)

#%.o: %.c
#	@echo $(notdir $<)
#	$(CC) -MMD -MP -MF $(DEPSDIR)/$*.d $(CFLAGS) -c $< -o $@

-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
