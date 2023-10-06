BD=../build/protobuf
SRC=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp,$(BD)/%.o,$(SRC))

include ../default.mk

all: $(BD) proto $(OBJS)

$(BD):
	mkdir -p $(BD)

$(BD)/%.o: %.cpp
	g++ -c $(FLAGS) --std=c++17 $(INCLUDE) $<
	mv $(patsubst %.cpp,%.o,$<) $(BD)

proto: opencl.pb.cpp

opencl.pb.cpp: opencl.proto
	protoc -I=. --cpp_out=. opencl.proto
	mv opencl.pb.cc opencl.pb.cpp

custom: proto.c custom.c
	gcc proto.c -o custom
	./custom
