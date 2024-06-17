.. _gui:

========================
Graphical User Interface
========================


.. _gui-tables:

Main Tables
===========

The |gui| contains two main tables:

1. the list of points, containing for each point:

   - number
   - class
   - name
   - initial coordinates or their variation after compensation

2. the list of observations, containing for each observation:
   
   - observation number
   - a checkbox to activate or deactivate the observation
   - name of the observation's starting point
   - name of the observation's target point
   - observation type
   - observation precision (:math:`\sigma`)
   - initial/final normalized residual (:math:`Res\ \sigma`)

All information contained in these two tables can be sorted by any column.

Tooltips are available to give more information on the cells.


It is possible to increase the size of one of the tables by sliding the central vertical separation.
If you hide one table, the other one will show extra information such as:

   - for points:
   
     - number of active observations to the point

     - empirical confidence interval per element/parameter

   - for observations:

     - residue of the observation in *mm*

     - number of active observations to the target point

     - redundancy of the observation in *%* (see :ref:`normal-matrix-inversion`)



.. _gui_obs_activ:

Changing Observation Status
=============================

By clicking on the ``Act`` field of an observation you can activate or deactivate it.
You can relaunch the computation to check if you isolated an error.

.. note::

   The |c3| |gui| does not change the :ref:`.obs file <obsfiles>`.

.. warning::

   Reloading files, with ``Reload``, leads to the loss of all changes regarding the observations status deactivate/activate.
   You have to record deactivated observations in the :ref:`.obs file<obsfiles>` (see :ref:`deactivated-obs`).

.. _gui-project:

Project Menu
==================

The ``Project`` menu is used to create, open, setup and process projects.

The :ref:`project-params` and processing commands are also available as buttons.

.. _gui-tools:

Tools Menu
==================

See :ref:`tools`.


.. _gui-lang:

Software Language Preferences
=============================

Interface language can be changed via ``Comp3D>Language`` menu.
