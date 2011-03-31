SUBDIRS=icons schemas

desktopdir = $(datadir)/applications
desktop_in_files = nemiver.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

man_MANS = nemiver.1

EXTRA_DIST= \
$(desktop_in_files) \
$(man_MANS)

DISTCLEANFILES = $(desktop_DATA)
