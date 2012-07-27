AM_LDFLAGS = -static-libtool-libs
export PROTOC=$(PROTOBUF_ROOT)/bin/protoc
export TOP_DIR=$(top_srcdir)

$(shell $(PROTOC) --cpp_out=./ -I=. ./DocIndexMessage.proto)
$(shell $(PROTOC) --cpp_out=./ -I=. ./DocMessage.proto)
$(shell cp DocIndexMessage.pb.h DocMessage.pb.h DocIndexMessage.proto DocMessage.proto $(TOP_DIR)/include/commdef)

.PHONY: clean
clean:
	rm -rf *.pb.* *.o *.lo *.la .libs/
