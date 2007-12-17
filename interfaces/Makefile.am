%.xml: %-full.xml
	$(XSLT) -o $@ $(top_srcdir)/docs/tools/spec-strip-docs.xsl $<

noinst_DATA = \
	gc-iface-geoclue.xml \
	gc-iface-position.xml \
	gc-iface-address.xml \
	gc-iface-geocode.xml \
	gc-iface-reverse-geocode.xml \
	gc-iface-velocity.xml

BUILT_SOURCES = $(noinst_DATA)
CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
	gc-iface-geoclue-full.xml \
	gc-iface-position-full.xml \
	gc-iface-address-full.xml \
	gc-iface-geocode-full.xml \
	gc-iface-reverse-geocode-full.xml \
	gc-iface-velocity-full.xml
