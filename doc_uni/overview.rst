.. _overview:

========
Overview
========

|c3| is a micro-geodesy compensation software that enables computation on a limited spread network (few kilometers)
using a global 3D least-squares bundle adjustment on several topometric observation types.

The computation is done in a local 3D system based on an oblique stereographic projection with a spherical Earth model.

Features
================

- simple input text format (see :ref:`input-files`)

- input/output in any projection (see :ref:`georef-proj`)

- topometric observations (see :ref:`obsfiles`)

- subframes adjustment (see :ref:`cartesian-subframe` and :ref:`polar-subframe`)

- rotation axes determination (see :ref:`rotation-axis`)

- vertical deflection (see :ref:`obs-deflection`)

- equality constraints (see :ref:`obs-equal`)

- GNSS baselines (see :ref:`gnss-baselines`)

- error detection assistance (see :ref:`residual-repartition`, :ref:`rank-deficiency`)

- many statistical indicators (see :ref:`output-obs`)

- internal constraints (see :ref:`internal-constraints`)

- simulations (see :ref:`simulation`)

- `Sinex format <https://www.iers.org/IERS/EN/Organization/AnalysisCoordinator/SinexFormat/sinex.html>`_ export (see :ref:`export-sinex`)

- coordinates constraints covariance matrix input (see :ref:`corcov`)

- relative precision export (see :ref:`relative-precision`)

- possibility of integrating an automatic pipeline (see :ref:`automatization`)

- and more



Example of Workflow
===================

Input preparation:

#. Create observations :doc:`obs` from measurements

#. Set coordinates for some starting points in :doc:`cor`

#. :ref:`create-proj`

Computation:

#. Initialization of all parameters (see :ref:`cap-init` step)

#. Bundle adjustment on all observations (see :ref:`computation`)

Error correction:

#. Study results (see :ref:`output-obs`)

#. Isolate errors (see :ref:`deactivated-obs`) and fix them

Output results:

#. :ref:`out-report`

#. :ref:`Exports <exports>`

All input data come from text files (see :doc:`cor` and :doc:`obs`).
Project configuration and results are stored in a json-like :ref:`.comp file <out-data>` that can be displayed in :ref:`html <out-report>`.
Some simple output text files are also created, such as *.new* file (see :ref:`out-coord`).

When analyzing the results, please keep in mind that the computation is global and tridimensional.
Therefore, it is crucial to avoid all aberrant measurements which, even in small number and well localized, may greatly disturb the whole network.





