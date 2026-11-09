// =============================================================================
//  Boîtier imprimable en 3D : Waveshare ESP32-S3-Touch-LCD-1.9 + batterie LiPo
// =============================================================================
//  Deux pièces :
//    - la coque : l'avant (fenêtre de l'écran) et les côtés (USB-C, BOOT, RST) ;
//    - le fond  : se clipse dans la coque ; ses 4 plots maintiennent la carte
//                 par ses trous M2 et des nervures calent la batterie.
//
//  Cotes de la carte : plan officiel Waveshare (docs.waveshare.com, « ESP32-S3-
//  LCD-1.9 », plan de la version Touch). Repères : X le long de la carte (USB-C
//  à gauche, X = 0), Y en hauteur, Z en profondeur depuis la face avant (écran).
//
//  Avant d'imprimer : mesurez votre batterie et vérifiez « dos_carte » (voir
//  ci-dessous), puis exportez :
//    openscad -D 'piece="coque"' -o coque.stl boitier.scad
//    openscad -D 'piece="fond"'  -o fond.stl  boitier.scad
//  Impression : PLA ou PETG, 0,2 mm, sans support. La coque se pose face avant
//  sur le plateau, le fond face extérieure sur le plateau.
// =============================================================================

piece = "ensemble";  // "coque", "fond", "ensemble" (aperçu monté), "eclate" (aperçu éclaté)

/* [Batterie : mesurez la vôtre] */
bat_long = 35.0;   // le long de la carte (ex. 503035 : 35 × 30 × 5 mm, 500 mAh)
bat_larg = 30.0;
bat_ep   = 5.0;

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
dos_carte = 7.80;  // de la face avant de la vitre au dos du circuit : À VÉRIFIER
composants = 4.00; // hauteur des composants au dos (USB-C, connecteur batterie)
// Trous M2 (depuis le bord gauche et le bord haut du circuit)
trou_x = [9.46, 50.21];
trou_y = [2.00, 27.00];
// USB-C (centré en hauteur, au dos du circuit)
usb_l = 9.0;       // largeur de la prise
usb_h = 3.3;       // épaisseur de la prise
// Boutons BOOT et RST : sur les grands côtés, près de l'USB-C
bouton_x = 4.40;   // depuis le bord gauche du circuit

/* [Boîtier] */
paroi = 1.6;       // épaisseur des côtés
face = 1.2;        // épaisseur de la face avant (rebord sur la vitre)
fond_ep = 1.6;     // épaisseur du fond
jeu = 0.25;        // jeu autour de la vitre (ajustez selon votre imprimante)
recouvrement = 1.4;// rebord de la face avant sur la vitre
rayon = 4.0;       // arrondi des angles extérieurs
cordon = true;     // passant pour une dragonne, côté opposé à l'USB-C

$fn = 48;

// ---- Dimensions dérivées -------------------------------------------------------
int_l = verre_l + 2 * jeu;              // intérieur de la coque
int_h = verre_h + 2 * jeu;
ext_l = int_l + 2 * paroi;
ext_h = int_h + 2 * paroi;
// profondeur intérieure : vitre + carte, composants, batterie, un peu de marge
prof_int = dos_carte + composants + bat_ep + 0.6;
coque_ep = face + prof_int;             // le fond vient fermer l'arrière
x0 = paroi + jeu;                       // coin de la vitre dans le repère du boîtier
y0 = paroi + jeu;
z_dos = face + dos_carte;               // dos du circuit (Z)
usb_z = z_dos + usb_h / 2;              // centre de la prise USB-C
bouton_z = z_dos + 0.9;                 // centre des boutons latéraux
levre = 3.0;                            // lèvre du fond qui entre dans la coque
clip_z = coque_ep - levre / 2;          // hauteur des clips

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
    // USB-C (côté gauche), avec un dégagement extérieur pour la fiche du câble
    translate([-0.01, y0 + verre_h / 2, usb_z]) rotate([0, 90, 0]) {
      linear_extrude(paroi + 0.1) offset(1.2) offset(-1.2) square([usb_h + 1.2, usb_l + 1.0], center = true);
      linear_extrude(0.8) offset(2) offset(-2) square([usb_h + 4.0, usb_l + 4.0], center = true);
    }
    // BOOT et RST : un passage sur chaque grand côté (appui au doigt ou avec un trombone)
    for (y = [-0.01, ext_h - paroi - 0.01])
      translate([x0 + retrait_carte + bouton_x - 2.0, y, bouton_z - 1.75]) cube([4.0, paroi + 0.02, 3.5]);
    // rainures des clips du fond (grands côtés)
    for (y = [paroi - 0.5, ext_h - paroi - 0.01])
      translate([ext_l / 2 - 9, y, clip_z - 0.6]) cube([18, 0.51, 1.2]);
    // encoche pour ouvrir (grand côté du bas, au ras du fond)
    translate([ext_l - 22, -0.01, coque_ep - 1.2]) cube([10, paroi + 0.02, 1.21]);
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

// ---- Fond --------------------------------------------------------------------
module fond() {
  jl = 0.15;  // jeu de la lèvre dans la coque
  lev_l = int_l - 2 * jl;
  lev_h = int_h - 2 * jl;
  // plaque extérieure, « USB-C » gravé du côté de la prise (le fond ne se pose
  // que dans un sens : ses plots doivent tomber dans les trous de la carte)
  difference() {
    rrect(ext_l, ext_h, rayon, fond_ep);
    translate([11, ext_h / 2, -0.01]) mirror([0, 1, 0]) linear_extrude(0.6)
      text("USB-C", size = 4.5, halign = "center", valign = "center", font = "Liberation Sans:style=Bold");
  }
  // lèvre qui entre dans la coque, avec deux clips
  ancr = 0.5;  // lèvre, plots et nervures entrent dans la plaque : une seule pièce
  translate([paroi + jl, paroi + jl, fond_ep - ancr]) difference() {
    rrect(lev_l, lev_h, verre_r + jeu, levre + ancr);
    translate([1.2, 1.2, -0.1]) rrect(lev_l - 2.4, lev_h - 2.4, 0.5, levre + ancr + 0.2);
  }
  for (s = [0, 1])
    translate([ext_l / 2 - 8, s ? ext_h - paroi - jl - 0.01 : paroi + jl - 0.4 + 0.01, fond_ep + levre / 2 - 0.4])
      cube([16, 0.4, 0.8]);
  // plots : appui sur les trous M2 du circuit, avec un ergot de centrage
  haut_plot = coque_ep - z_dos;  // de la face intérieure du fond au dos du circuit
  for (tx = trou_x, ty = trou_y)
    translate([x0 + retrait_carte + tx, y0 + retrait_carte + ty, fond_ep - ancr]) {
      cylinder(d = 3.8, h = haut_plot + ancr);
      cylinder(d = 1.6, h = haut_plot + ancr + 1.2);
    }
  // nervures qui calent la batterie (entre les plots)
  bx = x0 + retrait_carte + (trou_x[0] + trou_x[1]) / 2 - bat_long / 2;
  by = paroi + int_h / 2 - bat_larg / 2;
  translate([0, 0, fond_ep - ancr]) for (c = [[bx - 1.6, by + bat_larg / 2 - 6, 1.2, 12],
                                       [bx + bat_long + 0.4, by + bat_larg / 2 - 6, 1.2, 12],
                                       [bx + bat_long / 2 - 6, max(by - 1.6, paroi + 1.6), 12, 1.2],
                                       [bx + bat_long / 2 - 6, min(by + bat_larg + 0.4, ext_h - paroi - 2.8), 12, 1.2]])
    translate([c[0], c[1], 0]) cube([c[2], c[3], 2.0 + ancr]);
}

// ---- Aperçus -----------------------------------------------------------------
module carte_fantome() {  // vitre, circuit, batterie (pour vérifier l'encombrement)
  color("black") translate([x0, y0, face]) rrect(verre_l, verre_h, verre_r, verre_ep);
  color("royalblue") translate([x0 + actif_x, y0 + (verre_h - actif_h) / 2, face - 0.01]) cube([actif_l, actif_h, 0.05]);
  color("darkgreen") translate([x0 + retrait_carte, y0 + retrait_carte, z_dos - 1.6]) cube([carte_l, carte_h, 1.6]);
  color("silver") translate([x0 - 0.01, y0 + verre_h / 2 - usb_l / 2, z_dos]) cube([7.5, usb_l, usb_h]);
  color("orange") translate([x0 + retrait_carte + (trou_x[0] + trou_x[1]) / 2 - bat_long / 2,
                             paroi + int_h / 2 - bat_larg / 2, coque_ep - bat_ep - 0.3]) cube([bat_long, bat_larg, bat_ep]);
}

if (piece == "coque") coque();
else if (piece == "fond") fond();
else  // aperçus : écran vers le haut, fond écarté de 25 mm en vue éclatée
  rotate([180, 0, 0]) translate([0, -ext_h, 0]) {
    ecart = piece == "eclate" ? 25 : 0;
    color("gainsboro") coque();
    if (piece == "eclate") carte_fantome(); else %carte_fantome();
    color("lightsteelblue") translate([0, ext_h, coque_ep + fond_ep + ecart]) rotate([180, 0, 0]) fond();
  }
