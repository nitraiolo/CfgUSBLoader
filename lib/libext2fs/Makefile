
#default: cube-release wii-release
default: wii-release

all: debug release

debug: cube-debug wii-debug

release: cube-release wii-release

cube-debug:
	$(MAKE) -C source PLATFORM=cube BUILD=cube_debug

wii-debug:
	$(MAKE) -C source  PLATFORM=wii BUILD=wii_debug

cube-release:
	$(MAKE) -C source  PLATFORM=cube BUILD=cube_release

wii-release:
	$(MAKE) -C source  PLATFORM=wii BUILD=wii_release

clean: 
	$(MAKE) -C source clean

install_ogc: cube-release wii-release
	$(MAKE) -C source install

run: install
	$(MAKE) -C example
	$(MAKE) -C example run

include $(DEVKITPPC)/wii_rules
DKV			:= $(shell $(DEVKITPPC)/bin/$(CC) --version | sed 's/^.*(devkitPPC release \([0-9]*\)).*$$/\1/;q')
DEST_INC	:=	include
DEST_LIB	:=	lib$(DKV)

install: wii-release
	mkdir -p $(DEST_LIB)
	cp lib/wii/libext2fs.a $(DEST_LIB)

