AM_LDFLAGS=-static-libtool-libs

bindir=$(prefix)/tools
bin_PROGRAMS=queryHasher modifyEt ks_visit_stat

queryHasher_SOURCES=QueryCacheHash.cpp
queryHasher_LDADD=-lssl -lpthread -lkslib -lcommdef

modifyEt_SOURCES=modifyEt.cpp
modifyEt_LDADD=

ks_visit_stat_SOURCES=ks_visit_stat.cpp VisitStat.cpp VisitStatArgs.cpp VisitResult.cpp VisitStatSync.cpp
ks_visit_stat_LDADD=-lrt -ldl
