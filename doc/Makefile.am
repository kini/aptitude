SUBDIRS=cs de en es fi fr it ja pl po4a

EXTRA_DIST=aptitude-common.xsl aptitude-html.xsl aptitude-man.xsl aptitude-txt.xsl aptitude.css html-to-text

PO4AFLAGS="--srcdir=$(top_srcdir)/doc" "--destdir=$(top_builddir)/doc" \
	"$(top_srcdir)/doc/po4a/po4a.cfg"

dist distdir: update-po

update-po:
	if [ -n "$(PO4A)" ]; then \
	  $(PO4A) \
	    --no-translations \
	    "--package-name=$(PACKAGE)" "--package-version=$(VERSION)" \
	    $(PO4AFLAGS); \
	fi

.PHONY: update-po
