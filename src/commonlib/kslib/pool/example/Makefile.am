INCLUDES = -I$(top_srcdir)/src
noinst_PROGRAMS = example

example_SOURCES = Example.cpp

example_LDADD = $(top_builddir)/src/libpool.la -lpthread
example_LDFLAGS = $(LIBADD_DL)

