files=org.nemiver.gschema.xml
gsettingsschema_DATA = $(files)

export files

install-data-local: install-schemas

uninstall-local: uninstall-schemas

install-schemas: install-gsettingsschemaDATA
	$(GLIB_COMPILE_SCHEMAS) $(gsettingsschemadir)

uninstall-schemas: uninstall-gsettingsschemaDATA
	$(GLIB_COMPILE_SCHEMAS) $(gsettingsschemadir)

EXTRA_DIST=$(files)
