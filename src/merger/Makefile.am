SUBDIRS=src/frame src/plugin test conf

rpm: dist kingso_merger.spec
	mkdir -p rpmbuild/BUILD  rpmbuild/RPMS  rpmbuild/SOURCES;
	mkdir -p rpmbuild/SPECS  rpmbuild/SRPMS  rpmbuild/tmp;
	ln -sf `pwd`/kingso_merger.spec rpmbuild/SPECS
	ln -sf `pwd`/kingso_merger-@KINGSO_MERGER_MAJOR_VERSION@.@KINGSO_MERGER_MINOR_VERSION@.@KINGSO_MERGER_MICRO_VERSION@.tar.gz rpmbuild/SOURCES;
	rpmbuild -ba rpmbuild/SPECS/kingso_merger.spec

clean: clean-recursive
	rm -rf rpmbuild;
