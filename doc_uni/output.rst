.. _output:

======
Output
======

.. _out-coord:

Output Coordinates
==================

*.new* text file is created after the compensation process; it contains the compensated coordinates in the user-input projection.
To convert *.new* into *.cor* in order to update initial coordinates, use :ref:`export-to-cor` tool .

If :ref:`normal-matrix-inversion` was computed, compensated coordinates in *.new* file are followed by empirical half confidence intervals.


.. _out-xyz:

Subframes Files
==================

All *.xyz* files are updated after compensation with a final comment containing:

- transformation between the subframe and the global cartesian frame
- measured points (=raw observations) in the global cartesian frame
- measured points (=raw observations) in the projection frame (as in *.new* file).

This transformation is used by :ref:`apply-transfo` tool.



.. _out-data:

Computation Data
==================

All computation data are stored in the *.comp* file, alongside project configuration.

This file is a JavaScript source that can be interpreted as JSON once its first line is removed, making it easily machine-readable.


.. _out-report:

Computation Report
==================

The report can be displayed as a web page by opening the *.comp.html* file, which takes the data from the corresponding *.comp* file.

.. note::
   The tables can be sorted by any column and are easy to copy-paste into spreadsheets, as units and other information are not selected.


Project Configuration
----------------------------

It contains the list of projects parameters and is equivalent to the ``Project Settings`` form from the |gui| (see :ref:`project-params`). It also contains a map of the project.

.. _computation-info:

Computation Information
----------------------------

It represents general information about the computation.
It is possible to hide or show the information messages with the corresponding buttons.



:math:`\sigma_0` Evolution - :math:`\chi^2` Test
---------------------------------------------------

The log scale graph represents the evolution of :math:`\sigma_0` along the iterative process.

The :math:`\chi^2` test limits are computed from the degrees of freedom of the least squares adjustment and they are also represented in the graph.

If :ref:`normal-matrix-inversion` was computed, the biggest confidence ellipsoid is also given.
This may help to detect a problem of external constraints that does not affect :math:`\sigma_0`.

.. _init-coord:

Initial Coordinates
----------------------------

Initial coordinates are the coordinates of the points in the user-input projection after :ref:`cap-init`. They contain
the :ref:`point class <point-class>`, comment as infotip, *a priori* precisions of constraints and the number of active observations on each point.

.. _output-obs:

Observations 
-----------------

The observations table contains all the observations in the given input order, with their *.obs* file name and line number as infotip and several indicators, such as:

-  *distance* between points
-  *total* :math:`\sigma`: combination of *absolute* and *relative* :math:`\sigma`
-  *residual*, *projected residual* and *normalized residual* (*initial residuals* if before computation)

:ref:`normal-matrix-inversion` mode  adds more indicators:

-  `a posteriori` :math:`\sigma`: parameters precision propagated to the observation
-  *standard residual*: *residual normalized* by `a posteriori` :math:`\sigma`
-  *redundancy*

and in case of sufficient redundancy:

-  *standardized residual*: residual divided by its own :math:`\sigma`
-  :math:`\nabla` (nabla): biggest non-detectable fault, internal fiability
-  *probable error*

.. note::
    Some columns can be folded by clicking on them.

.. _residual-repartition:

Residual Repartition
--------------------------

It is represented by a histogram showing how the residual are distributed by type, normalized residual values and observation distances.

Infotips are present on each bar to display more information.


.. _pseudo-random-prop:

Pseudo Random Propositions
-------------------------------

To help fix observations' `a priori` :math:`\sigma`, a factor is proposed for each type of observation (if sufficient redundancy).
It is calculated from residuals and redundancy (see the infotips for more information).

The residuals' repartition by distance helps determine if the *absolute* or *relative* :math:`\sigma` has to be changed in the *.obs* file.

.. _biggest-residuals:

Biggest Residuals
----------------------------

Biggest residuals are displayed in an observations table containing only suspicious observations, ordered by decreasing residuals, with a maximum of 20 observations.

The anchors on the beginning of the lines link to the observation's line in the table containing all the observations.


Similarities
----------------

This section shows the subframes (:ref:`cartesian-subframe` or :ref:`polar-subframe`), with
vertical error and transformation between the subframe and the global cartesian frame.


Axes
----------------

This section shows the :ref:`rotation-axis` with their parametrizations
(from left to right: axis orientation, orientation precision, origin)
and the list of their targets (circle parameter, number of positions).


.. _compensated-coord:

Compensated Coordinates
----------------------------

The compensated coordinates are given in the user-input projection,
with their displacements since initialization.

These coordinates are also exported in *.new* file (see :ref:`out-coord`).

.. _confidence-indic:

Points Empirical Confidence Indicators
----------------------------------------

These indicators are available after :ref:`normal-matrix-inversion`.

They are given in the spherical computation frame, therefore there is no scale error, nor convergence of meridians.

Points empirical confidence indicators are confidence indicators scaled by *final* :math:`\sigma_0` to take into account the potential `a priori` :math:`\sigma` "optimism" or "pessimism" of the observations.

.. _confidence-ellipsoids:

Confidence Ellipsoids
~~~~~~~~~~~~~~~~~~~~~~~~~~

The confidence ellipsoids at :math:`1\sigma` (confidence of 20% because of the three dimensions) are given as lengths and orientations for each half axis.


Half Confidence Intervals
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Half confidence intervals are the projections of the confidence ellipsoids on each direction
(68% confidence), corresponding to *a posteriori* :math:`\sigma` of points coordinates.
These :math:`\sigma` are also exported in *.new* file (see :ref:`out-coord`).

.. _mc_points_disp:

Points Displacements Simulation
-------------------------------------

This output is available only for the :ref:`simul-mc` mode.

