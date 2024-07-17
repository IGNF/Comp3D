Docker builder for Comp3D
=========================

Build Comp3D form your own Comp3D folder with an old Ubuntu distribution.
AppImage and Windows installer are created.

To get and update Comp3D, use git on your machine.

The docker is to be used interactively to launch compilation.


Install docker if needed
------------------------

    sudo apt install docker.io libfuse2
    sudo usermod -aG docker $USER

Then close and re-open session.


Setup proxy
-----------

If you are behind a proxy:

    mkdir -p ~/.docker
    echo '{"proxies":{"default":{"httpProxy": "http://YOURPROXY:PORT","httpsProxy": "http://YOURPROXY:PORT", "noProxy": "localhost,127.0.0.1"}}}' > ~/.docker/config.json

And set YOURPROXY:PORT in ~/.docker/config.json

It may be necessary to add a systemd configuration as root:

    sudo mkdir /etc/systemd/system/docker.service.d
    echo '[Service]
    Environment="HTTP_PROXY=http://YOURPROXY:PORT/"
    Environment="HTTPS_PROXY=http://YOURPROXY:PORT/"
    Environment="NO_PROXY=localhost,127.0.0.1"' > tmp.conf
    sudo mv tmp.conf /etc/systemd/system/docker.service.d/http-proxy.conf
    sudo systemctl daemon-reload
    sudo systemctl restart docker.service


Build docker image
------------------

From docker directory:

    docker build --network=host -t c3ddocker2004 .

It needs 2 GB of disk space.

The image is based on ubuntu:20.04. In case of old base image (with 404 errors on packages), update with:

    docker pull ubuntu:20.04

Then retry to build.


Run image
---------
For automatic compilation and packaging for Linux and Windows, from docker directory:

    docker run --user "$(id -u):$(id -g)" --rm  -v $(pwd)/..:/c3d c3ddocker2004 /c3d/docker/compile_all.sh
    docker run --rm --device /dev/fuse --privileged -v $(pwd)/..:/c3d c3ddocker2004 /c3d/docker/package_all.sh

The compilation and tests are done with current user, the packaging (via fuse/appimage) needs to be done as root.

Output will be in comp3d/autobuild/, comp3d/autobuild-mxe/, comp3d/comp3d_5.??_linux.tgz and comp3d/comp3d_5.??_win.zip

If something is wrong with files authorizations, try cleaning as docker root :

    docker run --rm --device /dev/fuse --privileged -v $(pwd)/..:/c3d c3ddocker2004 /c3d/docker/clean_all.sh

For interactive compilation:

    docker run --user "$(id -u):$(id -g)" -ti --rm --device /dev/fuse --privileged -v $(pwd)/..:/c3d c3ddocker2004 bash

Look at comp3d/docker/compile_all.sh to get compilation instructions.


Remove docker image
-------------------

    docker rmi c3ddocker2004

