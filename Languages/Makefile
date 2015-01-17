
VERSION = $(shell sed -n '/^VERSION/{s/^.*=[\t ]*//p;Q}' ../Makefile)

all: zip

clean:
	rm -f *miss.lang *.zip lang?*.pot

zip: clean
	echo $(VERSION)
	./msg_merge.sh lang.pot
	./msg_miss.sh
	zip lang$(VERSION).zip *.lang lang.pot

