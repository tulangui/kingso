AUTOMAKE_OPTIONS = foreign 1.4

SUBDIRS=src test example

EXTRA_DIST =\
    autogen.sh\
    Doxygen

rpm: dist pool.spec
	mkdir -p rpmbuild/BUILD  rpmbuild/RPMS  rpmbuild/SOURCES;
	mkdir -p rpmbuild/SPECS  rpmbuild/SRPMS  rpmbuild/tmp;
	ln -sf `pwd`/pool.spec rpmbuild/SPECS
	ln -sf `pwd`/pool-@POOL_MAJOR_VERSION@.@POOL_MINOR_VERSION@.@POOL_MICRO_VERSION@.tar.gz rpmbuild/SOURCES;
	rpmbuild -ba rpmbuild/SPECS/pool.spec

clean: clean-recursive
	rm -rf rpmbuild;
doxygen:
	doxygen Doxygen
