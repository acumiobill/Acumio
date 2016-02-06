CXX = g++
CPPFLAGS += -I/usr/local/include -pthread
CXXFLAGS += -std=c++11
LDFLAGS += -L/usr/local/lib -lgrpc++_unsecure -lgrpc -lgpr -lprotobuf -lpthread -ldl
PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`
PLUGIN_ARG := --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH)

PROTOS_PATH = ../apiprotos/protos
GENSRC_PATH = ../apiprotos/gen-src
PROTOLIB_DIR = ../apiprotos/lib

vpath %.proto $(PROTOS_PATH)

all: AcumioServer.exe
	echo "Generated AcumioServer.exe"

SERVER_DEPS = $(PROTOLIB_DIR)/server.pb.o $(PROTOLIB_DIR)/server.grpc.pb.o obj/AcumioServer.o
AcumioServer.exe: $(SERVER_DEPS)
	$(CXX) $^ $(LDFLAGS) -o $@

obj/AcumioServer.o: src/AcumioServer.cpp
	echo "Building obj/AcumioServer.o from src/AcumioClient.cpp"
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -I $(GENSRC_PATH) -c -o obj/AcumioServer.o src/AcumioServer.cpp

clean:
	rm -f AcumioServer.exe *.o src/*.o obj/*.o

cleanall:
	rm -f AcumioServer.exe *.o src/*.o obj/*.o
