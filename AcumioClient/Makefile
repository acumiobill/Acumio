CXX = g++
CPPFLAGS += -I/usr/local/include -pthread
CXXFLAGS += -std=c++11
LDFLAGS += -L/usr/local/lib -lgrpc++_unsecure -lgrpc -lgpr -lprotobuf -lpthread -ldl
PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`
PLUGIN_ARG := --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH)

PROTOS_PATH = ../apiprotos/protos
GENCPP_PATH = ../apiprotos/gen-cpp
PROTOLIB_DIR = ../apiprotos/lib

vpath %.proto $(PROTOS_PATH)

all: AcumioCommandLine.exe
	echo "Generated AcumioCommandLine.exe"

CLIENT_DEPS = $(PROTOLIB_DIR)/user.pb.o $(PROTOLIB_DIR)/server.pb.o $(PROTOLIB_DIR)/server.grpc.pb.o obj/AcumioClient.o obj/AcumioCommandLine.o
AcumioCommandLine.exe: $(CLIENT_DEPS)
	$(CXX) $^ $(LDFLAGS) -o $@

obj/AcumioCommandLine.o: src/AcumioCommandLine.cpp src/AcumioClient.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -I $(GENCPP_PATH) -c -o obj/AcumioCommandLine.o src/AcumioCommandLine.cpp

obj/AcumioClient.o: src/AcumioClient.cpp src/AcumioClient.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -I $(GENCPP_PATH) -c -o obj/AcumioClient.o src/AcumioClient.cpp

clean:
	rm -f AcumioClient.exe *.o src/*.o obj/*.o

cleanall:
	rm -f AcumioClient.exe *.o src/*.o obj/*.o
