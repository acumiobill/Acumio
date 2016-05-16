# Author: Bill Province (bill@acumio.com)
# Copyright (C) 2016 Acumio
# Makefile for building AcumioServer.exe. The generated executable can be
# run using startserver.sh in the same directory as this Makefile.

CXX = g++
CPPFLAGS += -I/usr/local/include -pthread
CPPTSTFLAGS += $(CPPFLAGS)
CPPTSTFLAGS += -I/usr/include
CPPTSTFLAGS += -Isrc
# The below is for the non-debug version.
CXXFLAGS += -std=c++11 -Wall
# Debug version (comment out next line if you need non-debug).
CXXFLAGS += -g
# TODO: Replace -lgrpc++_unsecure with -lgrpc++ when ready to move to ssl
LDFLAGS += -L/usr/local/lib -lgrpc++_unsecure -lgrpc -lgpr -lprotobuf -lpthread -ldl -lboost_program_options -latomic
LDTSTFLAGS = -L/usr/lib -lgmock -lgmock_main -lmcheck
LDTSTFLAGS += $(LDFLAGS)
PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`
PLUGIN_ARG := --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH)

PROTOS_PATH := ../apiprotos/protos
GENCPP_PATH := ../apiprotos/gen-cpp
PROTOLIB_DIR := ../apiprotos/lib
PROTOLIB_LIST := $(shell find $(PROTOLIB_DIR) -type f -name "*.o")
SRCFILES := $(shell find src -type f -name "*.cpp" | grep -v "src/test/")
TSTFILES := $(shell find src/test -type f -name "*.cpp")
HDRFILES := $(shell find src -type f -name "*.h")
DOTOFILES := $(patsubst src/%.cpp, obj/%.o, $(SRCFILES))
DOTOMINUSMAIN := $(filter-out obj/AcumioServer.o, $(DOTOFILES))
DOTOTSTFILES := $(patsubst src/test/%.cpp, test_obj/%.o, $(TSTFILES))
DEPFILES := $(patsubst obj/%.o, obj/%.d, $(DOTOFILES))
TSTDEPFILES := $(patsubst test_obj/%.o, test_obj/%.d, $(DOTOTSTFILES))
TSTEXEFILES := $(patsubst test_obj/%.o, test_obj/%.exe, $(DOTOTSTFILES))
TSTRUNFILES := $(patsubst test_obj/%.exe, test_run/%.run, $(TSTEXEFILES))
VERIFIEDRUNS := $(patsubst test_run/%.run, test_run/%.verify, $(TSTRUNFILES))
# Building this sed command, we need to refer to the location where the
# proto-lib headers are located. However, since their path contains the "/"
# character - which is a special character in sed, we need to quote it.
# This is a bit messy in Makefiles, so I just hard-code the path here.
# Be sure to update this if we change GENCPP_PATH.

# Step 1 of sed: replace "mytarget.o:" with "obj/mytarget.o".
# Step 2 (second line):
# replace "foo.pb.h" with "../apiprotos/gen-cpp/foo.pb.h" also:
#   "server.grpc.pb.h" with "../apiprotos/gen-cpp/server.grpc.pb.h"
SEDCMD := sed "s/\(.*\)\.o:/obj\/\1\.o:/;
SEDCMD += s/\([a-z_\.]*\)\.pb\.h/..\/apiprotos\/gen-cpp\/\1\.pb\.h/g"

# Step 1 of sed: replace "mytarget.o:" with "test_obj/mytarget.o:"
#   second line between next two (potentially)
TSTSED := sed "s/\(.*\)\.o:/test_obj\/\1\.o:/;
TSTSED += s/\([a-zA-Z_\/]*[^\.]..\)\.h/src\/\1\.h/g;
TSTSED += s/\([a-z_\.]*\)\.pb\.h/..\/apiprotos\/gen-cpp\/\1\.pb\.h/g;
TSTSED += s/src\/src/src/g"

vpath %.proto $(PROTOS_PATH)

all: all_verified_tests AcumioServer.exe
	@echo "All Make Targets Complete"

all_verified_tests: $(VERIFIEDRUNS)
	@echo "Tests Verified"

all_test_runs: $(TSTRUNFILES)
	@echo "Tests Run"

all_tests: $(TSTEXEFILES)
	@echo "Tests Built."

SERVER_DEPS = $(DOTOFILES) $(PROTOLIB_LIST)
TEST_DEPS = $(DOTOMINUSMAIN) $(PROTOLIB_LIST)

debug_list:
	@echo "PROTOLIB_LIST: $(PROTOLIB_LIST)"
	@echo "SRCFILES: $(SRCFILES)"
	@echo "DOTOFILES: $(DOTOFILES)"
	@echo "DEPFILES: $(DEPFILES)"
	@echo "TEST_DEPS: $(TEST_DEPS)"
	@echo "VERIFIEDRUNS: $(VERIFIEDRUNS)"

AcumioServer.exe: $(SERVER_DEPS)
	@echo "Linking $@"
	@$(CXX) $^ $(LDFLAGS) -o $@

obj/%.d: src/%.cpp
	@$(SHELL) -ec 'mkdir -p obj'
	@$(SHELL) -ec '$(CXX) -MM -MG $(CXXFLAGS) $< | $(SEDCMD) > $@'

test_obj/%.d: src/test/%.cpp
	@$(SHELL) -ec 'mkdir -p test_obj'
	@$(SHELL) -ec '$(CXX) -MM -MG $(CXXFLAGS) $< | $(TSTSED) > $@'

obj/%.o: src/%.cpp
	@echo "Compiling $<"
	@$(SHELL) -ec 'mkdir -p obj'
	@$(CXX) -c -o $@ $(CXXFLAGS) $(CPPFLAGS) -I $(GENCPP_PATH) $(DEFINES) $<

test_obj/%.o: src/test/%.cpp
	@echo "Compiling $<"
	@$(SHELL) -ec 'mkdir -p test_obj'
	@$(CXX) -c -o $@ $(CXXFLAGS) $(CPPTSTFLAGS) -I $(GENCPP_PATH) $(DEFINES) $<

test_obj/%.exe: test_obj/%.o $(TEST_DEPS)
	@echo "Linking $@"
	@$(SHELL) -ec 'mkdir -p test_obj'
	@$(CXX) $^ $(LDTSTFLAGS) -o $@

test_run/%.run: test_obj/%.exe
	@echo "running test: $<"
	@$(SHELL) -ec 'mkdir -p test_run'
	@$(SHELL) -ec '$< > $@'

test_run/%.verify: test_run/%.run
	@echo "verifying test: $<"
	@$(SHELL) -ec './testchecker.sh $< $@'

clean:
	rm -f AcumioServer.exe *.o src/*.o obj/*.o obj/*.d test_obj/*.d test_obj/*.o test_obj/*.exe test_run/*.run test_run/*.verify

-include $(DEPFILES)
-include $(TSTDEPFILES)
