#!/usr/bin/env bash

sudo add-apt-repository ppa:beineri/opt-qt-5.15.2-focal -y
sudo apt-get update -qq
sudo apt-get install -qq qt515base qt515connectivity qt515multimedia qt515svg qt515translations qt515tools qt515webchannel qt515webengine mesa-common-dev libgl-dev
sudo apt-get install -qq libglu1-mesa-dev freeglut3-dev mesa-common-dev
source /opt/qt515/bin/qt515-env.sh
export QTHOME=/opt/qt515
export PATH=/opt/cmake/bin:/opt/qt515/bin:${PATH}
export CMAKE_PREFIX_PATH=/opt/qt515:${CMAKE_PREFIX_PATH}
mkdir -p $GITHUB_WORKSPACE/build
