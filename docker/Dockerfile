FROM ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive

#set locale
RUN apt-get update && apt-get install -y locales
RUN sed -i -e 's/# en_US.UTF-8 UTF-8/en_US.UTF-8 UTF-8/' /etc/locale.gen && \
    dpkg-reconfigure --frontend=noninteractive locales && \
    update-locale LANG=en_US.UTF-8
ENV LANG en_US.UTF-8

RUN apt-get update && apt-get install -y --fix-missing \
    wget unzip build-essential cmake libboost-all-dev qttools5-dev-tools qttools5-dev qtbase5-dev git \
    doxygen sqlite libsqlite3-dev fuse ccache libtiff-dev libcurl4-openssl-dev libeigen3-dev


#get proj
RUN cd / && \
    wget https://download.osgeo.org/proj/proj-9.3.1.tar.gz  && \
    tar -xf proj-9.3.1.tar.gz  && \
    cd proj-9.3.1  && \
    mkdir build  && \
    cd build  && \
    cmake .. -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=OFF -DBUILD_PROJSYNC=OFF  -DENABLE_CURL=OFF -DENABLE_TIFF=OFF -DCMAKE_INSTALL_PREFIX=/usr/local/proj93  && \
    make -j5 && \
    make install

#get proj data
RUN cd / && \
    wget https://download.osgeo.org/proj/proj-datumgrid-1.8.zip && \
    unzip proj-datumgrid-1.8.zip -d proj-data && \
    mkdir -p /usr/local/proj93/share/proj/ && \
    cp proj-data/* /usr/local/proj93/share/proj/

#add mxe
RUN apt-get -y --fix-missing  install software-properties-common curl && \
    curl -sSL 'http://keyserver.ubuntu.com/pks/lookup?op=get&search=0x86B72ED9' | apt-key add - && \
    add-apt-repository 'deb [arch=amd64] http://mirror.mxe.cc/repos/apt bionic main' && \
    apt-get update

RUN apt-get install -y --fix-missing mxe-x86-64-w64-mingw32.static-qt5 \
    mxe-x86-64-w64-mingw32.static-boost mxe-x86-64-w64-mingw32.static-eigen mxe-x86-64-w64-mingw32.static-sqlite

#  mxe: enable ccache for windows with qmake (it's possible since we're compiling on unix for windows)
RUN cp /usr/lib/mxe/usr/x86_64-w64-mingw32.static/qt5/mkspecs/features/unix/ccache.prf /usr/lib/mxe/usr/x86_64-w64-mingw32.static/qt5/mkspecs/features/win32/

#proj for mxe
RUN cd /proj-9.3.1 && \
    export PATH=/usr/lib/mxe/usr/bin/:$PATH && \
    mkdir build-mxe  && \
    cd build-mxe  && \
    x86_64-w64-mingw32.static-cmake .. \
        -DBUILD_TESTING=OFF \
        -DBUILD_SHARED_LIBS=OFF \
        -DBUILD_PROJSYNC=OFF \
        -DENABLE_CURL=OFF \
        -DENABLE_TIFF=OFF \
        -DCMAKE_INSTALL_PREFIX=/usr/local/mxe/proj93 \
        -DCMAKE_CXX_STANDARD=11 \
        -DCMAKE_CXX_EXTENSIONS=ON \
        -DCMAKE_C_COMPILER=x86_64-w64-mingw32.static-gcc \
        -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32.static-g++ \
        -DCMAKE_FIND_ROOT_PATH=/usr/lib/mxe/usr/x86_64-w64-mingw32.static  && \
    make clean  && \
    make -j5  && \
    make install && \
    cp /proj-data/* /usr/local/mxe/proj93/share/proj/

#get appimage
RUN cd / && \
    wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage && \
    wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage && \
    chmod a+x *.AppImage

#get wine
RUN dpkg --add-architecture i386 && \
    apt-get update && \
    apt-get install --no-install-recommends --assume-yes wine-stable

#get pip3
RUN apt-get install -y --fix-missing python3-pip

#get sphinx
RUN pip3 install sphinx sphinx-intl sphinx-mathjax-offline
RUN DEBIAN_FRONTEND=noninteractive apt-get -y --fix-missing  install python3-stemmer qttranslations5-l10n libjs-mathjax

#set rights for created files
RUN echo "umask 0000" >> /root/.bashrc

#set /c3d as a git safe directory, to be used with docker user
RUN git config --global --add safe.directory /c3d

#access local X11 server
ENV DISPLAY :0
