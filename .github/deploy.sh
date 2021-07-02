#!/usr/bin/env bash

cd $GITHUB_WORKSPACE
export VERSION=$(git rev-list master --count)
cd $GITHUB_WORKSPACE/build
export GITHUB_TOKEN=$1
alias qmake=qmake-qt5
export PATH=/opt/cmake/bin:/opt/qt515/bin:${PATH}
wget -c -nv "https://github.com/probonopd/linuxdeployqt/releases/download/5/linuxdeployqt-5-x86_64.AppImage"
chmod a+x linuxdeployqt-5-x86_64.AppImage
./linuxdeployqt-5-x86_64.AppImage appdir/usr/share/applications/*.desktop -appimage -exclude-libs="libnss3.so,libnssutil3.so"
find appdir -executable -type f -exec ldd {} \; | grep " => /usr" | cut -d " " -f 2-3 | sort | uniq
wget -c https://github.com/probonopd/uploadtool/raw/master/upload.sh
ls -al .
bash upload.sh Viper*.AppImage*
