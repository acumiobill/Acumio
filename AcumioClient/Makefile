CXX = g++
CPPFLAGS += -I/usr/local/include -pthread
# Non-Debug Version.
CXXFLAGS += -std=c++11 -Wall
# Debug Version.
CXXFLAGS += -std=c++11 -Wall -g
#DEFINES += -D_GLIBCXX_USE_CXX11_ABI=0
# TODO: User lgrpc++ instead of lgrpc++_unsecure
#LDFLAGS += -L/usr/local/lib -lgrpc++_unsecure -lgrpc -lgpr -lprotobuf -lpthread -ldl
LDFLAGS += -L/usr/local/lib -lgrpc++_unsecure -lgrpc -lprotobuf -lpthread -ldl
PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`
PLUGIN_ARG := --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH)

PROTOS_PATH = ../apiprotos/protos
GENCPP_PATH = ../apiprotos/gen-cpp
PROTOLIB_DIR = ../apiprotos/lib
PROTOLIB_LIST = $(shell find $(PROTOLIB_DIR) -type f -name "*.o")

vpath %.proto $(PROTOS_PATH)

all: AcumioCommandLine.exe
	echo "Generated AcumioCommandLine.exe"

CLIENT_DEPS = obj/AcumioClient.o obj/AcumioCommandLine.o $(PROTOLIB_LIST)
AcumioCommandLine.exe: $(CLIENT_DEPS)
	$(CXX) $^ $(LDFLAGS) -o $@

obj/AcumioCommandLine.o: src/AcumioCommandLine.cpp src/AcumioClient.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -I $(GENCPP_PATH) $(DEFINES) -c -o obj/AcumioCommandLine.o src/AcumioCommandLine.cpp

obj/AcumioClient.o: src/AcumioClient.cpp src/AcumioClient.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -I $(GENCPP_PATH) $(DEFINES) -c -o obj/AcumioClient.o src/AcumioClient.cpp

clean:
	rm -f AcumioClient.exe *.o src/*.o obj/*.o

cleanall:
	rm -f AcumioClient.exe *.o src/*.o obj/*.o
