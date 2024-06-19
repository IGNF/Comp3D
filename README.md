![logo_IGN](data/logo_comp3d5.png)

**Comp3D** is an open-source micro-geodesy compensation software that enables computation on a limited spread network (few kilometers) using a global 3D least-squares bundle adjustment on several topometric observation types.
The computation is done in a local 3D system based on an oblique stereographic projection with a spherical Earth model.

Copyright 1992-2024 [IGN France](https://www.ign.fr/), licensed under **GPLv3** license, see [LICENSE.md](LICENSE.md).


Documentation
-------------
User documentation is accessible here:
[IGNF.github.io/Comp3D/doc](https://IGNF.github.io/Comp3D/doc)

or via *Comp3D>Help* menu.


Source files
------------

[github.com/IGNF/Comp3D](https://github.com/IGNF/Comp3D)

Installation
-------------

### Windows

Extract the archive and run *Comp3D.exe*.


### AppImage (any Linux)
```shell
sudo apt install libfuse2

chmod a+x Comp3d5???-x86_64.AppImage
./Comp3d5???-x86_64.AppImage
```

### Deb package on Debian/Ubuntu
```shell
sudo apt install libfuse2

sudo apt install ./comp3d???.deb
```

Deb installation creates a `comp3d5` link targeting the last Comp3D installation.


Getting started
---------------
In Comp3D open an example .comp file.
Read the `Getting Started` chapter of the user documentation.


Contributors
------------
Refer to [CONTRIBUTORS.md](CONTRIBUTORS.md)


Contributing
------------
Refer to [CONTRIBUTING.md](CONTRIBUTING.md)


License
-------
Comp3D is provided with absolutely no warranty, under **GPLv3** license, see [LICENSE.md](LICENSE.md).

Refer to [NOTICE.md](NOTICE.md) for the embedded and linked libraries.


How to Cite
-----------
Please cite Comp3D and IGN if you use this software in your research or project.
Proper citations help others find and reference this work and support its continued development.

To cite this software, please use the following reference:

```bibtex
@software{IGNComp3D,
  author = {IGN},
  title = {Comp3D},
  version = {5},
  year = {2024},
  url = {https://github.com/IGNF/Comp3D}
}
```

Contacts
--------
[https://github.com/IGNF/Comp3D](https://github.com/IGNF/Comp3D) -- [comp3d@ign.fr](comp3d@ign.fr)

![logo_IGN](data/logo_IGN.jpg)
