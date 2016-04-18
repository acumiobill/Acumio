# Author: Bill Province (bill@acumio.com)
# Copyright (C) 2016 Acumio
# This Makefile builds AcumioCommandLineClient.exe. You can invoke
# AcumioCommandLineClient.exe from runclient.sh in this same directory.

CXX = g++
CPPFLAGS += -I/usr/local/include -pthread
CPPTSTFLAGS += $(CPPFLAGS)
CPPTSTFLAGS += -I/usr/include
CPPTSTFLAGS += -Isrc
# Non-Debug Version.
CXXFLAGS += -std=c++11 -Wall
# Debug Version. (Comment out next line if you need non-debug)
CXXFLAGS += -g
# TODO: User lgrpc++ instead of lgrpc++_unsecure when ready to move to ssl.
LDFLAGS += -L/usr/local/lib -lgrpc++_unsecure -lgrpc -lgpr -lprotobuf -lpthread -ldl -lboost_program_options
LDTSTFLAGS = -L/usr/lib -lgmock -lgmock_main
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
DOTOMINUSMAIN := $(filter-out obj/AcumioCommandLine.o, $(DOTOFILES))
DOTOTSTFILES := $(patsubst src/test/%.cpp, test_obj/%.o, $(TSTFILES))
DOTOTSTMINUSMAIN := $(filter-out test_obj/test%.o, $(DOTOTSTFILES))
DEPFILES := $(patsubst obj/%.o, obj/%.d, $(DOTOFILES))
TSTDEPFILES := $(patsubst test_obj/%.o, test_obj/%.d, $(DOTOTSTFILES))
TESTSNEEDINGEXE := $(filter test_obj/test%.o, $(DOTOTSTFILES))
TSTEXEFILES := $(patsubst test_obj/%.o, test_obj/%.exe, $(TESTSNEEDINGEXE))
# Building this sed command, we need to refer to the location where the
# proto-lib headers are located. However, since their path contains the "/"
# character - which is a special character in sed, we need to quote it.
# This is a bit messy in Makefiles, so I just hard-code the path here.
# Be sure to update this if we change GENCPP_PATH.
SEDCMD := sed "s/\(.*\)\.o:/obj\/\1\.o:/;
SEDCMD += s/\([a-z_\.]*\)\.pb\.h/..\/apiprotos\/gen-cpp\/\1\.pb\.h/g"

TSTSED := sed "s/\(.*\)\.o:/test_obj\/\1\.o:/;
TSTSED += s/\([a-zA-Z_\/]*[^\.]..\)\.h/src\/\1\.h/g;
TSTSED += s/src\/src\//src\//g;
TSTSED += s/\([a-z_\.]*\)\.pb\.h/..\/apiprotos\/gen-cpp\/\1\.pb\.h/g"

vpath %.proto $(PROTOS_PATH)

all: AcumioCommandLine.exe
	@echo "Done"

all_tests: $(TSTEXEFILES)
	@echo "Done building test executables"

CLIENT_DEPS = $(DOTOFILES) $(PROTOLIB_LIST)
TEST_DEPS = $(DOTOMINUSMAIN) $(DOTOTSTMINUSMAIN) $(PROTOLIB_LIST)

debug_list:
	@echo "PROTOLIB_LIST: $(PROTOLIB_LIST)"
	@echo "SRCFILES: $(SRCFILES)"
	@echo "DOTOFILES: $(DOTOFILES)"
	@echo "DEPFILES: $(DEPFILES)"
	@echo "TEST_DEPS: $(TEST_DEPS)"
	@echo "TESTSNEEDINGEXE: $(TESTSNEEDINGEXE)"
	@echo "TSTEXEFILES := $(TSTEXEFILES)"

AcumioCommandLine.exe: $(CLIENT_DEPS)
	$(CXX) $^ $(LDFLAGS) -o $@

obj/%.d: src/%.cpp
	@$(SHELL) -ec 'mkdir -p obj'
	@$(SHELL) -ec '$(CXX) -MM -MG $(CXXFLAGS) $< | $(SEDCMD) > $@'

test_obj/%.d: src/test/%.cpp
	@$(SHELL) -ec 'mkdir -p test_obj'
	@$(SHELL) -ec '$(CXX) -MM -MG $(CXXFLAGS) $< | $(TSTSED) > $@'

obj/%.o: src/%.cpp
	@$(SHELL) -ec 'mkdir -p obj'
	$(CXX) -c -o $@ $(CXXFLAGS) $(CPPFLAGS) -I $(GENCPP_PATH) $(DEFINES) $<

test_obj/%.o: src/test/%.cpp
	@$(SHELL) -ec 'mkdir -p test_obj'
	$(CXX) -c -o $@ $(CXXFLAGS) $(CPPTSTFLAGS) -I $(GENCPP_PATH) $(DEFINES) $<

test_obj/%.exe: test_obj/%.o $(TEST_DEPS)
	$(SHELL) -ec 'mkdir -p test_obj'
	$(CXX) $^ $(LDTSTFLAGS) -o $@

clean:
	rm -f AcumioCommandLine.exe *.o src/*.o obj/*.o obj/*.d test_obj/*.d test_obj/*.exe

-include $(DEPFILES)
-include $(TSTDEPFILES)
