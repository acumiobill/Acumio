CXX = g++
CPPFLAGS += -I/usr/local/include -pthread
# The below is for the non-debug version.
CXXFLAGS += -std=c++11 -Wall
# Debug version (comment out next line if you need non-debug).
CXXFLAGS += -std=c++11 -Wall -g
#CXXFLAGS += -std=c++11 -g
# We cannot use the boost libraries until we rebuild the grpc libraries. Why? Because of the change
# in handling of the ABI libraries. Basically, grpc++ and amily were built with pre-gcc.5.1 and boost
# was built with post-gcc.5.1. Currently, I have gcc version 5.2.1.
# LDFLAGS += -L/usr/local/lib -L/usr/lib/x86_64-linux-gnu -lboost_program_options -lgrpc++ -lgrpc -lgpr -lprotobuf -lpthread -ldl
# TODO: Replace -lgrpc++_unsecure with -lgrpc++ when ready to move to ssl
# communication.
LDFLAGS += -L/usr/local/lib -L/usr/lib/x86_64-linux-gnu -lgrpc++_unsecure -lgrpc -lgpr -lprotobuf -lpthread -ldl
PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`
PLUGIN_ARG := --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH)

PROTOS_PATH = ../apiprotos/protos
GENCPP_PATH = ../apiprotos/gen-cpp
PROTOLIB_DIR = ../apiprotos/lib

vpath %.proto $(PROTOS_PATH)

all: AcumioServer.exe
	echo "Done"

SERVER_DEPS = obj/AcumioServer.o obj/UserService.o obj/user_repository.o obj/comparable.o obj/encrypter.o $(PROTOLIB_DIR)/user.pb.o $(PROTOLIB_DIR)/server.pb.o $(PROTOLIB_DIR)/server.grpc.pb.o
AcumioServer.exe: $(SERVER_DEPS)
	$(CXX) $^ $(LDFLAGS) -o $@

obj/AcumioServer.o: src/AcumioServer.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -I $(GENCPP_PATH) $(DEFINES) -c -o obj/AcumioServer.o src/AcumioServer.cpp

obj/UserService.o: src/UserService.cpp src/UserService.h src/user_repository.h src/mem_repository.h src/comparable.h src/pointer_less.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -I $(GENCPP_PATH) $(DEFINES) -c -o obj/UserService.o src/UserService.cpp

obj/user_repository.o: src/user_repository.cpp src/user_repository.h src/mem_repository.h src/comparable.h src/pointer_less.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -I $(GENCPP_PATH) $(DEFINES) -c -o obj/user_repository.o src/user_repository.cpp

obj/comparable.o: src/comparable.cpp src/comparable.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -I $(GENCPP_PATH) $(DEFINES) -c -o obj/comparable.o src/comparable.cpp

obj/encrypter.o: src/encrypter.cpp src/encrypter.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -I $(GENCPP_PATH) $(DEFINES) -c -o obj/encrypter.o src/encrypter.cpp

clean:
	rm -f AcumioServer.exe *.o src/*.o obj/*.o

cleanall:
	rm -f AcumioServer.exe *.o src/*.o obj/*.o
