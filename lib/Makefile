
LIBS := grrlib/GRRLIB jpeg/src libfat/src libntfs/src png/src zlib/src libext2fs

all:
	for L in $(LIBS); do echo; echo = $$L =; $(MAKE) --no-print-directory -C $$L; done

install:
	for L in $(LIBS); do $(MAKE) -C $$L install; done

clean:
	for L in $(LIBS); do $(MAKE) -C $$L clean; done

