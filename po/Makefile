# Makefile for various po files.

srcdir = .
libdir = ../lib

#CATALOGS = $(addsuffix .po, LINGUAS)
CATALOGS = $(LINGUAS)
MO_FILES = $(addsuffix .mo, $(LINGUAS))

MSGMERGE = msgmerge
MSGFMT   = msgfmt
XGETTEXT = xgettext
CATOBJEXT = .po

include PACKAGE

TD = $(strip $(TEXTDOMAIN))

default: help

all: $(TD).pot update-po update-mo 

#install

#@echo "  pot                       - remake master catalog"
#@echo "  install                   - install mo files"

help:
	@echo "Available targets:"
	@echo "  update-po                 - merge po files"
	@echo "  update-mo                 - regenerate mo files"
	@echo "  all			   - all of the above"

POTFILES = $(srcdir)/POTFILES.in \
	$(shell cat $(srcdir)/POTFILES.in) 

pot: $(TD).pot 

clean:
	rm -f *~ *.bak *.mo

# FIXME: The parameter --from-code is only needed if your sources contain
# any 8 bit data (even in comments).  UTF-8 is only a guess here, but it
# will at least accept any 8 bit data.
#
# The parameter "--language=perl" is not strictly needed because the
# source language of all our files will be auto-detected by xgettext
# by their filename extension.  You should even avoid this parameter
# if you want to extract strings from multiple source languages.
$(TD).pot: $(POTFILES)
	$(XGETTEXT) --output=$(srcdir)/$(TD).pox --from-code=utf-8 \
		--add-comments=TRANSLATORS: --files-from=$(srcdir)/POTFILES.in \
		--copyright-holder="$(COPYRIGHT_HOLDER)" \
		--msgid-bugs-address="$(MSGID_BUGS_ADDRESS)" \
		--keyword --keyword='$$__' --keyword=__ --keyword=__x \
		--keyword=__n:1,2 --keyword=__nx:1,2 --keyword=__xn:1,2 \
		--keyword=__p:1c,2 --keyword=__np:1c,2,3 \
		--keyword=__npx:1c,2,3 --keyword=N__ --keyword=N__n:1,2 \
		--keyword=N__p:1c,2 --keyword=N__np:1c,2,3 --keyword=%__ \
		--language=perl && \
	rm -f $@ && mv $(TD).pox $@

install: $(MO_FILES)
	cd $(srcdir); 
	targetdir='$(libdir)/LocaleData'; \
	languages='$(LINGUAS)'; \
	for lang in $$languages; do \
		mkdir -p "$$targetdir/$$lang/LC_MESSAGES" || exit 1; \
		dest="$$targetdir/$$lang/LC_MESSAGES/$(TD).mo"; \
		cat="$$lang.mo"; \
		echo "installing $$cat as $$dest"; \
		cp -f $$cat $$dest && chmod 644 $$dest || exit 1; \
	done

update-mo-old: $(MO_FILES)

MO_PATH=../lib/LocaleData/

update-mo:
	@for i in $$(ls *.po); do \
	mkdir -p $(MO_PATH)$${i%.po}/LC_MESSAGES ; \
	msgfmt $$i -o $(MO_PATH)$${i%.po}/LC_MESSAGES/$(TD).mo; \
	echo "$$i -> $(MO_PATH)$${i%.po}/LC_MESSAGES/$(TD).mo"; \
	done

update-po: 
	$(MAKE) $(TD).pot
	@echo Merging...
	@for i in $$(ls *.po); do \
	cp $$i $$i~; \
	echo -n "$$i "; \
	msgmerge $$i~ $(TD).pot > $$i; \
	done
	@echo

update-po_old:
	$(MAKE) $(TD).pot
	cd $(srcdir); \
        catalogs='$(CATALOGS)'; \
        for cat in $$catalogs; do \
          cat=`basename $$cat`; \
          lang=`echo $$cat | sed 's/\$(CATOBJEXT)$$//'`; \
          mv $$lang.po $$lang.old.po; \
          echo "$$lang:"; \
          if $(MSGMERGE) $$lang.old.po $(TD).pot -o $$lang.po; then \
            rm -f $$lang.old.po; \
          else \
            echo "msgmerge for $$cat failed!"; \
            rm -f $$lang.po; \
            mv $$lang.old.po $$lang.po; \
          fi; \
        done

.SUFFIXES:
.SUFFIXES: .po .mo

.po.mo:
	$(MSGFMT) --check --statistics --verbose -o $@ $<

