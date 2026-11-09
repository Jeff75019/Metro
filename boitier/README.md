# Boîtier imprimable en 3D

Boîtier de poche pour la carte **Waveshare ESP32-S3-Touch-LCD-1.9** et sa batterie LiPo **803040** (40 × 30 × 8 mm, 1000 mAh), en trois pièces :

- la **coque** (avant et côtés) ;
- le **support**, une plaque clipsée entre la carte et la batterie : ses 4 plots tiennent la carte contre la vitre par ses trous M2, une fenêtre laisse passer le câble de la batterie ;
- le **fond**, qui se clipse par les petits côtés et cale la batterie.

![Aperçus : monté, vue éclatée, support, dos, intérieur de la coque, fond](apercus/planche.png)

- Encombrement : **58,7 × 35,1 × 24,6 mm** (65,7 mm de long avec le passant de dragonne).
- Fenêtre sur toute la vitre tactile (rebord de 1,4 mm), dégagement pour la fiche USB-C, passages pour les boutons BOOT et RST.
- La batterie se pose derrière le support : sa longueur ne gêne pas la fixation de la carte. Côté USB-C, une place est laissée pour le surplus de câble.
- Cotes de la carte : [plan officiel Waveshare](https://docs.waveshare.com/ESP32-S3-LCD-1.9) (version Touch).

## Fichiers

| Fichier | Contenu |
|---|---|
| `coque.stl` | coque, à imprimer face avant sur le plateau |
| `support.stl` | plaque de maintien, à imprimer plots vers le haut |
| `fond.stl` | fond, à imprimer face extérieure (gravée « USB-C ») sur le plateau |
| `boitier.scad` | modèle paramétrique [OpenSCAD](https://openscad.org) (logiciel libre) |
| `apercus/` | images des aperçus |

## Avant d'imprimer : 2 mesures à vérifier

Les STL sont prévus pour une batterie **803040 (40 × 30 × 8 mm, 1000 mAh)**. Si la vôtre est différente, ou si la carte n'a pas tout à fait l'épaisseur du plan, ouvrez `boitier.scad` dans OpenSCAD, changez les valeurs en tête de fichier, puis exportez à nouveau (F6, puis « Exporter en STL ») :

| Paramètre | Valeur prévue | Comment le mesurer |
|---|---|---|
| `bat_long`, `bat_larg`, `bat_ep` | 40 × 30 × 8 | dimensions de la batterie (souvent écrites dessus : « 803040 » = 8 mm × 30 mm × 40 mm) ; jusqu'à 48 × 31 mm à plat |
| `dos_carte` | 7,8 | de la face avant de la vitre au dos du circuit vert (hors prise USB-C) |
| `jeu` | 0,25 | jeu autour de la vitre : 0,3 si votre imprimante « gonfle » les trous |

Pour exporter en ligne de commande :

```bash
openscad -D 'piece="coque"'   -o coque.stl   boitier.scad
openscad -D 'piece="support"' -o support.stl boitier.scad
openscad -D 'piece="fond"'    -o fond.stl    boitier.scad
```

## Impression

- PLA ou PETG, couche de 0,2 mm, 3 périmètres, remplissage 20 %.
- **Sans support d'impression** : coque face avant sur le plateau, support plots vers le haut, fond face gravée sur le plateau.
- Conseil : imprimez d'abord la coque seule et vérifiez que la vitre s'y loge (sans forcer) et que la prise USB-C tombe en face de son ouverture.

## Montage

1. **Vérifiez la polarité de la batterie** avant de la brancher : les fiches JST/MX 1,25 mm ne sont pas câblées pareil chez tous les vendeurs. Le rouge (+) doit aller sur le « + » marqué au dos de la carte à côté du connecteur BAT. Une inversion détruit le circuit de charge.
2. Posez la carte dans la coque, écran en premier : la vitre vient contre le rebord de la fenêtre, la prise USB-C en face de son ouverture.
3. Branchez la batterie sur la carte et passez son câble par la fenêtre du support.
4. Enfoncez le support, plots vers la carte : les plots entrent dans les trous M2 (il ne se monte que dans un sens) et la plaque se clipse dans la coque.
5. Posez la batterie à plat sur le support, **le côté des fils vers l'USB-C** : le surplus de câble se range dans la place libre de ce côté, sans être pincé.
6. Clipsez le fond, « USB-C » gravé du côté de la prise.
7. Pour ouvrir : glissez un ongle ou un tournevis plat dans l'encoche du grand côté.

BOOT et RST s'actionnent par les petites ouvertures des grands côtés, du bout de l'ongle ou avec un trombone.

## Limites

- Modèle établi d'après le plan du fabricant, **pas encore imprimé ni essayé** : faites un premier essai, puis ajustez les paramètres si besoin.
- Prévu pour une carte **sans barrettes de broches soudées** : avec des broches, augmentez `composants` de leur hauteur.
- Ne comprimez pas et ne percez pas la batterie : laissez-lui un peu de jeu (prévu : 0,6 mm).
