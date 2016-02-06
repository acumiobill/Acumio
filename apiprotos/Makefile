CXX = g++
CPPFLAGS += -pthread -I/usr/local/include
CXXFLAGS += -std=c++11
#LDFLAGS += -L/usr/local/lib -lgrpc++_unsecure -lgrpc -lgpr -lprotobuf -lpthread -ldl
LDFLAGS += -L/usr/local/lib -lgrpc++_unsecure -lgrpc++ -lgrpc -lprotobuf -lpthread
PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`
PLUGIN_ARG := --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH)

PROTOS_PATH = protos
GENSRC_PATH = gen-src
OUTLIB_DIR = lib

vpath %.proto $(PROTOS_PATH)

all: $(OUTLIB_DIR)/server.pb.o $(OUTLIB_DIR)/server.grpc.pb.o
	echo "Generated server.pb.o and server.grpc.pb.o"

$(OUTLIB_DIR)/server.pb.o: $(GENSRC_PATH)/server.pb.cc
	$(CXX) -c -o $(OUTLIB_DIR)/server.pb.o $(CPPFLAGS) -I$(GENSRC_PATH) $(CXXFLAGS) $(LDFLAGS) $(GENSRC_PATH)/server.pb.cc

$(OUTLIB_DIR)/server.grpc.pb.o: $(GENSRC_PATH)/server.grpc.pb.cc
	$(CXX) -c -o $(OUTLIB_DIR)/server.grpc.pb.o $(CPPFLAGS) -I$(GENSRC_PATH) $(CXXFLAGS) $(LDFLAGS) $(GENSRC_PATH)/server.grpc.pb.cc

$(GENSRC_PATH)/server.grpc.pb.cc: $(PROTOS_PATH)/server.proto
	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=$(GENSRC_PATH)/ $(PLUGIN_ARG) $(PROTOS_PATH)/server.proto

$(GENSRC_PATH)/server.pb.cc: $(PROTOS_PATH)/server.proto
	$(PROTOC) -I $(PROTOS_PATH) --cpp_out=$(GENSRC_PATH) $(PROTOS_PATH)/server.proto

clean:
	rm -f $(OUTLIB_DIR)/*.o

cleanall:
	rm -f $(OUTLIB_DIR)/*.o $(GENSRC_PATH)/*.pb.cc $(GENSRC_PATH)/*.pb.h
