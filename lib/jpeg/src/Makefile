# Quick'n'dirty makefile [BC] v2

ifeq ($(strip $(DEVKITPPC)),)
  $(error "Use   export DEVKITPPC=<path to>devkitPPC   and try again")
endif

ifeq ($(strip $(DEVKITPRO)),)
  $(error "Use   export DEVKITPRO=<path to>devkitPRO   and try again")
endif

include $(DEVKITPPC)/wii_rules

#PREFIX	:= $(DEVKITPPC)/bin/powerpc-eabi-
#CC		:=	$(PREFIX)gcc
#AR		:=	$(PREFIX)ar

LIBOGC_INC	:=	$(DEVKITPRO)/libogc/include
LIBOGC_LIB	:=	$(DEVKITPRO)/libogc/lib/wii
DEPSDIR := .

DKV			:= $(shell $(DEVKITPPC)/bin/$(CC) --version | sed 's/^.*(devkitPPC release \([0-9]*\)).*$$/\1/;q')
DEST_INC	:=	../include
DEST_LIB	:=	../lib$(DKV)

INCLUDE	:= 
MACHDEP := -DGEKKO -mrvl -mcpu=750 -meabi -mhard-float
CFLAGS  := -Os $(MACHDEP) $(INCLUDE)

LIB 	:= jpeg
CFILES	:= $(wildcard *.c)
OFILES	:= $(CFILES:.c=.o)
DEPENDS	:= $(OFILES:.o=.d)
ARC 	:= lib$(LIB).a
HDR 	:= jpeglib.h jconfig.h jmorecfg.h

#all : $(OFILES) $(ARC)
#	$(AR) -r $(ARC) $(OFILES)

all : $(ARC)

$(ARC) : $(OFILES)

clean :
	rm -f $(OFILES) $(ARC) $(DEPENDS)

install :
	mkdir -p  $(DEST_LIB)  $(DEST_INC)
	cp -f $(ARC) $(DEST_LIB)/
	cp -f $(HDR) $(DEST_INC)/

#%.o : %.c
#	$(CC) $(CFLAGS) -c $< -o $@

