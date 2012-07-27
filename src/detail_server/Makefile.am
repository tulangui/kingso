SUBDIRS=src conf

rpm: dist kingso_detail.spec
	mkdir -p rpmbuild/BUILD  rpmbuild/RPMS  rpmbuild/SOURCES;
	mkdir -p rpmbuild/SPECS  rpmbuild/SRPMS  rpmbuild/tmp;
	ln -sf `pwd`/kingso_detail.spec rpmbuild/SPECS
	ln -sf `pwd`/kingso_detail-@KINGSO_DETAIL_MAJOR_VERSION@.@KINGSO_DETAIL_MINOR_VERSION@.@KINGSO_DETAIL_MICRO_VERSION@.tar.gz rpmbuild/SOURCES;
	rpmbuild -ba rpmbuild/SPECS/kingso_detail.spec

clean: clean-recursive
	rm -rf rpmbuild;
