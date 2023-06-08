.. _corfiles:

=========
COR File
=========

The *.cor* file contains the initial values of points coordinates and their constraints.

All points may be declared or only the minimum amount for allowing automatic :ref:`cap-init` to compute the coordinates of all the other points.
In the case of a simulation project, it is mandatory to declare the coordinates of all points, as the observations values are not taken into account.

For example in case of a 3D computation, in order to settle the project translation, it is necessary to constrain at least an X, a Y and a Z coordinates, not necessarily on the same point. These constaints must appear in the *.cor file*.

.. _cor-protocol:

Protocol for .COR File
-----------------------

The coordinates file is a plain text file with *.cor* as an extension. It has one line per point, containing the following fields:

    +------------------------------------------------------------------------------------+
    |:math:`code\ name\ E\ N\ h\ [\sigma_E\ \sigma_N\ \sigma_h\ [\eta\ \xi]]\ [*comment]`|
    +------------------------------------------------------------------------------------+

- *code*: point dimension and type of contraints

   - ``0``: 3D free point

   - ``1``: 3D point constrained on ENh

   - ``2``: 3D point constrained on EN

   - ``3``: 3D point constrained on h

   - ``4``: 1D free point

   - ``5``: 1D point constrained on h

   - ``6``: 2D free point

   - ``7``: 2D point constrained on EN

   - ``8``: remote 2D free point, excluded from internal constraints

   - ``9``: remote 2D point constrained on EN, excluded from internal constraints

   - ``-1``: point to be ignored in *.obs* and not used in computation, useful for project debugging

- *name*: point name

- *E*, *N*: Easting and Northing coordinates in input projection of the project

- *h*: ellipsoidal height or altitude (see :ref:`ref-frame`)

- :math:`\sigma_E,\ \sigma_N,\ \sigma_h`: *a priori* precisions of constraints

- :math:`\eta,\ \xi`: Easting and Northing vertical deflection, in arcseconds

- *comment*: saved and displayed in the report

.. note::
    :math:`\sigma_E`, :math:`\sigma_N` and :math:`\sigma_h` are mandatory only in case of constrained points or when :math:`\eta` and :math:`\xi` are expected.

    A *negative value* for :math:`\sigma` deactivates the corresponding constraint.

    If :math:`\sigma` on a constrained coordinate is set to *0*, the corresponding parameter is removed from the computation (the coordinate is fixed).

.. note::
    :math:`\eta` and :math:`\xi` are mandatory on every verticalized point in order to use vertical deflection in computation.

.. note::
    For ignored points (code ``-1``), only the code and the point name are mandatory.

.. note::
    *.cor* subfiles can be included into a *.cor* file using *@* (see :ref:`ex-cor-file`).


.. _ref-frame:

Reference Frame in .COR File
-----------------------------

Coordinates in a *.cor* file are provided in a local stereographic projection, in meters or using any projection, as defined in :ref:`project-params`.

The vertical component must be close to the ellipsoidal height, except in case of leveling (1D) compensation where altitude can be used (see :ref:`project-params`).


.. note::
    The *a priori* precisions are always given in the internal computation frame (see :ref:`spherical-coord`), that has a scale error an convergience of meridians different from user projection.


.. _1D-points:

1D Points
---------

In the case of 1D points (code ``4`` or ``5``), all coordinates are mandatory.
The altimetric component will be the only parameter for least squares, but the planimetric coordinates are used to compute the precision of the observation, if it has a part relative to distance, and the correction of the vertical deviation.

The planimetric coordinates are also used to correctly display the points on the report map.


.. _2D-points:

2D Points
---------

In the case of 2D points  (code ``6`` to ``9``), all coordinates are mandatory, the vertical coordinate is used only as information and for precision computation.

Remote 2D points (code ``8`` and ``9``) are the same as normal 2D points except that
they are not used for internal constraints (see :ref:`internal-constraints`).

.. _ex-cor-file:

Example of .COR File
--------------------

.. code-block:: none

    1  S1 100.000  100.000  50.000 0.0010 0.0010 0.0010   *3d constr
    2  S3  82.961  103.782  50.074 0.0100 0.0100 0.0000   *plani constr
    0  S2  89.930  100.827  50.067                        *free
    3  S4  76.453  100.878  50.034 0.0000 0.0000 0.0001   *alti constr
    0  S5  64.648  100.000  49.921 0.0000 0.0000 0.0000 
    0  S6  54.648  110.000  38.451 0.0000 0.0000 0.0000 0.0003 0.0002 *vert deflection
    1  S7  64.648  100.000  49.921 0.0001 0.0001 0.0010 
    1  S8  64.648  100.000  49.921 0      0      0        *fixed point 
    1  S9 100.000  100.000  50.000 -0.0010 0.0010 0.0010  *N and h constr
   -1  S10                                                *ignored point
    @detail.cor * subfile

Each constraint on the coordinates of a point with :math:`\sigma>0` adds an observation in the bundle adjustment computation. 

.. _point-class:

Point Class
-----------------------

In the |gui| and report, the point class is displayed in an abbreviated form in order to easily see its dimensions and constraints. A ``-`` indicates a dimension without constraint, the name of the coordinate in lowercase letter when the coordinate is constrained and in uppercase letter when the coordinate is fixed, following this protocol:

- ``"---"``: 3D free point

- ``"--z"``: 3D point constrained on Z

- ``"  -"``: 1D free point

- ``"  z"``: 1D point constrained on Z

- ``"XYz"``: 3D point with fixed XY and constrained on Z

- ``"--R"``: remote free 2D point


.. _corcov:

COR Covariance Matrix File
--------------------------

If a ``COR Covariance Matrix File`` has been given in :ref:`project-params`, all the variances and covariances between coordinates constraints observations will be replaced by the values found in the *.csv* file.

The *.csv* file must have the same format as the one of :ref:`export-covar` tool.

This can be used to place a new set of observations in the exact same reference as an old network.

.. note::
    Fixed points are not affected by this since they have no coordinates constraints observations.

.. note::
    This is not used in :ref:`internal-constraints` mode since all coordinates constraints observations are discarded.

.. note::
    The covariances are not used in :ref:`simul-mc`.

