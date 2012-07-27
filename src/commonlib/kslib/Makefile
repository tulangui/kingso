PKGS=mxml libmemcached alog pool anet gtest ksutil kslib
CLEANS=mxml-clean libmemcached-clean alog-clean pool-clean pool-clean anet-clean anet-clean gtest-clean ksutil-clean kslib-clean
UNINSTALLS=mxml-uninstall libmemcached-uninstall alog-uninstall pool-uninstall anet-uninstall gtest-uninstall ksutil-uninstall kslib-uninstall
INSTALLS=mxml-install libmemcached-install alog-install pool-install anet-install gtest-install ksutil-install kslib-install
CURDIR=$(shell pwd)
INSTALLDIR=$(CURDIR)/installed
PREFIX=$(prefix)
MK_OPTS=-j8
CONFIGURE_OPT=CXXFLAGS="-fPIC -g" CFLAGS="-fPIC -g"

all:: $(PKGS)

clean:: $(CLEANS)

uninstall:: $(UNINSTALLS)

install:: $(INSTALLS)

mxml::
		cd mxml/ && ./configure --prefix=$(INSTALLDIR)/mxml $(CONFIGURE_OPT) && make $(MK_OPTS) && make install
mxml-clean::
		rm -rf $(INSTALLDIR)/mxml
mxml-uninstall::
		rm -rf $(INSTALLDIR)/mxml
mxml-install::
		mkdir -p $(PREFIX)/include/ && cd mxml/ && cp mxml.h $(PREFIX)/include/.

libmemcached::
	cd libmemcached/ && ./configure --enable-shared=no --prefix=$(INSTALLDIR)/libmemcached $(CONFIGURE_OPT) && make $(MK_OPTS) && make install
libmemcached-clean::
	rm -rf $(INSTALLDIR)/libmemcached
libmemcached-uninstall::
	rm -rf $(INSTALLDIR)/libmemcached
libmemcached-install::
	cd libmemcached && ./configure --enable-shared=no --prefix=$(PREFIX) && make $(MK_OPTS) install-data

pool::
		cd pool && ./autogen.sh && ./configure --prefix=$(INSTALLDIR)/pool $(CONFIGURE_OPT) && make $(MK_OPTS) && make install
pool-clean::
		rm -rf $(INSTALLDIR)/pool
pool-uninstall::
		cd pool && make uninstall-data
pool-install::
		cd pool && ./configure --prefix=$(PREFIX) $(CONFIGURE_OPT) && make $(MK_OPTS) install-data

alog::
		cd alog && ./autogen.sh && ./configure --prefix=$(INSTALLDIR)/alog $(CONFIGURE_OPT) && make $(MK_OPTS) && make install
alog-clean::
		rm -rf $(INSTALLDIR)/alog
alog-uninstall::
		cd alog/src/cpp && make uninstall-data
alog-install::
		cd alog && ./configure --prefix=$(PREFIX) $(CONFIGURE_OPT) && make $(MK_OPTS) install-data	

anet::
		cd anet && ./autogen.sh && ./configure --prefix=$(INSTALLDIR)/anet --with-alog=$(INSTALLDIR)/alog && make $(MK_OPTS) $(CONFIGURE_OPT) && make install
anet-clean::
		rm -rf $(INSTALLDIR)/anet
anet-uninstall::
		cd anet/src/anet/ && make uninstall-data
anet-install::
		cd anet/ && ./configure --prefix=$(PREFIX) --with-alog=$(INSTALLDIR)/alog $(CONFIGURE_OPT) && make install-data

gtest::
		cd gtest && ./configure --prefix=$(INSTALLDIR)/gtest $(CONFIGURE_OPT) && make $(MK_OPTS) && make install
gtest-clean::
		rm -rf $(INSTALLDIR)/gtest
gtest-uninstall::
		rm -rf $(INSTALLDIR)/gtest
gtest-install::
		cd gtest/ && ./configure --prefix=$(PREFIX) $(CONFIGURE_OPT) && make install-data

ksutil::
		cd ksutil && ./autogen.sh && ./configure --prefix=$(INSTALLDIR)/ksutil --with-mxml=$(INSTALLDIR)/mxml/ --with-alog=$(INSTALLDIR)/alog/ --with-pool=$(INSTALLDIR)/pool/ $(CONFIGURE_OPT) && make $(MK_OPTS) && make install
ksutil-clean::
		rm -rf $(INSTALLDIR)/ksutil
ksutil-uninstall::
		cd ksutil && make uninstall-data
ksutil-install::
		cd ksutil && ./configure --prefix=$(PREFIX) --with-mxml=$(INSTALLDIR)/mxml/ --with-alog=$(INSTALLDIR)/alog/ --with-pool=$(INSTALLDIR)/pool/ $(CONFIGURE_OPT) && make install-data 

kslib::
		mkdir -p $(INSTALLDIR)/kslib && cd $(INSTALLDIR)/kslib/ && ar -x $(INSTALLDIR)/alog/lib/libalog.a && ar -x $(INSTALLDIR)/anet/lib/libanet.a && ar -x $(INSTALLDIR)/pool/lib/libpool.a && ar -x $(INSTALLDIR)/mxml/lib/libmxml.a && ar -x $(INSTALLDIR)/libmemcached/lib/libmemcached.a && ar -x $(INSTALLDIR)/ksutil/lib/libksutil.a && ar cru libkslib.a *.o && ranlib libkslib.a && rm -rf $(INSTALLDIR)/kslib/*.o
kslib-clean::
	        rm -rf $(INSTALLDIR)/kslib
kslib-uninstall::
	rm -rf $(INSTALLDIR)/kslib
kslib-install::
	mkdir -p $(PREFIX)/lib/ && cp $(INSTALLDIR)/kslib/libkslib.a $(PREFIX)/lib/.


