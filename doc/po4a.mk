TRANSLATED=aptitude.xml manpage.xml

PO4AFLAGS="--srcdir=$(top_srcdir)/doc" "--destdir=$(top_builddir)/doc" \
	"$(top_srcdir)/doc/po4a/po4a.cfg"

clean-local: po4a-clean-local
po4a-clean-local:
	-rm -fr $(TRANSLATED)

%.xml %.svg: $(top_srcdir)/doc/po4a/po/$(LANGCODE).po
	if [ -n "$(PO4A)" ]; then \
	  targ="$(LANGCODE)/$@"; \
	  $(PO4A) "--translate-only=$${targ}" $(PO4AFLAGS); \
	  rm -f "$(top_srcdir)/doc/$${targ}" | true; \
	fi

aptitude.xml: $(top_srcdir)/doc/en/aptitude.xml
manpage.xml: $(top_srcdir)/doc/en/manpage.xml
images/safety-cost-level-diagram.svg: $(top_srcdir)/doc/en/images/safety-cost-level-diagram.svg

.PHONY: update-po
