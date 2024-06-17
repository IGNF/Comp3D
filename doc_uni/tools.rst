.. _tools:

=====
Tools
=====

There are several tools available in the |gui| ``Tools`` menu, such as:

.. _matrix-obs:

Sight Matrix to Obs
------------------------

This tool is available via ``Tools > Sight Matrix to Obs``. It converts a sight matrix into a simulated *.obs* file.

The sight matrix is given as a text file having on the first line the targets names and on the first column the stations names.

Example of matrix file:

.. code-block:: none

       X  T1 T2 T3 T4 S1
       S1  1  0  0  1  0
       S2  1  1  1  0  1
       S3  0  0  1  0  0
       S4  1  1  0  0  1

.. note::
       ``1`` means that there is intervisibility between the given station and target; hence, three observations will be created into the output *.obs* file (horizontal angle, vertical angle and distance, see :ref:`simple-obs`).

In this example, there is intervisibility between station S1 and targets T1 and T4, which will create six observations from S1 in the output *.obs* file, intervisibility between station S2 and targets T1, T2, T3 and station S1 and so on.


.. _infinity-asc-to-bas:

Infinity ASC to BAS
-------------------

``Infinity ASC to BAS`` tool converts an *.asc* file, output of `Leica Infinity software
<https://leica-geosystems.com/fr-fr/products/gnss-systems/software/leica-infinity/>`_, into a |c3| :ref:`gnss-baselines` *.bas* file.


A *.bas* file is generated for each baselines' starting point from the *.asc* file.

If the input *.asc* file contains heights values, it means that these heights were taken into account when computing the baseline. Therefore, the tool adds the heights as comments and with 0 values in the output *.bas* file.


.. _exports:

.. _export-coord:

Export Coordinates
---------------------

The initial or compensated coordinates can be exported in geographic (decimal degrees or DMS) or cartesian (geocentric or project global) frames
using the tool available in ``Tools > Export Coordinates``.


.. _export-to-cor:

Export to Cor
-------------

``Export to Cor`` tool allows for the transformation of the initial or compensated coordinates into a *.cor* file.

.. warning::
       As the adjusted coordinates of constrained points have variations due to least squares adjustment, it is mandatory to provide their original values in the *.cor* file.


.. _export-sight:

Export Sight Matrix
---------------------

In order to export the sight matrix of a project it is necessary to provide the *.comp* file of the project when using the tool available in ``Tools > Export Sight Matrix``. The matrix is then exported as a *.csv*. file, to be used with :ref:`matrix-obs`.

.. _export-covar:

Export Variance-Covariance Matrix
----------------------------------

After :ref:`normal-matrix-inversion`, the variance-covariance matrix can be exported as a
*.comp_var_covar.csv* file via ``Tools > Export Sight Matrix``.
This matrix is not affected by the value of *final* :math:`\sigma_0`.

Each line and column of the output file starts with the name of the corresponding parameter.


.. _export-sinex:

Export Sinex
------------------

If the computation used a georeferenced projection, it is possible to export a subset of points to `sinex format <https://www.iers.org/IERS/EN/Organization/AnalysisCoordinator/SinexFormat/sinex.html>`_ by using ``Tools > Export Sinex``.

All metadata provided in the form are saved into the *.comp* file of the project for all future exports.

.. _relative-precision:

Export Relative Precision
------------------------------

After computation, the relative precision between a set of points can be exported using ``Tools > Export Relative Precision``.


.. _parameters-variations:

Export Parameters Variations
------------------------------

``Export Parameters Variations`` tool provides the possibility to export the variations of each parameter at each iteration after a compensation. It may be useful for debugging purposes.


.. _apply-transfo:

Apply Cartesian Transfo
------------------------------

This tool reads a cartesian transformation from a *.xyz* file (see :ref:`out-xyz`) and applies it to a cartesian coordinates file.

It is then possible to transform several point clouds from instrument frame (local frame) into project frame (global cartesian frame).


.. _cart-proj:

Cartesian from/to Projection
------------------------------

``Cartesian From/To Projection`` tool allows for the transformation between projected (local or georeferenced) coordinates and global cartesian coordinates.


.. _project-template:

Project File Template
------------------------------

This tool retrieves all the necessary files for the html report. It also writes a minimal *.comp* file to create computation configurations outside |c3|.
