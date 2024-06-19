![logo_IGN](data/logo_comp3d5.png)

**Comp3D** est un programme de compensation de micro-géodésie qui permet de calculer un réseau peu étendu (quelques kilomètres) avec compensation simultanée en bloc des observations planimétriques et altimétri­ques. Le calcul a lieu en tridimensionnel local (la projection étant approximée par une stéréogra­phique oblique de la sphère de courbure moyenne au centre du chantier).

Copyright 1992-2024 [IGN France](https://www.ign.fr/), sous licence **GPLv3**, voir [LICENSE.md](LICENSE.md).


Fichiers source
---------------

[github.com/IGNF/Comp3D](https://github.com/IGNF/Comp3D)

Documentation
-------------
La documentation utilisateur est accessible ici :
[IGNF.github.io/Comp3D/doc](https://IGNF.github.io/Comp3D/doc)

ou via le menu *Comp3D>Aide*.


Installation
------------

### Utilisation version Windows

Décompresser le .zip et lancer Comp3D.exe.

### Utilisation à partir d'un AppImage (tous Linux)
```shell
sudo apt install libfuse2

chmod a+x Comp3d5???-x86_64.AppImage
./Comp3d5???-x86_64.AppImage
```

### Installation du paquet deb sous Debian/Ubuntu
```shell
sudo apt install libfuse2

sudo apt install ./comp3d???.deb
```

Un lien symbolique `comp3d5` est créé vers la dernière version de Comp3D installée.


Démarrage
---------
Dans Comp3D, ouvrir le fichier .comp d'un exemple.

Voir le chapitre `Getting Started` de la documentation utilisateur.


Contributeurs
------------
Voir [CONTRIBUTORS.md](CONTRIBUTORS.md)


Contribuer
------------
Voir [CONTRIBUTING.md](CONTRIBUTING.md)


Licence
-----------------------------
Comp3D est distribué sans aucune garante, sous la licence **GPLv3**, voir [LICENSE.md](LICENSE.md).

Voir [NOTICE.md](NOTICE.md) pour les bibliothèques embarquées et liées.


Comment Citer
-------------
Merci de citer Comp3D et l'IGN si vous utilisez ce logiciel dans vos recherches ou projets.
Les citations appropriées aident les autres à trouver et à référencer ce travail et soutiennent son développement continu.

Pour citer ce logiciel, veuillez utiliser la référence suivante :

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
