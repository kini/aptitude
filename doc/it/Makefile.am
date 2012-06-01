SUBDIRS = images
EXTRA_DIST =

LC = it

include $(top_srcdir)/buildlib/doc.mk
include $(top_srcdir)/buildlib/po4a.mk

README = README.it
README_encoding = utf-8

IMAGES := $(wildcard $(srcdir)/images/*.png) $(PO4A_IMAGES)

DOCBOOK_XML = aptitude.xml manpage.xml
DOCBOOK_TARGETS = docbook-html docbook-man docbook-readme

include $(top_srcdir)/buildlib/docbook.mk

all-local: $(DOCBOOK_TARGETS)
