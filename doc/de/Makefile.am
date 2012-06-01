SUBDIRS =
EXTRA_DIST =

LC = de

PO4A_TRANSLATED = aptitude.xml manpage.xml

include $(top_srcdir)/buildlib/doc.mk
include $(top_srcdir)/buildlib/po4a.mk

DOCBOOK_TARGETS = docbook-man

include $(top_srcdir)/buildlib/docbook.mk

all-local: $(DOCBOOK_TARGETS)
