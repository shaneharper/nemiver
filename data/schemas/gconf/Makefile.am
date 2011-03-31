files=nemiver-dbgperspective.schemas nemiver-workbench.schemas
schemasdir = $(GCONF_SCHEMA_FILE_DIR)
schemas_DATA = $(files)

GCONFTOOL=@GCONFTOOL@
export GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE)

export files

install-data-local: install-schemas

uninstall-local: uninstall-schemas

install-schemas:
if GCONF_SCHEMAS_INSTALL
	if test -z "$(DESTDIR)" ; then \
		for s in `echo $$files | tr ' ' '\n'` ; do \
		$(GCONFTOOL) --makefile-install-rule $(top_srcdir)/data/schemas/gconf/$$s ; \
		done ; \
		gconfpid=`pidof gconfd-2` ; \
		if test x$$gconfpid != x ; then \
		kill -HUP $$gconfpid ; \
		echo "gconf $$gconfpid reloaded its config" ;\
		fi \
	fi
endif # GCONF_SCHEMAS_INSTALL

uninstall-schemas:
if GCONF_SCHEMAS_INSTALL
	if test -z "$(DESTDIR)" ; then \
	for s in `echo $$files | tr ' ' '\n'` ; do $(GCONFTOOL) --makefile-uninstall-rule $(top_srcdir)/data/schemas/gconf/$$s ; done \
	fi
endif # GCONF_SCHEMAS_INSTALL

EXTRA_DIST=$(files)
