AM_LDFLAGS = -static-libtool-libs
export PROTOC=$(PROTOBUF_ROOT)/bin/protoc
export TOP_DIR=$(top_srcdir)

$(shell $(PROTOC) --cpp_out=./ -I=. ./KSResult.proto)
$(shell cp KSResult.pb.h KSResult.proto $(TOP_DIR)/include/commdef)

.PHONY: clean
clean:
	rm -rf *.pb.* *.o *.lo *.la .libs/
