#!/bin/bash

function bcolorer {
  BIT=$1
  PLUGIN=FarColorer

  cd farcolorer/ || return 1
  rm -fR bin
  cd src || return 1
  wine cmd /c ../../../colorer.${BIT}.bat &> ../../../logs/colorer${BIT}
  cd ..

  mkdir -p ../../outfinalnew${BIT}/Plugins/${PLUGIN}

  cp -f changelog history.ru.txt LICENSE README ../../outfinalnew${BIT}/Plugins/$PLUGIN/

  if [ ! -e bin ]; then
    return 1
  fi

  if [ "$BIT" == "64" ]; then
    cd bin
    mv colorer_x64.dll colorer.dll
    mv colorer_x64.map colorer.map
    cd ..
  fi

  cp -Rf bin ../../outfinalnew${BIT}/Plugins/$PLUGIN/

  cd ../schemes || return 1
  cp -Rf base ../../outfinalnew${BIT}/Plugins/$PLUGIN/
  cd ..
}

mkdir farcolorer
cd farcolorer || exit 1

rm -fR farcolorer
rm -fR colorer
#rm -fR schemes - will not delete, lots of traffic and slow, will just pull updates

( \
	svn co http://svn.code.sf.net/p/colorer/svn/trunk/far3colorer farcolorer && \
	svn co http://svn.code.sf.net/p/colorer/svn/trunk/colorer/src/shared colorer/src/shared && \
	svn co http://svn.code.sf.net/p/colorer/svn/trunk/colorer/src/zlib colorer/src/zlib && \
	svn co http://svn.code.sf.net/p/colorer/svn/trunk/schemes schemes \
) || exit 1

cd schemes || exit 1

chmod +x ./build.sh

#neweset ubuntu ant 1.8.2 has a bug and can't find the resolver, 1.8.4 works fine
PATH=~/apache-ant-1.8.4/bin:$PATH
export PATH
./build.sh farbase.clean
./build.sh farbase &> ../../logs/colorerschemes || exit 1

cd ..

( \
	bcolorer 32 && \
	bcolorer 64 \
) || exit 1

cd ..
