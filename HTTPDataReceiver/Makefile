
.PHONY: all

all: HTTPDataResender

PROTO_FILES=HTTPMessage.proto
PROTO_INCLUDE_FOLDER=../ProtobufMessages/Out/cpp
CXX=c++
CXX_FLAGS=-std=c++11

CXX_FLAGS+=$(shell pkg-config --cflags --libs protobuf) #add protobuf libs to flags

INCLUDES=-I/opt/homebrew/include

all: HTTPDataResender

run: 
	./HTTPDataResender

HTTPDataResender: main.cpp
	pkg-config --cflags protobuf  # fails if protobuf is not installed
	$(CXX) $(CXX_FLAGS) $(INCLUDES) $< $(PROTO_INCLUDE_FOLDER)/HTTPMessage.pb.cc -o $@ 

protoc_compile: ../ProtobufMessages/Out/cpp/HTTPMessage.pb.cc
	cd ../ProtobufMessages/Src && protoc --cpp_out=../Out/cpp $(PROTO_FILES) && cd ../../HTTPDataReceiver