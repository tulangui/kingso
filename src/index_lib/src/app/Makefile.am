AM_LDFLAGS=-static-libtool-libs
DEFS+=$(MACRO)

bin_PROGRAMS=buildIndex buildProfile DocIdDictCreater BuildHFIndex idxBin2Txt profileBin2Txt BuildHFIndex docidMgrDriver

buildIndex_SOURCES=BuildIndex.cpp
buildIndex_LDADD=$(top_srcdir)/src/libindex_lib.la -lssl -lpthread -lkslib -lrt -lcommdef -lprotobuf

buildProfile_SOURCES=ProfileBuilder.cpp
buildProfile_LDADD=$(top_srcdir)/src/libindex_lib.la -lssl -lpthread -lkslib -lcommdef -lprotobuf

DocIdDictCreater_SOURCES=$(top_srcdir)/tools/DocIdDictCreater.cpp
DocIdDictCreater_LDADD=$(top_srcdir)/src/libindex_lib.la -lssl -lpthread -lkslib -lcommdef -lprotobuf

BuildHFIndex_SOURCES=BuildHFIndex.cpp
BuildHFIndex_LDADD=$(top_srcdir)/src/libindex_lib.la -lssl -lpthread -lkslib -lcommdef -lprotobuf

idxBin2Txt_SOURCES=idxBin2Txt.cpp
idxBin2Txt_LDADD=$(top_srcdir)/src/libindex_lib.la -lssl -lpthread -lkslib -lcommdef -lprotobuf

profileBin2Txt_SOURCES=ProfileBin2Txt.cpp
profileBin2Txt_LDADD=$(top_srcdir)/src/libindex_lib.la -lssl -lpthread -lkslib -lcommdef -lprotobuf

docidMgrDriver_SOURCES=DocIdManagerDriver.cpp
docidMgrDriver_LDADD=$(top_srcdir)/src/libindex_lib.la -lssl -lpthread -lkslib -lcommdef -lprotobuf

noinst_PROGRAMS=searchIndex makeBitmap profileTester profileLoadDriver printProfileDesc profileLoadTester profileGetValueNum profileGetSingleValue profileGetMultiValue profileGetMultiValuePtr realpostfeeDriver cmpTxtIdx latlngDriver getDistance

searchIndex_SOURCES=SearchIndex.cpp
searchIndex_LDADD=$(top_srcdir)/src/libindex_lib.la -lssl -lpthread -lkslib -lcommdef -lprotobuf
#searchIndex_LDFLAGS=$(CXXFLAGS) -pg

makeBitmap_SOURCES=makeBitmap.cpp
makeBitmap_LDADD=$(top_srcdir)/src/libindex_lib.la -lssl -lpthread -lkslib -lcommdef -lprotobuf

profileTester_SOURCES=ProfileTester.cpp
profileTester_LDADD=$(top_srcdir)/src/libindex_lib.la -lssl -lpthread -lkslib -lcommdef -lprotobuf

profileLoadDriver_SOURCES=ProfileLoadDriver.cpp
profileLoadDriver_LDADD=$(top_srcdir)/src/libindex_lib.la -lssl -lpthread -lkslib -lcommdef -lprotobuf

printProfileDesc_SOURCES=ProfileDescInfoReader.cpp
printProfileDesc_LDADD=$(top_srcdir)/src/libindex_lib.la -lssl -lpthread -lkslib -lcommdef -lprotobuf

profileLoadTester_SOURCES=ProfileLoadTester.cpp
profileLoadTester_LDADD=$(top_srcdir)/src/libindex_lib.la -lssl -lpthread -lkslib -lcommdef -lprotobuf

profileGetValueNum_SOURCES=ProfileGetValueNum.cpp
profileGetValueNum_LDADD=$(top_srcdir)/src/libindex_lib.la -lssl -lpthread -lkslib -lcommdef -lprotobuf

profileGetSingleValue_SOURCES=ProfileGetSingleValue.cpp
profileGetSingleValue_LDADD=$(top_srcdir)/src/libindex_lib.la -lssl -lpthread -lkslib -lcommdef -lprotobuf

profileGetMultiValue_SOURCES=ProfileGetMultiValue.cpp
profileGetMultiValue_LDADD=$(top_srcdir)/src/libindex_lib.la -lssl -lpthread -lkslib -lcommdef -lprotobuf

profileGetMultiValuePtr_SOURCES=ProfileGetMultiValuePtr.cpp
profileGetMultiValuePtr_LDADD=$(top_srcdir)/src/libindex_lib.la -lssl -lpthread -lkslib -lcommdef -lprotobuf

realpostfeeDriver_SOURCES=RealPostFeeDriver.cpp
realpostfeeDriver_LDADD=$(top_srcdir)/src/libindex_lib.la -lssl -lpthread -lkslib -lcommdef -lprotobuf

latlngDriver_SOURCES=LatLngDriver.cpp
latlngDriver_LDADD=$(top_srcdir)/src/libindex_lib.la -lssl -lpthread -lkslib -lcommdef -lprotobuf -lm

getDistance_SOURCES=GetDistance.cpp
getDistance_LDADD=$(top_srcdir)/src/libindex_lib.la -lssl -lpthread -lkslib -lcommdef -lprotobuf -lm

cmpTxtIdx_SOURCES=cmpTxtIdx.cpp
cmpTxtIdx_LDADD=-lssl -lpthread -lkslib -lcommdef

EXTRA_DIST = hf_index_cfg.xml
configdir=$(prefix)/etc
config_DATA=$(top_srcdir)/src/app/hf_index_cfg.xml
