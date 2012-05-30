# automake doesn't play nicely with DocBook :(

include $(top_srcdir)/doc/common.mk
include $(top_srcdir)/doc/po4a.mk

SUBDIRS=images
EXTRA_DIST=fixman
LANGCODE=fr

XMLSOURCES=aptitude.xml manpage.xml

IMAGES=$(wildcard $(srcdir)/images/*.png) images/safety-cost-level-diagram.png
