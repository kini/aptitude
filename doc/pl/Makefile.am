SUBDIRS = 
EXTRA_DIST = fixman

LC = pl

PO4A_TRANSLATED = aptitude.xml manpage.xml

include $(top_srcdir)/buildlib/doc.mk
include $(top_srcdir)/buildlib/po4a.mk

DOCBOOK_MANS = aptitude.8
DOCBOOK_TARGETS = docbook-man

include $(top_srcdir)/buildlib/docbook.mk

all-local: docbook-stamp
