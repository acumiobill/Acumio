# Author: Bill Province
# Copyright (C) 2016 Acumio
# Makefile for building AcumioServer.exe. The generated executable can be
# run using startserver.sh in the same directory as this Makefile.

CXX = g++
CPPFLAGS += -I/usr/local/include -pthread
# The below is for the non-debug version.
CXXFLAGS += -std=c++11 -Wall
# Debug version (comment out next line if you need non-debug).
CXXFLAGS += -std=c++11 -Wall -g
#CXXFLAGS += -std=c++11 -g
# We cannot use the boost libraries until we rebuild the grpc libraries. Why? Because of the change
# in handling of the ABI libraries. Basically, grpc++ and family were built with pre-gcc.5.1 and boost
# was built with post-gcc.5.1. Currently, I have gcc version 5.2.1.
# LDFLAGS += -L/usr/local/lib -L/usr/lib/x86_64-linux-gnu -lboost_program_options -lgrpc++ -lgrpc -lgpr -lprotobuf -lpthread -ldl
# TODO: Replace -lgrpc++_unsecure with -lgrpc++ when ready to move to ssl
LDFLAGS += -L/usr/local/lib -lgrpc++_unsecure -lgrpc -lgpr -lprotobuf -lpthread -ldl -lboost_program_options
PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`
PLUGIN_ARG := --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH)

PROTOS_PATH := ../apiprotos/protos
GENCPP_PATH := ../apiprotos/gen-cpp
PROTOLIB_DIR := ../apiprotos/lib
PROTOLIB_LIST := $(shell find $(PROTOLIB_DIR) -type f -name "*.o")
SRCFILES := $(shell find src -type f -name "*.cpp")
HDRFILES := $(shell find src -type f -name "*.h")
DOTOFILES := $(patsubst src/%.cpp, obj/%.o, $(SRCFILES))
DEPFILES := $(patsubst obj/%.o, obj/%.d, $(DOTOFILES))

vpath %.proto $(PROTOS_PATH)

all: AcumioServer.exe
	@echo "Done"

SERVER_DEPS = $(DOTOFILES) $(PROTOLIB_LIST)

debug_list:
	@echo "PROTOLIB_LIST: $(PROTOLIB_LIST)"
	@echo "DEPFILES: $(DEPFILES)"

AcumioServer.exe: $(SERVER_DEPS)
	$(CXX) $^ $(LDFLAGS) -o $@

obj/%.d: src/%.cc
	@$(SHELL) -ec '$(CXX) -MM -MG $(CXXFLAGS) $< > $@'

obj/%.o: src/%.cpp
	$(CXX) -c -o $@ $(CXXFLAGS) $(CPPFLAGS) -I $(GENCPP_PATH) $(DEFINES) $<

clean:
	rm -f AcumioServer.exe *.o src/*.o obj/*.o obj/*.d


-include $(DEPFILES)
