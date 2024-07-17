.. _compilation:

===========
Compilation
===========

Setup
=====

Comp3D depends on the libraries:

  - Boost
  - Eigen v3
  - proj 9.3
  - Qt 5 (optional)

The main tools used for its compilation are:

  - a c++17 compiler (gcc or  vc++)
  - cmake


.. _setuplinux:

GNU/Linux
---------

All the GNU/Linux commands are described for **Ubuntu 22.04**.

Packages
~~~~~~~~

.. code-block:: bash

    sudo apt install cmake libboost-all-dev qttools5-dev-tools qtbase5-dev \
                     libeigen3-dev libclang-common-11-dev qttools5-dev

Proj
~~~~

LibProj version is fixed to 9.3, installed into ``/usr/local/proj93/``.
 
Download grids:

.. code-block:: bash

    wget https://download.osgeo.org/proj/proj-datumgrid-1.8.zip
    unzip proj-datumgrid-1.8.zip -d proj-data

.. code-block:: bash

    sudo mkdir -p /usr/local/proj93/share/proj/
    sudo cp proj-data/* /usr/local/proj93/share/proj/
  
Compile proj-9.3 :

.. code-block:: bash

    sudo apt install sqlite libsqlite3-dev

.. code-block:: bash

    wget https://download.osgeo.org/proj/proj-9.3.1.tar.gz
    tar -xf proj-9.3.1.tar.gz
    cd proj-9.3.1
    mkdir build
    cd build
    cmake .. -DBUILD_SHARED_LIBS=OFF -DBUILD_PROJSYNC=OFF  -DENABLE_CURL=OFF -DENABLE_TIFF=OFF -DBUILD_TESTING=OFF -DCMAKE_INSTALL_PREFIX=/usr/local/proj93
    make

.. code-block:: bash

    sudo make install



Configure Comp3D
~~~~~~~~~~~~~~~~
.. code-block:: bash

    git clone https://github.com/IGNF/Comp3D
    cd Comp3D
    mkdir build
    cd build
    cmake ..


Configure tests
~~~~~~~~~~~~~~~
.. code-block:: bash

    cd Comp3D
    cd tests
    mkdir build
    cd build
    cmake ..


.. _setupwin:

Windows
-------

 - install *git* (https://git-scm.com/download/win)
 - install *msvc++* community edition (https://learn.microsoft.com/en-us/visualstudio/install/install-visual-studio, https://visualstudio.microsoft.com/en/downloads/)
 - install *cmake* (https://cmake.org/download/), add it to system *PATH*

vcpkg
~~~~~

Choose a directory for vcpkg (here marked \*VCPKG_DIR\*)

.. code-block:: bash

    cd *VCPKG_DIR*
    git clone https://github.com/microsoft/vcpkg.git
    cd vcpkg
    bootstrap-vcpkg.bat

Configure Comp3D
~~~~~~~~~~~~~~~~
.. code-block:: bash

    git clone https://github.com/IGNF/Comp3D
    cd Comp3D
    mkdir build
    cd build
    cmake .. -DCMAKE_TOOLCHAIN_FILE=*VCPKG_DIR*/vcpkg/scripts/buildsystems/vcpkg.cmake


Configure tests
~~~~~~~~~~~~~~~
.. code-block:: bash

    cd Comp3D
    cd tests
    mkdir build
    cd build
    cmake .. -DCMAKE_TOOLCHAIN_FILE=*VCPKG_DIR*/vcpkg/scripts/buildsystems/vcpkg.cmake



.. _compile:

Compilation
===========

GNU/Linux
---------

Comp3D (in ``Comp3D/build/`` directory):

.. code-block:: bash

    make -j4


Tests (in ``Comp3D/tests/build/ directory``):

.. code-block:: bash

    make -j4

Then run the tests from ``Comp3D/tests/`` directory:

.. code-block:: bash

    ./build/Comp3D_tests


Windows
-------

Comp3D (in ``Comp3D/build/`` directory):

.. code-block:: bash

    cmake --build . --config Release


Tests (in ``Comp3D/tests/build/ directory``):

.. code-block:: bash

    cmake --build . --config Release

Then run the tests from ``Comp3D/tests/`` directory:

.. code-block:: bash

    ./build/Comp3D_tests.exe



.. _compileauto:

Automatic compilation/cross-compilation/tests/packaging
=======================================================

All these actions are done with the command:

.. code-block:: bash

    ./run_compile-test-package_docker.sh

This command will:

  - setup the compilation docker image if needed
  - compile user documentation
  - compile |c3| for GNU/Linux
  - compile and run tests
  - cross-compile for Windows
  - cross-compile and run tests
  - packages (appimage, deb, zip)

See ``docker/README.md`` for more information and check the docker scripts to learn more about cross-complation and packaging.


Build options
=============

The software project is defined by *Comp3D_cpp.pro*.
It is possible to change the compilation options on the ``DEFINES +=`` lines of this file:

  - ``USE_QT``: to use Qt, mandatory in order to have messages translation
  - ``USE_RES``: depends on ``USE_QT``, to be able to create report resources directory *res/* (see :ref:`out-report`)
  - ``USE_GUI``: depends on ``USE_QT``, to enable the GUI
  - ``USE_AUTO``: to enable the automatic mode (see :ref:`automatization`)
  - ``ADD_PROJ_CC``, ``ADD_PROJ_NTF``, ``ADD_PROJ_UTM``: to fill the pre-recorded projections list


.. _compiledoc:

Documentation generation
========================


User documentation
------------------

The user documentation is in *doc_uni/*. It is written in *reStructuredText* and compiled by *sphinx*.

Linux dependencies:

.. code-block:: bash

    sudo apt install python3-stemmer qttranslations5-l10n

On any OS:

.. code-block:: bash

    pip3 install -U sphinx sphinx-mathjax-offline sphinx_intl


To generate the html pages for every supported language:

On Linux:

.. code-block:: bash

    ./build_doc.sh

On Windows:

.. code-block:: bash

    build_doc.bat

|c3| must then be compiled to take into account the new user documentation.


Math documentation
------------------

The math documentation is in *doc_math/*, written in LaTeX.

Linux dependencies:

.. code-block:: bash

    sudo apt install texlive-latex-extra texlive-lang-french

To generate de pdf file:

.. code-block:: bash

    make


