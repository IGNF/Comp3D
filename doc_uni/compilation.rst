.. _compilation:

===========
Compilation
===========

Dependencies
==============

The libraries to install are:

  - Boost
  - Eigen v3
  - proj 8.2
  - Qt 5 (optional)


.. _compilelinux:

GNU/Linux
---------

All the GNU/Linux commands are described for **Ubuntu 22.04**.

Packages
~~~~~~~~

.. code-block:: bash

    sudo apt install libboost-all-dev qttools5-dev-tools qt5-qmake qtbase5-dev \
                     libeigen3-dev proj-data libclang-common-11-dev

Proj
~~~~

LibProj version is fixed to 8.2, installed into ``/usr/local/proj82/``.
 
Download grids:

.. code-block:: bash

    wget https://download.osgeo.org/proj/proj-datumgrid-1.8.zip
    unzip proj-datumgrid-1.8.zip -d proj-data

.. code-block:: bash

    sudo mkdir -p /usr/local/proj82/share/proj/
    sudo cp proj-data/* /usr/local/proj82/share/proj/
  
Compile proj-8.2 :

.. code-block:: bash

    sudo apt install sqlite libsqlite3-dev

.. code-block:: bash

    wget https://download.osgeo.org/proj/proj-8.2.1.tar.gz
    tar -xf proj-8.2.1.tar.gz
    cd proj-8.2.1
    ./configure --prefix=/usr/local/proj82/ --enable-static --disable-shared --without-curl --disable-tiff
    make

.. code-block:: bash

    sudo make install


.. _compilewin:

Windows
-------

For now, there is no compilation process on Windows, only cross-compilation via *Docker*, see :ref:`compileauto`.


.. _compilebasic:

Basic compilation
=================

Project setup:

.. code-block:: bash

    mkdir build
    cd build
    qmake ../Comp3D_cpp.pro

Compilation:

.. code-block:: bash

    make


Tests
=====

To compile the tests:

.. code-block:: bash

    cd tests
    qmake CONFIG+=release
    make

Then run the tests from ``tests/`` directory:

.. code-block:: bash

    ./Comp3D_tests


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

The software project is défined by *Comp3D_cpp.pro*.
It is possible to change the compilation options on the ``DEFINES +=`` lines of this file:

  - ``USE_QT``: to use Qt, mandatory in order to have messages translation
  - ``USE_RES``: depends on ``USE_QT``, to be able to create report ressources directory *res/* (see :ref:`out-report`)
  - ``USE_GUI``: depends on ``USE_QT``, to enable the GUI
  - ``USE_SIM``: to enable simulations (see :ref:`simulation`)
  - ``USE_AUTO``: to enable the automatic mode (see :ref:`automatization`)
  - ``ADD_PROJ_CC``, ``ADD_PROJ_NTF``, ``ADD_PROJ_UTM``: to fill the pre-recorded projections list


.. _compiledoc:

Documentation generation
========================


User documentation
------------------

The user documentation is in *doc_uni/*. It is written in *reStructuredText* and compiled by *sphinx*.

Dependencies: 

.. code-block:: bash

    sudo apt install python3-sphinx python3-stemmer qttranslations5-l10n


.. code-block:: bash

    pip3 install -U sphinx-mathjax-offline sphinx_intl


To generate the html pages for every supported language:

.. code-block:: bash

    ./build_doc.sh

|c3| must be compiled to take into account the new user documentation.


Math documentation
------------------

The math documentation is in *doc_math/*, written in LaTeX.

Dependencies: 

.. code-block:: bash

    sudo apt install texlive-latex-extra texlive-lang-french

to generate de pdf file:

.. code-block:: bash

    make


