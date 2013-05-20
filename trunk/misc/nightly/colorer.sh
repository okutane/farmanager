#!/bin/bash

function bcolorer {
  BIT=$1
  PLATFORM=$2
  PLUGIN=FarColorer

  pushd build/Release/${PLATFORM} || return 1

  rm -fR CMakeFiles
  rm *.dll
  rm *.lib

  wine cmd /c ../../../../colorer.${BIT}.bat &> ../../../../logs/colorer${BIT}

  if [ ! -e colorer.dll ]; then
    return 1
  fi

  mkdir -p ../../../../outfinalnew${BIT}/Plugins/${PLUGIN}/bin

  cp -f colorer.dll colorer.map ../../../../outfinalnew${BIT}/Plugins/$PLUGIN/bin

  popd

  cp -f docs/history.ru.txt LICENSE README.MD ../outfinalnew${BIT}/Plugins/$PLUGIN/
  cp -f misc/* ../outfinalnew${BIT}/Plugins/$PLUGIN/bin

  pushd ../Colorer-schemes || return 1
  cp -Rf base ../outfinalnew${BIT}/Plugins/$PLUGIN/
  popd
}

#git clone must already exist
cd Colorer-schemes || exit 1
git pull || exit 1

chmod +x ./build.sh

#neweset ubuntu ant 1.8.2 has a bug and can't find the resolver, 1.8.4 works fine
PATH=~/apache-ant-1.8.4/bin:$PATH
export PATH
./build.sh farbase.clean
./build.sh farbase &> ../logs/colorerschemes || exit 1

cd ..

#git clone must already exist
#all build dirs with cmake cache files must also exist as cmake gets stuck under wine on first run without cache files
cd FarColorer || exit 1
git pull || exit 1
git submodule update || exit 1

( \
	bcolorer 32 x86 && \
	bcolorer 64 x64 \
) || exit 1

cd ..
