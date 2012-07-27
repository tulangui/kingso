
SUBDIRS=src/cpp src/example src/test

CURDIR=$(shell pwd)
export INCLUDE=-I${CURDIR}/src/include

doc:
	doxygen

rpm: dist alog.spec
	mkdir -p rpmbuild/BUILD  rpmbuild/RPMS  rpmbuild/SOURCES;
	mkdir -p rpmbuild/SPECS  rpmbuild/SRPMS  rpmbuild/tmp;
	ln -sf `pwd`/alog.spec rpmbuild/SPECS
	ln -sf `pwd`/alog-@ALOG_MAJOR_VERSION@.@ALOG_MINOR_VERSION@.@ALOG_MICRO_VERSION@.tar.gz rpmbuild/SOURCES;
	rpmbuild -ba rpmbuild/SPECS/alog.spec
clean: clean-recursive
	rm -rf rpmbuild;
