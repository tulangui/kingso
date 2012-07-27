SUBDIRS=src conf src/tools scripts

rpm: dist kingso_searcher.spec
	mkdir -p rpmbuild/BUILD  rpmbuild/RPMS  rpmbuild/SOURCES;
	mkdir -p rpmbuild/SPECS  rpmbuild/SRPMS  rpmbuild/tmp;
	ln -sf `pwd`/kingso_searcher.spec rpmbuild/SPECS
	ln -sf `pwd`/kingso_searcher-@KINGSO_SEARCHER_MAJOR_VERSION@.@KINGSO_SEARCHER_MINOR_VERSION@.@KINGSO_SEARCHER_MICRO_VERSION@.tar.gz rpmbuild/SOURCES;
	rpmbuild -ba rpmbuild/SPECS/kingso_searcher.spec

clean: clean-recursive
	    rm -rf rpmbuild;

