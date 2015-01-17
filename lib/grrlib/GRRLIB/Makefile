# Quick'n'dirty makefile [BC] v2

ifeq ($(strip $(DEVKITPPC)),)
  $(error "Use   export DEVKITPPC=<path to>devkitPPC   and try again")
endif

ifeq ($(strip $(DEVKITPRO)),)
  $(error "Use   export DEVKITPRO=<path to>devkitPRO   and try again")
endif

include $(DEVKITPPC)/wii_rules

NULLSTR :=
PWD     := $(subst $(NULLSTR) ,\ ,$(shell pwd))

#PREFIX	:= $(DEVKITPPC)/bin/powerpc-eabi-
#CC		:=	$(PREFIX)gcc
#AR		:=	$(PREFIX)ar

LIBOGC_INC	:=	$(DEVKITPRO)/libogc/include
LIBOGC_LIB	:=	$(DEVKITPRO)/libogc/lib/wii

#INCLUDE	:= -I../lib/jpeg -I../lib/pngu -I../lib/freetype/include -I$(PWD) -I$(LIBOGC_INC)
INCLUDE	:= -I../../jpeg/include -I../../../source/pngu -I$(PWD) -I$(LIBOGC_INC)
MACHDEP := -DGEKKO -mrvl -mcpu=750 -meabi -mhard-float
CFLAGS  := -Os -Wall $(MACHDEP) $(INCLUDE)

LIB		:= grrlib
CFILES	:= $(wildcard *.c)
OFILES	:= $(CFILES:.c=.o)
ARC		:= lib$(LIB).a
HDR		:= $(LIB).h
INL		:= $(wildcard $(LIB)/*.h)

#all : $(OFILES) $(ARC)
#	$(AR) -r $(ARC) $(OFILES)

all : $(ARC)

$(ARC) : $(OFILES)

clean :
	rm -f $(OFILES) $(ARC)

install_ogc :
	mkdir -p  $(LIBOGC_LIB)  $(LIBOGC_INC)  $(LIBOGC_INC)/grrlib
	cp -f $(ARC) $(LIBOGC_LIB)/
	cp -f $(HDR) $(LIBOGC_INC)/
	cp -f $(INL) $(LIBOGC_INC)/grrlib

DKV			:= $(shell $(DEVKITPPC)/bin/$(CC) --version | sed 's/^.*(devkitPPC release \([0-9]*\)).*$$/\1/;q')
DEST_INC	:=	../include
DEST_LIB	:=	../lib$(DKV)

install :
	mkdir -p  $(DEST_LIB)  $(DEST_INC)  $(DEST_INC)/grrlib
	cp -f $(ARC) $(DEST_LIB)/
	cp -f $(HDR) $(DEST_INC)/
	cp -f $(INL) $(DEST_INC)/grrlib

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@
