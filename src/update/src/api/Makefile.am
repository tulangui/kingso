AM_LDFLAGS = -static-libtool-libs

lib_LTLIBRARIES=libupdate_api.la libupdate.la

libupdate_api_la_SOURCES=update_api.cpp \
						 $(NULL)
libupdate_api_la_LDFLAGS=-lupdater_common -L../common/

libupdate_la_SOURCES=DetailUpdater.cpp \
					 IndexUpdater.cpp \
					 Updater.cpp \
					 UpdateProcessor.cpp \
					 $(NULL)
libupdate_la_LDFLAGS=-lupdater_common -L../common/

DEFS+=$(MACRO)
