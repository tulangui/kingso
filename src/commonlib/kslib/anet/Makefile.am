SUBDIRS=src/anet test util

.PHONY: doc rpm

doc:
	$(mkdir_p) @DOXYGEN_OUTPUT_DIR@
	doxygen

rpm: dist anet.spec
	$(mkdir_p) rpmbuild/BUILD  rpmbuild/RPMS  rpmbuild/SOURCES
	$(mkdir_p)  rpmbuild/SPECS  rpmbuild/SRPMS  rpmbuild/tmp
	$(RM) rpmbuild/SPECS/anet.spec
	$(LN_S) `pwd`/anet.spec rpmbuild/SPECS
	$(RM) rpmbuild/SOURCES/$(distdir).tar.gz
	$(LN_S) `pwd`/$(distdir).tar.gz rpmbuild/SOURCES
	rpmbuild -ba rpmbuild/SPECS/anet.spec
clean: clean-recursive
	rm -rf rpmbuild;



