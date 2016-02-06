CXX = g++
CPPFLAGS += -I/usr/local/include -pthread
CXXFLAGS += -std=c++11
#LDFLAGS += -L/usr/local/lib -lgrpc++_unsecure -lgrpc -lgpr -lprotobuf -lpthread -ldl
LDFLAGS += -L/usr/local/lib -lgrpc++_unsecure -lgrpc -lgpr -lprotobuf -lpthread -ldl
PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`
PLUGIN_ARG := --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH)

PROTOS_PATH = ../apiprotos/protos
GENSRC_PATH = ../apiprotos/gen-src
PROTOLIB_DIR = ../apiprotos/lib

vpath %.proto $(PROTOS_PATH)

all: AcumioClient.exe
	echo "Generated AcumioClient.exe"

# CLIENT_DEPS = src/AcumioClient.cpp $(PROTOLIB_DIR)/server.pb.o $(PROTOLIB_DIR)/server.grpc.pb.o
CLIENT_DEPS = $(PROTOLIB_DIR)/server.pb.o $(PROTOLIB_DIR)/server.grpc.pb.o obj/AcumioClient.o
AcumioClient.exe: $(CLIENT_DEPS)
	$(CXX) $^ $(LDFLAGS) -o $@

obj/AcumioClient.o: src/AcumioClient.cpp
	echo "Building obj/AcumioClient.o from src/AcumioClient.cpp"
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -I $(GENSRC_PATH) -c -o obj/AcumioClient.o src/AcumioClient.cpp

clean:
	rm -f AcumioClient.exe *.o src/*.o obj/*.o

cleanall:
	rm -f AcumioClient.exe *.o src/*.o obj/*.o
