CXX = g++
CPPFLAGS += -pthread -I/usr/local/include
# Non-Debug Version.
#CXXFLAGS += -std=c++11
# Debug version
CXXFLAGS += -std=c++11 -g
# TODO: replace lgrpc++_unsecure with lgrpc++.
LDFLAGS += -L/usr/local/lib -lgrpc++_unsecure -lgrpc -lgpr -lprotobuf -lpthread -ldl
PROTOC := protoc
GRPC_CPP_PLUGIN := grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`
PLUGIN_ARG := --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH)

PROTOS_PATH = protos
GENCPP_PATH = gen-cpp
GENJAVA_PATH = gen-java
OUTLIB_DIR = lib

ROOTLESSPROTOS = $(shell find $(PROTOS_PATH) -type f -name "*.proto" | \
                   sed "s/^$(PROTOS_PATH)\///")
PROTOFILES := $(patsubst %, $(PROTOS_PATH)/%, $(ROOTLESSPROTOS))
GENCPPFILES := $(patsubst %.proto, $(GENCPP_PATH)/%.pb.cc, \
                          $(ROOTLESSPROTOS)) $(GENCPP_PATH)/server.grpc.pb.cc
DOTOFILES := $(patsubst %.proto, $(OUTLIB_DIR)/%.pb.o, \
                        $(ROOTLESSPROTOS)) $(OUTLIB_DIR)/server.grpc.pb.o
DOTODEPS := $(patsubst %.o, %.d, $(DOTOFILES))

all: $(DOTOFILES)
	echo "Done"

debug_dummy:
	echo "ROOTLESSPROTOS ($(ROOTLESSPROTOS))"
	echo "PROTOFILES: ($(PROTOFILES))"
	echo "GENCPPFILES: ($(GENCPPFILES))"
	echo "DOTOFILES: ($(DOTOFILES))"
	echo "DOTODEPS: ($(DOTODEPS))"

$(OUTLIB_DIR)/%.o: $(GENCPP_PATH)/%.cc $(GENCPP_PATH)/generation.done $(OUTLIB_DIR)/depends.done
	$(CXX) -c -o $@ $(CPPFLAGS) -I$(GENCPP_PATH) $(DEFINES) $(CXXFLAGS) $(LDFLAGS) $<

$(GENCPP_PATH)/generation.done: $(GENCPPFILES)
	echo "All code-generation done." > $(GENCPP_PATH)/generation.done

$(OUTLIB_DIR)/depends.done: $(DOTODEPS)
	echo "All dependency generation done." > $(OUTLIB_DIR)/depends.done

$(OUTLIB_DIR)/%.d: $(GENCPP_PATH)/%.cc
	@$(SHELL) -ec '$(CXX) -MM -MG $(CXXFLAGS) $< > $@'

$(GENCPP_PATH)/%.pb.cc: $(PROTOS_PATH)/%.proto
	@$(PROTOC) -I $(PROTOS_PATH) --cpp_out=$(GENCPP_PATH) $<

$(GENCPP_PATH)/server.grpc.pb.cc: $(PROTOS_PATH)/server.proto
	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=$(GENCPP_PATH)/ $(PLUGIN_ARG) $(PROTOS_PATH)/server.proto


clean:
	rm -f $(OUTLIB_DIR)/*.o $(GENCPP_PATH)/*.pb.cc $(GENCPP_PATH)/*.pb.h
	rm -f $(OUTLIB_DIR)/*.d
	rm -f $(GENCPP_PATH)/generation.done
	rm -f $(OUTLIB_DIR)/depends.done
	rm -rf $(GENJAVA_PATH)/com

-include $(DOTODEPS)
