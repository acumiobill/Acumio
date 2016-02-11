CXX = g++
CPPFLAGS += -pthread -I/usr/local/include
CXXFLAGS += -std=c++11
LDFLAGS += -L/usr/local/lib -lgrpc++_unsecure -lgrpc++ -lgrpc -lprotobuf -lpthread
PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`
PLUGIN_ARG := --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH)

PROTOS_PATH = protos
GENCPP_PATH = gen-cpp
GENJAVA_PATH = gen-java
OUTLIB_DIR = lib

vpath %.proto $(PROTOS_PATH)

all: $(OUTLIB_DIR)/server.pb.o $(OUTLIB_DIR)/server.grpc.pb.o $(OUTLIB_DIR)/user.pb.o
	echo "Generated user.pb.o, server.pb.o and server.grpc.pb.o"

$(OUTLIB_DIR)/server.pb.o: $(GENCPP_PATH)/server.pb.cc $(GENCPP_PATH)/user.pb.cc
	$(CXX) -c -o $(OUTLIB_DIR)/server.pb.o $(CPPFLAGS) -I$(GENCPP_PATH) $(CXXFLAGS) $(LDFLAGS) $(GENCPP_PATH)/server.pb.cc

$(OUTLIB_DIR)/user.pb.o: $(GENCPP_PATH)/user.pb.cc
	$(CXX) -c -o $(OUTLIB_DIR)/user.pb.o $(CPPFLAGS) -I$(GENCPP_PATH) $(CXXFLAGS) $(LDFLAGS) $(GENCPP_PATH)/user.pb.cc

$(OUTLIB_DIR)/server.grpc.pb.o: $(GENCPP_PATH)/server.grpc.pb.cc $(GENCPP_PATH)/user.pb.cc
	$(CXX) -c -o $(OUTLIB_DIR)/server.grpc.pb.o $(CPPFLAGS) -I$(GENCPP_PATH) $(CXXFLAGS) $(LDFLAGS) $(GENCPP_PATH)/server.grpc.pb.cc

$(GENCPP_PATH)/server.grpc.pb.cc: $(PROTOS_PATH)/server.proto
	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=$(GENCPP_PATH)/ $(PLUGIN_ARG) $(PROTOS_PATH)/server.proto

$(GENCPP_PATH)/server.pb.cc: $(PROTOS_PATH)/server.proto
	$(PROTOC) -I $(PROTOS_PATH) --cpp_out=$(GENCPP_PATH) $(PROTOS_PATH)/server.proto

$(GENCPP_PATH)/user.pb.cc: $(PROTOS_PATH)/user.proto
	$(PROTOC) -I $(PROTOS_PATH) --cpp_out=$(GENCPP_PATH) $(PROTOS_PATH)/user.proto

clean:
	rm -f $(OUTLIB_DIR)/*.o

cleanall:
	rm -f $(OUTLIB_DIR)/*.o $(GENCPP_PATH)/*.pb.cc $(GENCPP_PATH)/*.pb.h
