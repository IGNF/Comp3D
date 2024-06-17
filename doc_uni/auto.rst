.. _automatization:

==============
Automatization
==============

|c3| can be used without its |gui| to automatically compensate observations, in order to build a real-time monitoring system.

.. _run-auto:

Usage
-------------------------

Launch |c3| with a *.comp* file path as a command line parameter.

The output is the same as with |gui| (see :ref:`output`).

An automatic topometric software chain should be able to read the json data inside the *.comp* file directly.

The ``-l`` option can be used to change the project language. See ``-h`` option for help.

.. _project-params-auto:

Setting Project Configuration
------------------------------

The :ref:`project-params` is recorded in the *config* node of the *.comp* file.

An example of minimal *.comp* file can be obtained from the command line
when running |c3| with ``-h`` option, or via the :ref:`project-template` tool.
