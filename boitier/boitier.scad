// =============================================================================
//  Boîtier imprimable en 3D : Waveshare ESP32-S3-Touch-LCD-1.9 + batterie LiPo
// =============================================================================
//  Trois pièces :
//    - la coque   : l'avant (fenêtre de l'écran) et les côtés (USB-C, BOOT, RST) ;
//    - le support : plaque clipsée dans la coque, derrière les broches de la carte ;
//                   ses 4 plots appuient sur les entretoises M2 et tiennent la carte
//                   contre la vitre, une fenêtre laisse passer le câble de la batterie ;
//    - le fond    : se clipse par les petits côtés et cale la batterie.
//  La batterie se pose derrière le support : la plaque la sépare des pointes des
//  broches. Côté USB-C, une place est laissée pour le surplus de câble.
//
//  Cotes de la carte : plan officiel Waveshare (docs.waveshare.com, « ESP32-S3-
//  LCD-1.9 », plan de la version Touch). Repères : X le long de la carte (USB-C
//  en X = 0), Y en hauteur, Z en profondeur depuis la face avant (écran).
//
//  Exports :
//    openscad -D 'piece="coque"'   -o coque.stl   boitier.scad
//    openscad -D 'piece="support"' -o support.stl boitier.scad
//    openscad -D 'piece="fond"'    -o fond.stl    boitier.scad
//  Impression : PLA ou PETG, 0,2 mm, sans support d'impression. Coque : face
//  avant sur le plateau ; support : plots vers le haut ; fond : face gravée
//  sur le plateau.
// =============================================================================

piece = "ensemble";  // "coque", "support", "fond", "ensemble" (aperçu), "eclate" (aperçu éclaté)

/* [Batterie] */
bat_long = 40.0;   // 803040 : 40 × 30 × 8 mm, 1000 mAh, fiche JST/MX 1,25 mm
bat_larg = 30.0;
bat_ep   = 8.0;

/* [Carte (plan Waveshare)] */
verre_l = 55.00;   // vitre tactile
verre_h = 31.40;
verre_r = 1.00;
verre_ep = 2.50;   // vitre + écran
carte_l = 52.60;   // circuit imprimé
carte_h = 29.00;
retrait_carte = 1.20;  // retrait du circuit par rapport au bord de la vitre
actif_l = 43.12;   // zone d'affichage
actif_h = 23.10;
actif_x = 7.90;    // marge côté USB-C (l'affichage est décentré)
dos_carte = 7.80;  // de la face avant de la vitre au dos du circuit
// Au dos du circuit (hauteurs depuis le dos du circuit) :
entretoise = 4.00; // entretoises M2 en laiton aux 4 trous (0 : carte sans entretoises)
broches = 9.60;    // barrettes de broches soudées, pointes comprises (0 : sans barrettes)
composants = 3.60; // autres composants (USB-C, connecteur de batterie, boutons)
rangees_y = [5.61, 23.39];  // barrettes (2 × 20 broches au pas de 2,54, 17,78 mm d'écart)
// Trous M2 (depuis le bord gauche et le bord haut du circuit)
trou_x = [9.46, 50.21];
trou_y = [2.00, 27.00];
// Connecteur de la batterie, au dos du circuit (depuis les mêmes bords)
bat_conn_x = 20.8;
// USB-C (centré en hauteur, au dos du circuit)
usb_l = 9.0;       // largeur de la prise
usb_h = 3.3;       // épaisseur de la prise
// Boutons BOOT et RST : sur les grands côtés, près de l'USB-C ; leur poussoir
// blanc dépasse du circuit presque jusqu'au bord de la vitre
bouton_x = 4.40;   // depuis le bord gauche du circuit

/* [Boîtier] */
paroi = 1.6;       // épaisseur des côtés
face = 1.2;        // épaisseur de la face avant (rebord sur la vitre)
fond_ep = 1.6;     // épaisseur du fond
support_ep = 1.4;  // épaisseur de la plaque de maintien
jeu = 0.25;        // jeu autour de la vitre (ajustez selon votre imprimante)
recouvrement = 1.4;// rebord de la face avant sur la vitre
rayon = 4.0;       // arrondi des angles extérieurs
cordon = true;     // passant pour une dragonne, côté opposé à l'USB-C
boutons_souples = true;  // BOOT et RST : boutons intégrés à la paroi (false : simples trous)
course = 0;        // saillie d'un poussoir sous la languette (0 : languette au ras de la paroi)

$fn = 48;

// ---- Dimensions dérivées -------------------------------------------------------
int_l = verre_l + 2 * jeu;              // intérieur de la coque
int_h = verre_h + 2 * jeu;
ext_l = int_l + 2 * paroi;
ext_h = int_h + 2 * paroi;
x0 = paroi + jeu;                       // coin de la vitre dans le repère du boîtier
y0 = paroi + jeu;
z_dos = face + dos_carte;               // dos du circuit
h_dos = max(broches + 0.6, entretoise + 0.4, composants + 0.6);  // place derrière le circuit
z_support = z_dos + h_dos;              // face avant du support (côté carte)
plot_h = z_support - (z_dos + entretoise + 0.1);  // plots : jusqu'aux entretoises (0,1 mm de jeu)
z_bat = z_support + support_ep;         // la batterie se pose ici
coque_ep = z_bat + bat_ep + 0.6;        // face intérieure du fond (0,6 mm de jeu)
usb_z = z_dos + usb_h / 2;              // centre de la prise USB-C
bouton_z = z_dos + 0.9;                 // centre des boutons latéraux
btn_cx = x0 + retrait_carte + bouton_x;  // centre des boutons (X)
// Bouton intégré : languette souple découpée dans la paroi (fente en U), amincie
// côté extérieur, avec une pastille sous le doigt et un poussoir côté intérieur.
// Orientée le long de la paroi (dans le sens des couches) : elle plie sans casser.
lang_h = 5.0;                           // hauteur de la languette
lang_l = 13.0;                          // longueur (bout libre sur le bouton)
lang_x0 = btn_cx - 2.5;                 // bout libre, côté USB-C
fente = 0.7;
creux = 0.6;                            // amincissement : languette de 1 mm
levre = 3.0;                            // pattes du fond qui entrent dans la coque
clip_z = coque_ep - levre / 2;          // hauteur des clips du fond
jl = 0.15;                              // jeu des pièces glissées dans la coque
patte = 1.2;                            // épaisseur des pattes du fond
// Batterie contre le petit côté opposé à l'USB-C (place pour une patte du fond) ;
// le surplus de câble se range côté USB-C
bat_x = paroi + int_l - (jl + patte + 0.3) - bat_long;
bat_y = paroi + (int_h - bat_larg) / 2;
conn_x = x0 + retrait_carte + bat_conn_x;  // connecteur de la batterie (X)

assert(bat_larg <= int_h - 0.4, "batterie trop large pour le boîtier");
assert(bat_x >= paroi + jl + patte + 4, "batterie trop longue pour le boîtier");

module rrect(l, h, r, ep) {  // rectangle arrondi extrudé
  r2 = min(r, l / 2, h / 2);
  linear_extrude(ep) offset(r2) offset(-r2) square([l, h]);
}

// ---- Coque -------------------------------------------------------------------
module coque() {
  difference() {
    rrect(ext_l, ext_h, rayon, coque_ep);
    // intérieur (la vitre se pose contre le rebord de la face avant)
    translate([paroi, paroi, face]) rrect(int_l, int_h, verre_r + jeu, coque_ep);
    // fenêtre de l'écran, chanfreinée à l'extérieur
    translate([x0 + recouvrement, y0 + recouvrement, -0.01])
      rrect(verre_l - 2 * recouvrement, verre_h - 2 * recouvrement, 1.2, face + 0.02);
    hull() {
      translate([x0 + recouvrement, y0 + recouvrement, face * 0.4])
        rrect(verre_l - 2 * recouvrement, verre_h - 2 * recouvrement, 1.2, 0.01);
      translate([x0 + recouvrement - 0.7, y0 + recouvrement - 0.7, -0.01])
        rrect(verre_l - 2 * recouvrement + 1.4, verre_h - 2 * recouvrement + 1.4, 1.9, 0.01);
    }
    // USB-C (petit côté X = 0), avec un dégagement extérieur pour la fiche du câble
    translate([-0.01, y0 + verre_h / 2, usb_z]) rotate([0, 90, 0]) {
      linear_extrude(paroi + 0.1) offset(1.2) offset(-1.2) square([usb_h + 1.2, usb_l + 1.0], center = true);
      linear_extrude(0.6) offset(2) offset(-2) square([usb_h + 4.0, usb_l + 4.0], center = true);
    }
    // rainure intérieure au-dessus de la prise : si elle dépasse un peu de la vitre,
    // elle glisse jusqu'à son ouverture sans frotter sur la paroi
    translate([paroi - 0.4, y0 + verre_h / 2 - (usb_l + 1.0) / 2, usb_z]) cube([0.41, usb_l + 1.0, coque_ep]);
    // BOOT et RST, un sur chaque grand côté : boutons intégrés ou simples trous
    for (m = [0, 1]) translate([0, m ? ext_h : 0, 0]) mirror([0, m, 0]) {
      if (boutons_souples) {
        for (z = [bouton_z + lang_h / 2, bouton_z - lang_h / 2 - fente])  // fentes du haut et du bas
          translate([lang_x0 - fente, -0.01, z]) cube([lang_l + fente, paroi + 0.02, fente]);
        translate([lang_x0 - fente, -0.01, bouton_z - lang_h / 2 - fente]) cube([fente, paroi + 0.02, lang_h + 2 * fente]);
        translate([lang_x0, -0.01, bouton_z - lang_h / 2]) cube([lang_l, creux + 0.01, lang_h]);  // creux extérieur
      } else {
        translate([btn_cx - 2.0, -0.01, bouton_z - 1.75]) cube([4.0, paroi + 0.02, 3.5]);
      }
    }
    // rainures des clips du support (grands côtés)
    for (y = [paroi - 0.5, ext_h - paroi - 0.01])
      translate([ext_l / 2 - 9, y, z_support + support_ep / 2 - 0.6]) cube([18, 0.51, 1.2]);
    // rainures des clips du fond (petits côtés)
    for (x = [paroi - 0.5, ext_l - paroi - 0.01])
      translate([x, ext_h / 2 - 8, clip_z - 0.6]) cube([0.51, 16, 1.2]);
    // encoche pour ouvrir (grand côté, au ras du fond)
    translate([ext_l - 22, -0.01, coque_ep - 1.2]) cube([10, paroi + 0.02, 1.21]);
  }
  // boutons intégrés : pastille sous le doigt (affleurante). La languette est au ras
  // de la paroi à l'intérieur : en appuyant, elle pousse le bouton de la carte.
  // Un poussoir (course > 0) n'est utile que si le bouton est loin de la paroi :
  // s'il touche le bouton au repos, la carte se monte de travers.
  if (boutons_souples)
    for (m = [0, 1]) translate([0, m ? ext_h : 0, 0]) mirror([0, m, 0]) {
      translate([btn_cx, creux, bouton_z]) rotate([90, 0, 0]) cylinder(r = 1.6, h = creux);
      if (course > 0) hull() {
        translate([btn_cx - 1.5, paroi - 0.01, bouton_z - 1.2]) cube([3, course + 0.01, 2.4]);
        translate([btn_cx - 1.5, paroi - 0.01, bouton_z + 2.4]) cube([3, 0.02, 0.01]);
      }
    }
  // passant pour une dragonne (au ras de la face avant : posé sur le plateau, sans support)
  if (cordon)
    translate([ext_l - 0.5, ext_h / 2, 0]) difference() {
      hull() {
        translate([0, -4, 0]) cube([0.5, 8, 4]);
        translate([4.5, 0, 0]) cylinder(r = 3, h = 4);
      }
      translate([4.2, 0, -0.1]) cylinder(r = 1.4, h = 4.2);
    }
}

// ---- Support (dans le repère de la coque, plaque en Z = 0, plots vers +Z) ------
module support() {
  ancr = 0.5;  // plots ancrés dans la plaque : une seule pièce
  difference() {
    translate([paroi + jl, paroi + jl, 0]) rrect(int_l - 2 * jl, int_h - 2 * jl, verre_r + jeu, support_ep);
    // fenêtre du câble : de la place laissée côté USB-C jusqu'au connecteur de la carte
    translate([paroi + jl + 2.0, ext_h / 2 - 6, -0.1]) rrect(conn_x + 6 - (paroi + jl + 2.0), 12, 2, support_ep + 0.2);
  }
  // clips (grands côtés)
  for (s = [0, 1])
    translate([ext_l / 2 - 8, s ? ext_h - paroi - jl - 0.01 : paroi + jl - 0.4 + 0.01, support_ep / 2 - 0.4])
      cube([16, 0.4, 0.8]);
  // plots : appui sur le bout des entretoises M2 (sans entretoises : sur le circuit,
  // avec un ergot de centrage dans le trou)
  for (tx = trou_x, ty = trou_y)
    translate([x0 + retrait_carte + tx, y0 + retrait_carte + ty, support_ep - ancr]) {
      cylinder(d = 3.4, h = plot_h + ancr);
      if (entretoise == 0) cylinder(d = 1.6, h = plot_h + ancr + 1.2);
    }
}

// ---- Fond ----------------------------------------------------------------------
module fond() {
  ancr = 0.5;  // pattes et nervures ancrées dans la plaque : une seule pièce
  // plaque extérieure, « USB-C » gravé du côté de la prise
  difference() {
    rrect(ext_l, ext_h, rayon, fond_ep);
    translate([11, ext_h / 2, -0.01]) mirror([0, 1, 0]) linear_extrude(0.6)
      text("USB-C", size = 4.5, halign = "center", valign = "center", font = "Liberation Sans:style=Bold");
  }
  // pattes de clipsage (petits côtés), avec leur bossage
  for (s = [0, 1]) {
    px = s ? ext_l - paroi - jl - patte : paroi + jl;
    translate([px, ext_h / 2 - 7, fond_ep - ancr]) cube([patte, 14, levre + ancr]);
    translate([s ? ext_l - paroi - jl - 0.01 : paroi + jl - 0.39, ext_h / 2 - 6, fond_ep + levre / 2 - 0.4])
      cube([0.4, 12, 0.8]);
  }
  // nervures qui calent la batterie (côté câble, et en haut et en bas)
  translate([0, 0, fond_ep - ancr]) {
    translate([bat_x - 1.6, ext_h / 2 - 6, 0]) cube([1.2, 12, 2.5 + ancr]);
    for (y = [bat_y - 1.4, bat_y + bat_larg + 0.2])
      if (y > paroi + jl && y + 1.2 < ext_h - paroi - jl)
        translate([bat_x + bat_long / 2 - 6, y, 0]) cube([12, 1.2, 2.5 + ancr]);
  }
}

// ---- Aperçus -----------------------------------------------------------------
module carte_seule() {  // vitre, circuit et ce qu'il porte au dos (sans la batterie)
  cx = x0 + retrait_carte; cy = y0 + retrait_carte;
  color("black") translate([x0, y0, face]) rrect(verre_l, verre_h, verre_r, verre_ep);
  color("royalblue") translate([x0 + actif_x, y0 + (verre_h - actif_h) / 2, face - 0.01]) cube([actif_l, actif_h, 0.05]);
  color("black") translate([cx, cy, face + verre_ep]) cube([carte_l, carte_h, z_dos - 1.6 - face - verre_ep]);
  color("darkgreen") translate([cx, cy, z_dos - 1.6]) cube([carte_l, carte_h, 1.6]);
  color("silver") translate([x0 + 0.1, y0 + verre_h / 2 - usb_l / 2, z_dos]) cube([7.5, usb_l, usb_h]);
  color("white") translate([conn_x - 3, ext_h / 2 - 2.5, z_dos]) cube([6, 5, 3.5]);
  if (entretoise > 0) color("gold") for (tx = trou_x, ty = trou_y)
    translate([cx + tx, cy + ty, z_dos]) cylinder(d = 3.5, h = entretoise);
  if (broches > 0) for (ry = rangees_y) {
    color("dimgray") translate([cx + 0.75, cy + ry - 1.27, z_dos]) cube([20 * 2.54, 2.54, min(2.5, broches)]);
    color("gold") for (k = [0 : 19]) translate([cx + 2.0 + k * 2.54 - 0.32, cy + ry - 0.32, z_dos]) cube([0.64, 0.64, broches]);
  }
  // boutons BOOT et RST : boîtier au dos, poussoir blanc jusqu'à 0,2 mm du bord de la vitre
  for (m = [0, 1]) translate([0, m ? ext_h : 0, 0]) mirror([0, m, 0]) {
    color("silver") translate([btn_cx - 2.2, cy, z_dos]) cube([4.4, 2.6, 1.8]);
    color("white") translate([btn_cx - 0.8, y0 + 0.2, bouton_z - 0.6]) cube([1.6, cy - y0, 1.2]);
  }
}
module carte_fantome(bat_dz = 0) {  // carte et batterie
  carte_seule();
  color("orange") translate([bat_x, bat_y, z_bat + bat_dz]) cube([bat_long, bat_larg, bat_ep]);
}

module support_monte(dz = 0) {  // support à sa place (plots vers la carte)
  translate([0, ext_h, z_support + support_ep + dz]) rotate([180, 0, 0]) support();
}
module fond_monte(dz = 0) {
  translate([0, ext_h, coque_ep + fond_ep + dz]) rotate([180, 0, 0]) fond();
}

if (piece == "coque") coque();
else if (piece == "support") support();
else if (piece == "fond") fond();
else if (piece == "ensemble" || piece == "eclate")  // aperçus : écran vers le haut
  rotate([180, 0, 0]) translate([0, -ext_h, 0]) {
    e = piece == "eclate";
    color("gainsboro") coque();
    if (e) carte_fantome(22); else %carte_fantome();
    color("darkseagreen") support_monte(e ? 16 : 0);
    color("lightsteelblue") fond_monte(e ? 32 : 0);
  }
