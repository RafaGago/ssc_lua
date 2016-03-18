#!/bin/sh

cwd=$(pwd)
ssc=$cwd/ssc/
bl=$ssc/dependencies/base_library

cd $cwd/code
tar -xvzf LuaJIT-2.0.4.tar.gz
cd LuaJIT-2.0.4
make -j4
make install PREFIX=$(pwd)/stage

cd $ssc/dependencies
./prepare_dependencies.sh

cd $ssc/build/premake
./premake5 gmake
make -C ../linux 
make -C ../linux config=debug

cd $cwd/../build/premake
ln -sf $bl/build/premake/premake5

