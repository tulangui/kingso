# $Id: Makefile.am,v 1.5 2009/05/26 02:44:07 hightman Exp $
# automake file by hightman for SCWS-0.1.x
#

AUTOMAKE_OPTIONS = foreign

SUBDIRS = . libscws cli etc

DIST_SUBDIRS = libscws cli etc

EXTRA_DIST = COPYING README ChangeLog phpext win32

dist-hook:
	-rm -rf $(distdir)/phpext/CVS $(distdir)/phpext/win32/CVS $(distdir)/win32/CVS
