#!/bin/bash
curPath=$(cd `dirname $0`;pwd)
pushd "$curPath" > /dev/null
#产生protoc文件
if [ ! ./proto/msg.pb.cc -nt ./proto/msg.proto ]
then
    protoc --cpp_out=./ ./proto/msg.proto
fi
if [ -d "build" ]
then
    if [ "$1" = "-f" ]
    then
        rm -rf ./build
    fi
fi
mkdir build
pushd ./build > /dev/null
cmake .. || exit -1
make 
popd > /dev/null
