.. _project-params:

=====================
Project Configuration
=====================

The project configuration is available and editable via a |gui| when creating a new project using ``New`` or when using ``Configuration`` from :ref:`gui-project`. It is recorded into the *.comp* project file.

Project Basic Configuration
===========================

-  Metadata:

   -  Projet name

   -  Project description

-  Input:

   -  Main :ref:`COR <corfiles>` file

   -  Main :ref:`OBS <obsfiles>` file

-  Frame:

   -  Local center coordinates: coordinates of project center point in input projection. All the topometric points should be close to it

   -  Local frame:

      -  Center latitude: used to get spherical approximation of Earth radius (in decimal degrees, :math:`0.1^{\circ}` precision is sufficient).

   -  Georeferenced frame:

      -  CRS name: only used to pre-fill Proj definition

      -  Proj definition: something like ``epsg:32719``,
         ``IGNF:LAMB1`` or
         ``+proj=lcc +lon_0=-90 +lat_1=33 +lat_2=45``. Any projection, as long as it has a correct libProj definition, is possible (some additional grid files may be required).


Project Advanced Configuration
==============================

-  Input:

   -  COR Covariance Matrix File: a *.csv* file for covariances on coordinates constraints (see :ref:`corcov`)

   -  *COR* with ellipsoidal heights: if this checkbox is unchecked, altitude is used instead of ellipsoidal height (allowed only for 1D projects)

   -  Refraction coefficient: it only affects zenithal measurements (default value: 0.12)

-  Output:

   -  Decimal places: for output files and report

   -  Language for report

   -  Option to display the map in html report

   -  Option to remove comments from output file *.new*, to simplify importing as spreadsheet

-  Computation type:

   -  Compensation: least square adjustment

   -  Propagation simulation: simulation by variance propagation
      (see :ref:`simul-propag`)

   -  Monte-Carlo simulation: simulation by repeated random sampling
      (see :ref:`simul-mc`)

-  Computation options:

   -  Invert normal matrix: used to get confidence intervals (see :ref:`normal-matrix-inversion`)

   -  Internal contraints: use of internal constraints in the last iteration (see :ref:`internal-constraints`)

-  Iterations:

   -  Maximum iterations: the compensation will be interrupted if this number of iterations is reached.
      For Monte-Carlo simulations, this is the number of random samples

   -  Additional iterations: number of iterations forced after convergence (iterations will continue if convergence is unreached during forced iterations)

   -  Convergence criterion: when :math:`\sigma_0` relative variation is below this threshold, convergence is reached

