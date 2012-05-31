MANPAGE=aptitude.$(LANGCODE).8
README=README.$(LANGCODE)

HTML2TEXT=$(top_srcdir)/doc/html-to-text

# Put documentation in /usr/share/doc/aptitude (by default)
localemandir=$(mandir)/$(LANGCODE)
htmldir=$(docdir)/html/$(LANGCODE)

pkgdata_DATA=$(README)

images/%.png: images/%.svg
	rsvg-convert -x 1.5 -y 1.5 -f png -o $@ $<

all-local: doc-stamp

clean-local:
	-rm -fr output-html/ output-txt/ output-man/
	-rm -f doc-stamp doc-css-stamp doc-html-stamp doc-txt-stamp doc-man-stamp
	-rm -fr $(MANPAGE) $(README) *.tmp

doc-stamp: doc-html-stamp doc-css-stamp $(README) $(MANPAGE)
	touch doc-stamp

install-data-hook:
	$(mkinstalldirs) $(DESTDIR)$(localemandir)/man8
	$(INSTALL_DATA) $(MANPAGE) $(DESTDIR)$(localemandir)/man8/aptitude.8

	$(mkinstalldirs) $(DESTDIR)$(htmldir)/images
	$(INSTALL_DATA) output-html/*.html output-html/*.css $(DESTDIR)$(htmldir)
	$(INSTALL_DATA) output-html/images/* $(DESTDIR)$(htmldir)/images

$(MANPAGE): $(XMLSOURCES) $(top_srcdir)/doc/aptitude-man.xsl
	-rm -fr output-man $(MANPAGE)
	xsltproc -o output-man/aptitude.8 $(top_srcdir)/doc/aptitude-man.xsl aptitude.xml
	mv output-man/aptitude.8 $(MANPAGE)

	if [ -x "$(srcdir)/fixman" ]; then \
	  "$(srcdir)/fixman"; \
	fi

$(README): $(XMLSOURCES) $(top_srcdir)/doc/aptitude-txt.xsl
	-rm -fr output-txt/
	xsltproc -o output-txt/index.html $(top_srcdir)/doc/aptitude-txt.xsl aptitude.xml
	$(HTML2TEXT) output-txt/index.html $(README_encoding) > $(README)

doc-css-stamp: doc-html-stamp $(top_srcdir)/doc/aptitude.css
	rm -f output-html/aptitude.css
	cp $(top_srcdir)/doc/aptitude.css output-html/
	touch doc-css-stamp

doc-html-stamp: $(XMLSOURCES) $(top_srcdir)/doc/aptitude-html.xsl $(IMAGES)
	-rm -fr output-html/

	xsltproc -o output-html/ $(top_srcdir)/doc/aptitude-html.xsl aptitude.xml

	mkdir output-html/images/
	ln -f $(srcdir)/images/*.png output-html/images/
	for x in caution important note tip warning; do \
	  ln -s /usr/share/xml/docbook/stylesheet/nwalsh/images/$$x.png \
	      output-html/images/; \
	done
	for x in home next prev up; do \
	  ln -s /usr/share/xml/docbook/stylesheet/nwalsh/images/$$x.gif \
	      output-html/images/; \
	done

	touch doc-html-stamp
