.. _translation:

===========
Translation
===========

There are three targets for translation:

- graphical user interface (GUI)
- reports
- documentation


GUI
===

Translation
-----------

Dependencies:

.. code-block:: bash

    sudo apt install qttranslations5-l10n


Update list of strings to translate by using the lupdate target (in the build directory) :

.. code-block:: bash

    cmake --build . --target lupdate


Then run the the *linguist* GUI via

.. code-block:: bash

    linguist

and open file *gui/translations/Comp3D_fr.ts*.

Translations are compiled at the same time as the executable.


Add a new language translation
------------------------------

Add to the project the Qt ressources files corresponding to the language: *qt_XX.qm* and *qtbase_XX.qm* (that can be found in *qttranslations-opensource-src-5.X.X.zip*)
in *gui/translations* and in *ressource.qrc*.

Add the language 2-letters code in CMakeLists.txt:

.. code-block:: bash

    set (TRANSLATIONS fr XX)

and run lupdate:

.. code-block:: bash

    cmake --build . --target lupdate

Insert the language name and its abbrevation in the following lines of *src/compile.h*:

.. code-block:: bash

    #define SUPPORTED_LANG_CODE {"en","fr"}
    #define SUPPORTED_LANG_NAME {"English",u8"Fran\u00e7ais"}


  
Report
======

Translation
-----------

Mandatory for the first translation update, in *Comp3D_cpp/gui/html/*:

.. code-block:: bash

    git clone https://github.com/jmimu/simple_i18n

Then run, in *Comp3D_cpp/gui/html/simple_i18n/*:

.. code-block:: bash

    ./prep_translation.py ../visu_comp.js fr
    
(replace ``fr`` with the target language).

It will add untranslated strings to the translation list.
Replace the old list in *visu_comp.js* by the new one and translate the new strings that are between ``~``. 

|c3| must be compiled to take into account the new *visu_comp.js*.


Add a new language translation
------------------------------

To add a new language, add a new entry in the ``translations`` dict in *visu_comp.js*.



User documentation
==================

Translation
-----------

Dependencies:

.. code-block:: bash

    sudo apt install qttranslations5-l10n

The following commands are to be run in *doc_uni/* directory.

Update the strings to translate:

.. code-block:: bash

    ./up_translation.sh

The files to translate are `doc_uni/locale/??/LC_MESSAGES/*.po`, using *linguist*.

Apply the translations:

.. code-block:: bash

    ./build_doc.sh

|c3| must be compiled to take into account the new user documentation.

Add a new language translation
------------------------------

Update *build_doc.sh*.

