How to do a Nemiver release

* Make sure configure.ac has the right version variables set
 - These are MAJOR_VERSION, MINOR_VERSION, MICRO_VERSION and the version part of
  the AC_INIT macro invocation.  Normally these should have been
  already set, but you never know.
 - Commit this.

* Update the NEWS file:
 - Run git shortlog <nemiver-last-tag-name>..HEAD
   This gives the list of changes by author. Prepend it to the NEWS
   file and edit the new entries to make them comply with the rest of
   the NEW items.  Also remove the redundant and not meaningful
   entries.
 - Commit this.

* Update the ChangeLog
 - Run make update-changelog.  This augments the ChangeLog file with
   information coming from the new commits that happened since the
   last time this command was run.
 - Commit this

* Run make release
 - This will tag the release, generate a tarball, and copy it to the gnome machine.
 - Connect to the gnome machine and run udpate-module <tarball>
 - Copy the announcement template text pasted on the terminal, amend
   it and sent the announcement to nemiver-list@gnome.org,
   gnome-announce-list@gnome.org and gnome-devtools@gnome.org.
 - Update the nemiver website for the new release.  It's in the git
   repository git.gnome.org/git/gnomeweb-wml.
