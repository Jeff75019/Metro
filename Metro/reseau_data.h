// Généré par tools/make_reseau.py — ne pas modifier à la main.
// Lignes de métro, RER et tramway d'Île-de-France, stations dans l'ordre de la ligne.
// Source : open data Île-de-France Mobilités (emplacement-des-gares-idf,
// traces-du-reseau-ferre-idf, projets_arrets_idf, projets_lignes_idf), licence ODbL.
#pragma once
#include <stdint.h>

// Noms des stations, chacun terminé par \0 (une seule fois par nom)
static const char NET_NAMES[] =
    "La Défense" "\0"
    "Esplanade de la Défense" "\0"
    "Pont de Neuilly" "\0"
    "Les Sablons" "\0"
    "Porte Maillot" "\0"
    "Argentine" "\0"
    "Charles De Gaulle-Étoile" "\0"
    "George V" "\0"
    "Franklin D. Roosevelt" "\0"
    "Champs-Élysées – Clemenceau" "\0"
    "Concorde" "\0"
    "Tuileries" "\0"
    "Palais Royal - Musée du Louvre" "\0"
    "Louvre-Rivoli" "\0"
    "Châtelet" "\0"
    "Hôtel de Ville" "\0"
    "Saint-Paul" "\0"
    "Bastille" "\0"
    "Gare de Lyon" "\0"
    "Reuilly-Diderot" "\0"
    "Nation" "\0"
    "Porte de Vincennes" "\0"
    "Saint-Mandé" "\0"
    "Bérault" "\0"
    "Château de Vincennes" "\0"
    "Porte Dauphine" "\0"
    "Victor Hugo" "\0"
    "Ternes" "\0"
    "Courcelles" "\0"
    "Monceau" "\0"
    "Villiers" "\0"
    "Rome" "\0"
    "Place de Clichy" "\0"
    "Blanche" "\0"
    "Pigalle" "\0"
    "Anvers" "\0"
    "Barbès-Rochechouart" "\0"
    "La Chapelle" "\0"
    "Stalingrad" "\0"
    "Jaurès" "\0"
    "Colonel Fabien" "\0"
    "Belleville" "\0"
    "Couronnes" "\0"
    "Ménilmontant" "\0"
    "Père Lachaise" "\0"
    "Philippe Auguste" "\0"
    "Alexandre Dumas" "\0"
    "Avron" "\0"
    "Pont de Levallois" "\0"
    "Anatole France" "\0"
    "Louise Michel" "\0"
    "Porte de Champerret" "\0"
    "Pereire-Levallois" "\0"
    "Wagram" "\0"
    "Malesherbes" "\0"
    "Europe" "\0"
    "Saint-Lazare" "\0"
    "Havre-Caumartin" "\0"
    "Opéra" "\0"
    "Quatre Septembre" "\0"
    "Bourse" "\0"
    "Sentier" "\0"
    "Réaumur Sébastopol" "\0"
    "Arts et Métiers" "\0"
    "Temple" "\0"
    "République" "\0"
    "Parmentier" "\0"
    "Rue Saint-Maur" "\0"
    "Gambetta" "\0"
    "Porte de Bagnolet" "\0"
    "Gallieni" "\0"
    "Porte des Lilas" "\0"
    "Saint-Fargeau" "\0"
    "Pelleport" "\0"
    "Porte de Clignancourt" "\0"
    "Simplon" "\0"
    "Marcadet-Poissonniers" "\0"
    "Château Rouge" "\0"
    "Gare du Nord" "\0"
    "Gare de l’Est" "\0"
    "Château d’Eau" "\0"
    "Strasbourg-Saint-Denis" "\0"
    "Étienne Marcel" "\0"
    "Les Halles" "\0"
    "Cité" "\0"
    "Saint-Michel" "\0"
    "Odéon" "\0"
    "Saint-Germain-des-Prés" "\0"
    "Saint-Sulpice" "\0"
    "Saint-Placide" "\0"
    "Montparnasse-Bienvenüe" "\0"
    "Vavin" "\0"
    "Raspail" "\0"
    "Denfert-Rochereau" "\0"
    "Mouton-Duvernet" "\0"
    "Alésia" "\0"
    "Porte d’Orléans" "\0"
    "Mairie de Montrouge" "\0"
    "Barbara" "\0"
    "Bagneux - Lucie Aubrac" "\0"
    "Bobigny Pablo Picasso" "\0"
    "Bobigny-Pantin - Raymond Queneau" "\0"
    "Église de Pantin" "\0"
    "Hoche" "\0"
    "Porte de Pantin" "\0"
    "Ourcq" "\0"
    "Laumière" "\0"
    "Jacques Bonsergent" "\0"
    "Oberkampf" "\0"
    "Richard Lenoir" "\0"
    "Bréguet-Sabin" "\0"
    "Quai de la Rapée" "\0"
    "Gare d’Austerlitz" "\0"
    "Saint-Marcel" "\0"
    "Campo Formio" "\0"
    "Place d’Italie" "\0"
    "Kléber" "\0"
    "Boissière" "\0"
    "Trocadéro" "\0"
    "Passy" "\0"
    "Bir-Hakeim" "\0"
    "Dupleix" "\0"
    "La Motte Picquet-Grenelle" "\0"
    "Cambronne" "\0"
    "Sèvres-Lecourbe" "\0"
    "Pasteur" "\0"
    "Edgar Quinet" "\0"
    "Saint-Jacques" "\0"
    "Glacière" "\0"
    "Corvisart" "\0"
    "Nationale" "\0"
    "Chevaleret" "\0"
    "Quai de la Gare" "\0"
    "Bercy" "\0"
    "Dugommier" "\0"
    "Daumesnil" "\0"
    "Bel-Air" "\0"
    "Picpus" "\0"
    "La Courneuve 8 Mai 1945" "\0"
    "Fort d’Aubervilliers" "\0"
    "Aubervilliers Pantin - Quatre Chemins" "\0"
    "Porte de la Villette" "\0"
    "Corentin Cariou" "\0"
    "Crimée" "\0"
    "Riquet" "\0"
    "Louis Blanc" "\0"
    "Château-Landon" "\0"
    "Poissonnière" "\0"
    "Cadet" "\0"
    "Le Peletier" "\0"
    "Chaussée d’Antin - La Fayette" "\0"
    "Pyramides" "\0"
    "Pont Neuf" "\0"
    "Pont Marie" "\0"
    "Sully-Morland" "\0"
    "Jussieu" "\0"
    "Place Monge" "\0"
    "Censier-Daubenton" "\0"
    "Les Gobelins" "\0"
    "Tolbiac" "\0"
    "Maison Blanche" "\0"
    "Porte d’Italie" "\0"
    "Le Kremlin-Bicêtre" "\0"
    "Villejuif - Léo Lagrange" "\0"
    "Villejuif Paul Vaillant-Couturier" "\0"
    "Villejuif - Louis Aragon" "\0"
    "Porte de Choisy" "\0"
    "Porte d’Ivry" "\0"
    "Pierre et Marie Curie" "\0"
    "Mairie d’Ivry" "\0"
    "Bolivar" "\0"
    "Buttes-Chaumont" "\0"
    "Botzaris" "\0"
    "Danube" "\0"
    "Pré Saint-Gervais" "\0"
    "Place des Fêtes" "\0"
    "Balard" "\0"
    "Lourmel" "\0"
    "Boucicaut" "\0"
    "Félix Faure" "\0"
    "Commerce" "\0"
    "École Militaire" "\0"
    "La Tour Maubourg" "\0"
    "Invalides" "\0"
    "Madeleine" "\0"
    "Richelieu-Drouot" "\0"
    "Grands Boulevards" "\0"
    "Bonne Nouvelle" "\0"
    "Filles du Calvaire" "\0"
    "Saint-Sébastien-Froissart" "\0"
    "Chemin Vert" "\0"
    "Ledru Rollin" "\0"
    "Faidherbe Chaligny" "\0"
    "Montgallet" "\0"
    "Michel Bizot" "\0"
    "Porte Dorée" "\0"
    "Porte de Charenton" "\0"
    "Liberté" "\0"
    "Charenton-Écoles" "\0"
    "École Vétérinaire de Maisons-Alfort" "\0"
    "Maisons-Alfort-Stade" "\0"
    "Maisons-Alfort-Les Juilliottes" "\0"
    "Créteil–L’Échat" "\0"
    "Créteil-Université" "\0"
    "Créteil–Préfecture" "\0"
    "Créteil - Pointe du Lac" "\0"
    "Pont de Sèvres" "\0"
    "Billancourt" "\0"
    "Marcel Sembat" "\0"
    "Porte de Saint-Cloud" "\0"
    "Exelmans" "\0"
    "Michel Ange-Molitor" "\0"
    "Michel Ange-Auteuil" "\0"
    "Jasmin" "\0"
    "Ranelagh" "\0"
    "La Muette" "\0"
    "Rue de la Pompe" "\0"
    "Iéna" "\0"
    "Alma-Marceau" "\0"
    "Saint-Philippe du Roule" "\0"
    "Miromesnil" "\0"
    "Saint-Augustin" "\0"
    "Saint-Ambroise" "\0"
    "Voltaire" "\0"
    "Charonne" "\0"
    "Rue des Boulets" "\0"
    "Buzenval" "\0"
    "Maraîchers" "\0"
    "Porte de Montreuil" "\0"
    "Robespierre" "\0"
    "Croix de Chavaux" "\0"
    "Mairie de Montreuil" "\0"
    "Église d’Auteuil" "\0"
    "Porte d’Auteuil" "\0"
    "Boulogne - Jean Jaurès" "\0"
    "Chardon Lagache" "\0"
    "Mirabeau" "\0"
    "Javel - André Citroën" "\0"
    "Charles Michels" "\0"
    "Avenue Émile Zola" "\0"
    "Ségur" "\0"
    "Duroc" "\0"
    "Vaneau" "\0"
    "Sèvres-Babylone" "\0"
    "Mabillon" "\0"
    "Cluny-La Sorbonne" "\0"
    "Maubert-Mutualité" "\0"
    "Cardinal Lemoine" "\0"
    "Boulogne - Pont de Saint-Cloud" "\0"
    "Rambuteau" "\0"
    "Goncourt" "\0"
    "Pyrénées" "\0"
    "Jourdain" "\0"
    "Télégraphe" "\0"
    "Mairie des Lilas" "\0"
    "Serge Gainsbourg" "\0"
    "Romainville - Carnot" "\0"
    "Montreuil - Hôpital" "\0"
    "La Dhuys" "\0"
    "Coteaux Beauclair" "\0"
    "Rosny-Bois-Perrier" "\0"
    "Mairie d’Aubervilliers" "\0"
    "Aimé Césaire" "\0"
    "Front Populaire" "\0"
    "Porte de la Chapelle" "\0"
    "Marx Dormoy" "\0"
    "Jules Joffrin" "\0"
    "Lamarck-Caulaincourt" "\0"
    "Abbesses" "\0"
    "Saint-Georges" "\0"
    "Notre-Dame-de-Lorette" "\0"
    "Trinité d’Estienne d’Orves" "\0"
    "Assemblée Nationale" "\0"
    "Solférino" "\0"
    "Rue du Bac" "\0"
    "Rennes" "\0"
    "Notre-Dame-des-Champs" "\0"
    "Montparnasse Bienvenüe" "\0"
    "Falguière" "\0"
    "Volontaires" "\0"
    "Vaugirard" "\0"
    "Convention" "\0"
    "Porte de Versailles" "\0"
    "Corentin Celton" "\0"
    "Mairie d’Issy" "\0"
    "Les Courtilles" "\0"
    "Les Agnettes" "\0"
    "Gabriel Péri" "\0"
    "Mairie de Clichy" "\0"
    "Porte de Clichy" "\0"
    "Brochant" "\0"
    "La Fourche" "\0"
    "Liège" "\0"
    "Varenne" "\0"
    "Saint-François-Xavier" "\0"
    "Gaîté" "\0"
    "Pernety" "\0"
    "Plaisance" "\0"
    "Porte de Vanves" "\0"
    "Malakoff - Plateau de Vanves" "\0"
    "Malakoff - Rue Étienne Dolet" "\0"
    "Châtillon-Montrouge" "\0"
    "Guy Môquet" "\0"
    "Porte de Saint-Ouen" "\0"
    "Garibaldi" "\0"
    "Mairie de Saint-Ouen" "\0"
    "Carrefour Pleyel" "\0"
    "Saint-Denis - Porte de Paris" "\0"
    "Basilique de Saint-Denis" "\0"
    "Saint-Denis - Université" "\0"
    "Saint-Denis Pleyel" "\0"
    "Saint-Ouen" "\0"
    "Pont Cardinet" "\0"
    "Cour Saint-Émilion" "\0"
    "Bibliothèque François Mitterrand" "\0"
    "Olympiades" "\0"
    "Hôpital Bicêtre" "\0"
    "Villejuif - Gustave Roussy" "\0"
    "L’Haÿ-les-Roses" "\0"
    "Chevilly-Larue" "\0"
    "Thiais - Orly" "\0"
    "Aéroport d’Orly" "\0"
    "La Courneuve-Six Routes" "\0"
    "Le Bourget" "\0"
    "Le Blanc-Mesnil" "\0"
    "Aulnay" "\0"
    "Sevran-Beaudottes" "\0"
    "Sevran-Livry" "\0"
    "Clichy-Montfermeil" "\0"
    "Chelles Gournay" "\0"
    "Noisy-Champs" "\0"
    "Le Bourget Aéroport" "\0"
    "Triangle de Gonesse" "\0"
    "Parc des expositions" "\0"
    "Aéroport Charles de Gaulle T2" "\0"
    "Aéroport Charles de Gaulle 2e gare" "\0"
    "Le Mesnil-Amelot" "\0"
    "Versailles Chantiers" "\0"
    "Satory" "\0"
    "Guyancourt" "\0"
    "Christ de Saclay" "\0"
    "Université Paris-Saclay" "\0"
    "Polytechnique" "\0"
    "Massy – Palaiseau" "\0"
    "Massy Opéra" "\0"
    "Antonypole Wissous Centre" "\0"
    "Cergy Le Haut" "\0"
    "Cergy-Saint-Christophe" "\0"
    "Cergy-Préfecture" "\0"
    "Neuville Université" "\0"
    "Conflans-Fin-d’Oise" "\0"
    "Achères-Ville" "\0"
    "Achères-Grand-Cormier" "\0"
    "Maisons-Laffitte" "\0"
    "Sartrouville" "\0"
    "Houilles-Carrières-sur-Seine" "\0"
    "Nanterre-Université" "\0"
    "Nanterre-Préfecture" "\0"
    "Auber" "\0"
    "Châtelet-Les Halles" "\0"
    "Vincennes" "\0"
    "Fontenay-sous-Bois" "\0"
    "Val de Fontenay" "\0"
    "Neuilly-Plaisance" "\0"
    "Bry-sur-Marne" "\0"
    "Noisy-le-Grand-Mont d’Est" "\0"
    "Noisy - Champs" "\0"
    "Noisiel" "\0"
    "Lognes" "\0"
    "Torcy" "\0"
    "Bussy-Saint-Georges" "\0"
    "Val d’Europe" "\0"
    "Marne-la-Vallée-Chessy" "\0"
    "Poissy" "\0"
    "Nanterre-Ville" "\0"
    "Rueil-Malmaison" "\0"
    "Châtou-Croissy" "\0"
    "Le Vésinet-Centre" "\0"
    "Le Vésinet-Le Pecq" "\0"
    "Saint-Germain-en-Laye" "\0"
    "Nogent-sur-Marne" "\0"
    "Joinville-le-Pont" "\0"
    "Saint-Maur-Créteil" "\0"
    "Le Parc de Saint-Maur" "\0"
    "Champigny" "\0"
    "La Varenne-Chennevières" "\0"
    "Sucy-Bonneuil" "\0"
    "Boissy-Saint-Léger" "\0"
    "Saint-Rémy-lès-Chevreuse" "\0"
    "Courcelle-sur-Yvette" "\0"
    "Gif-sur-Yvette" "\0"
    "La Hacquinière" "\0"
    "Bures-sur-Yvette" "\0"
    "Orsay-Ville" "\0"
    "Le Guichet" "\0"
    "Lozère" "\0"
    "Palaiseau-Villebon" "\0"
    "Palaiseau" "\0"
    "Massy-Palaiseau" "\0"
    "Massy-Verrières" "\0"
    "Les Baconnets" "\0"
    "Fontaine-Michalon" "\0"
    "Antony" "\0"
    "La Croix-de-Berny-Fresnes" "\0"
    "Parc de Sceaux" "\0"
    "Bourg-la-Reine" "\0"
    "Bagneux" "\0"
    "Arcueil - Cachan" "\0"
    "Laplace" "\0"
    "Gentilly" "\0"
    "Cité Universitaire" "\0"
    "Port Royal" "\0"
    "Luxembourg" "\0"
    "La Plaine-Stade de France" "\0"
    "La Courneuve-Aubervilliers" "\0"
    "Drancy" "\0"
    "Aulnay-sous-Bois" "\0"
    "Villepinte" "\0"
    "Parc des Expositions" "\0"
    "Aéroport Charles De Gaulle 1" "\0"
    "Aéroport Charles De Gaulle 2-TGV" "\0"
    "Sceaux" "\0"
    "Fontenay-aux-Roses" "\0"
    "Robinson" "\0"
    "Vert-Galant" "\0"
    "Villeparisis-Mitry-le-Neuf" "\0"
    "Mitry-Claye" "\0"
    "Pontoise" "\0"
    "Saint-Ouen-l’Aumône" "\0"
    "Saint-Ouen-l’Aumône-Liesse" "\0"
    "Pierrelaye" "\0"
    "Montigny-Beauchamp" "\0"
    "Franconville-Le Plessis-Bouchard" "\0"
    "Cernay" "\0"
    "Ermont-Eaubonne" "\0"
    "Saint-Gratien" "\0"
    "Épinay-sur-Seine" "\0"
    "Gennevilliers" "\0"
    "Les Grésillons" "\0"
    "Neuilly Porte Maillot" "\0"
    "Avenue Foch" "\0"
    "Avenue Henri Martin" "\0"
    "Boulainvilliers" "\0"
    "Avenue du Président Kennedy" "\0"
    "Javel" "\0"
    "Champ de Mars" "\0"
    "Pont de l’Alma" "\0"
    "Musée d’Orsay" "\0"
    "Paris Austerlitz" "\0"
    "Ivry-sur-Seine" "\0"
    "Vitry-sur-Seine" "\0"
    "Les Ardoines" "\0"
    "Choisy-le-Roi" "\0"
    "Les Saules" "\0"
    "Villeneuve-le-Roi" "\0"
    "Ablon" "\0"
    "Athis-Mons" "\0"
    "Juvisy" "\0"
    "Savigny-sur-Orge" "\0"
    "Épinay-sur-Orge" "\0"
    "Sainte-Geneviève-des-Bois" "\0"
    "Saint-Michel-sur-Orge" "\0"
    "Brétigny" "\0"
    "Marolles-en-Hurepoix" "\0"
    "La Norville" "\0"
    "Arpajon" "\0"
    "Égly" "\0"
    "Breuillet" "\0"
    "Breuillet-Village" "\0"
    "Saint-Chéron" "\0"
    "Sermaise" "\0"
    "Dourdan" "\0"
    "Dourdan-la-Forêt" "\0"
    "Pont du Garigliano" "\0"
    "Issy-Val-de-Seine" "\0"
    "Issy" "\0"
    "Meudon-Val-Fleury" "\0"
    "Chaville-Vélizy" "\0"
    "Viroflay Rive Gauche" "\0"
    "Porchefontaine" "\0"
    "Versailles-Chantiers" "\0"
    "Saint-Cyr" "\0"
    "Saint-Quentin-en-Yvelines" "\0"
    "Orly Ville" "\0"
    "Pont de Rungis" "\0"
    "Rungis-La Fraternelle" "\0"
    "Chemin d’Antony" "\0"
    "Bouray" "\0"
    "Lardy" "\0"
    "Chamarande" "\0"
    "Étréchy" "\0"
    "Étampes" "\0"
    "Saint-Martin-d’Étampes" "\0"
    "Versailles Château" "\0"
    "Creil" "\0"
    "Chantilly-Gouvieux" "\0"
    "Orry-la-Ville-Coye-la-Forêt" "\0"
    "La Borne Blanche" "\0"
    "Survilliers-Fosses" "\0"
    "Louvres" "\0"
    "Les Noues" "\0"
    "Goussainville" "\0"
    "Villiers-le-Bel-Gonesse-Arnouville" "\0"
    "Garges-Sarcelles" "\0"
    "Pierrefitte Stains" "\0"
    "Saint-Denis" "\0"
    "Stade de France-Saint-Denis" "\0"
    "Maisons-Alfort-Alfortville" "\0"
    "Le Vert de Maisons" "\0"
    "Créteil Pompadour" "\0"
    "Villeneuve-Triage" "\0"
    "Villeneuve-Saint-Georges" "\0"
    "Montgeron-Crosne" "\0"
    "Yerres" "\0"
    "Brunoy" "\0"
    "Boussy-Saint-Antoine" "\0"
    "Combs-la-Ville - Quincy" "\0"
    "Lieusaint-Moissy" "\0"
    "Savigny-le-Temple-Nandy" "\0"
    "Cesson" "\0"
    "Le Mée" "\0"
    "Melun" "\0"
    "Vosves" "\0"
    "Boissise-le-Roi" "\0"
    "Ponthierry-Pringy" "\0"
    "Le Coudray-Montceaux" "\0"
    "Le Plessis-Chenet" "\0"
    "Villabé" "\0"
    "Essonnes-Robinson" "\0"
    "Corbeil-Essonnes" "\0"
    "Moulin-Galant" "\0"
    "Mennecy" "\0"
    "Ballancourt" "\0"
    "La Ferté-Alais" "\0"
    "Boutigny" "\0"
    "Maisse" "\0"
    "Buno-Gironville" "\0"
    "Boigneville" "\0"
    "Vigneux-sur-Seine" "\0"
    "Viry-Châtillon" "\0"
    "Ris-Orangis" "\0"
    "Grand-Bourg" "\0"
    "Évry" "\0"
    "Le Bras-de-Fer" "\0"
    "Évry Courcouronnes" "\0"
    "Orangis-Bois de l’Epine" "\0"
    "Grigny-Centre" "\0"
    "Nanterre - La Folie" "\0"
    "Haussmann-Saint-Lazare" "\0"
    "Magenta" "\0"
    "Rosa Parks" "\0"
    "Pantin" "\0"
    "Noisy-le-Sec" "\0"
    "Bondy" "\0"
    "Rosny-sous-Bois" "\0"
    "Nogent-le-Perreux" "\0"
    "Les Boullereaux-Champigny" "\0"
    "Villiers-sur-Marne - Le Plessis-Trévise" "\0"
    "Les Yvris-Noisy-le-Grand" "\0"
    "Émerainville-Pontault-Combault" "\0"
    "Roissy-en-Brie" "\0"
    "Ozoir-la-Ferrière" "\0"
    "Gretz-Armainvilliers" "\0"
    "Tournan" "\0"
    "Le Raincy-Villemomble-Montfermeil" "\0"
    "Gagny" "\0"
    "Le Chénay-Gagny" "\0"
    "Chelles-Gournay" "\0"
    "Asnières Quatre Routes" "\0"
    "Le Luth" "\0"
    "Le Village" "\0"
    "Timbaud" "\0"
    "Parc des Chanteraines" "\0"
    "Chemin des Reniers" "\0"
    "La Noue" "\0"
    "Mairie de Villeneuve-la-Garenne" "\0"
    "L’Ile-Saint-Denis" "\0"
    "Théâtre Gérard Philipe" "\0"
    "Marché de Saint-Denis" "\0"
    "Cimetière de Saint-Denis" "\0"
    "Hôpital Delafontaine" "\0"
    "Cosmonautes" "\0"
    "Six Routes" "\0"
    "Hôtel de Ville de La Courneuve" "\0"
    "Stade Géo André" "\0"
    "Danton" "\0"
    "Maurice Lachâtre" "\0"
    "Drancy-Avenir" "\0"
    "Hôpital Avicenne" "\0"
    "Gaston Roulaud" "\0"
    "Escadrille Normandie-Niémen" "\0"
    "La Ferme" "\0"
    "Libération" "\0"
    "Hôtel de Ville de Bobigny" "\0"
    "Jean Rostand" "\0"
    "Auguste Delaune" "\0"
    "Pont de Bondy" "\0"
    "Petit Noisy" "\0"
    "Pont de Bezons" "\0"
    "Parc Pierre Lagravère" "\0"
    "Victor Basch" "\0"
    "Jacqueline Auriol" "\0"
    "Charlebourg" "\0"
    "Les Fauvelles" "\0"
    "Faubourg de l’Arche" "\0"
    "Puteaux" "\0"
    "Belvédère" "\0"
    "Suresnes-Longchamp" "\0"
    "Les Coteaux" "\0"
    "Les Milons" "\0"
    "Parc de Saint-Cloud" "\0"
    "Musée de Sèvres" "\0"
    "Brimborion" "\0"
    "Meudon-sur-Seine" "\0"
    "Les Moulineaux" "\0"
    "Jacques-Henri Lartigue" "\0"
    "Henri Farman" "\0"
    "Suzanne Lenglen" "\0"
    "Porte d’Issy" "\0"
    "Desnouettes" "\0"
    "Georges Brassens" "\0"
    "Brancion" "\0"
    "Didot" "\0"
    "Jean Moulin" "\0"
    "Montsouris" "\0"
    "Stade Charléty" "\0"
    "Poterne des Peupliers" "\0"
    "Maryse Bastié" "\0"
    "Avenue de France" "\0"
    "Baron Le Roy" "\0"
    "Montempoivre" "\0"
    "Alexandra David-Néel" "\0"
    "Anna de Noailles" "\0"
    "Anny Flore" "\0"
    "Thérèse Pierre" "\0"
    "Square Sainte-Odile" "\0"
    "Porte d’Asnières" "\0"
    "Honoré de Balzac" "\0"
    "Épinettes - Pouchet" "\0"
    "Angélique Compoint" "\0"
    "Diane Arbus" "\0"
    "Colette Besson" "\0"
    "Porte d’Aubervilliers" "\0"
    "Canal Saint-Denis" "\0"
    "Ella Fitzgerald" "\0"
    "Delphine Seyrig" "\0"
    "Butte du Chapeau Rouge" "\0"
    "Hôpital Robert Debré" "\0"
    "Adrienne Bolland" "\0"
    "Séverine" "\0"
    "Marie de Miribel" "\0"
    "Rougemont-Chanteloup" "\0"
    "Freinville-Sevran" "\0"
    "L’Abbaye" "\0"
    "Lycée Henri Sellier" "\0"
    "Gargan" "\0"
    "République Marx Dormoy" "\0"
    "Léon Blum" "\0"
    "Maurice Audin" "\0"
    "Clichy-sous-Bois Mairie" "\0"
    "Romain Rolland" "\0"
    "Clichy - Montfermeil" "\0"
    "Notre-Dame-des-Anges" "\0"
    "Arboretum" "\0"
    "Hôpital de Montfermeil" "\0"
    "Les Pavillons-sous-Bois" "\0"
    "Allée de la Tour Rendez-Vous" "\0"
    "Les Coquetiers" "\0"
    "La Remise à Jorelle" "\0"
    "Lochères" "\0"
    "Paul Valéry" "\0"
    "Les Flanades" "\0"
    "Les Cholettes" "\0"
    "Butte Pinson" "\0"
    "Jacques Prévert" "\0"
    "Alcide d’Orbigny" "\0"
    "Mairie de Pierrefitte" "\0"
    "Suzanne Valadon" "\0"
    "Joncherolles" "\0"
    "Petit Pierrefitte" "\0"
    "Guynemer" "\0"
    "Roger Sémat" "\0"
    "Baudelaire" "\0"
    "Viroflay Rive Droite" "\0"
    "Robert Wagner" "\0"
    "L’Onde" "\0"
    "Mairie de Vélizy" "\0"
    "Louvois" "\0"
    "Inovel Parc Nord" "\0"
    "Dewoitine" "\0"
    "Vélizy 2" "\0"
    "Meudon-la-Forêt" "\0"
    "Georges Millandy" "\0"
    "Georges Pompidou" "\0"
    "Pavé Blanc" "\0"
    "Mail de la Plaine" "\0"
    "Hôpital Béclère" "\0"
    "Soleil Levant" "\0"
    "Division Leclerc" "\0"
    "Parc André Malraux" "\0"
    "Centre de Châtillon" "\0"
    "Vauban" "\0"
    "Lamartine" "\0"
    "Domaine Chérioux" "\0"
    "Moulin Vert" "\0"
    "Bretagne" "\0"
    "Auguste Perret" "\0"
    "La Belle Épine" "\0"
    "Place de la Logistique" "\0"
    "Porte de Rungis" "\0"
    "Saarinen" "\0"
    "Robert Schuman" "\0"
    "La Fraternelle" "\0"
    "Hélène Boucher" "\0"
    "Caroline Aigle" "\0"
    "Coeur d’Orly" "\0"
    "Porte de l’Essonne" "\0"
    "Épinay-Orgemont" "\0"
    "Gilbert Bonnemaison" "\0"
    "Lacépède" "\0"
    "Rose Bertin" "\0"
    "Les Béatus" "\0"
    "Les Mobiles" "\0"
    "Blumenthal" "\0"
    "César" "\0"
    "Delaunay-Belleville" "\0"
    "Paul Éluard" "\0"
    "Pierre De Geyter" "\0"
    "Jean Vilar" "\0"
    "Pablo Neruda" "\0"
    "Villetaneuse-Université" "\0"
    "Châteaudun - Barbès" "\0"
    "Cimetière Parisien d’Ivry" "\0"
    "La Briqueterie" "\0"
    "Germaine Tailleferre" "\0"
    "Beethoven - Concorde" "\0"
    "Musée MAC VAL" "\0"
    "Mairie de Vitry-sur-Seine" "\0"
    "Camille Groult" "\0"
    "Constant Coquelin" "\0"
    "Watteau - Rondenay" "\0"
    "Trois communes" "\0"
    "Verdun Hoche" "\0"
    "Rouget de Lisle" "\0"
    "Carle-Darthe" "\0"
    "Four-Peary" "\0"
    "Christophe Colomb" "\0"
    "Orly - Gaston Viens" "\0"
    "Jardin Parisien" "\0"
    "Le Hameau" "\0"
    "Parc des Sports" "\0"
    "Noveos" "\0"
    "Malabry" "\0"
    "Vallée aux Loups" "\0"
    "Cité-Jardin" "\0"
    "Les Peintres" "\0"
    "Théâtre La Piscine" "\0"
    "Petit-Châtenay" "\0"
    "LaVallée" "\0"
    "La Croix de Berny" "\0"
    "Épinay Villetaneuse" "\0"
    "Villetaneuse - Université" "\0"
    "Stains La Cerisaie" "\0"
    "Dugny - La Courneuve" "\0"
    "Massy Europe" "\0"
    "Champlan" "\0"
    "Longjumeau" "\0"
    "Chilly-Mazarin" "\0"
    "Gravigny-Balizy" "\0"
    "Petit-Vaux" "\0"
    "Parc du Château" "\0"
    "Coteaux de l’Orge" "\0"
    "Amédée Gordini" "\0"
    "Ferme Neuve" "\0"
    "Bois de Saint-Eutrope" "\0"
    "Traité de Rome" "\0"
    "Bois Briard" "\0"
    "Évry - Courcouronnes" "\0"
    "Camp des Loges" "\0"
    "Lisière Pereire" "\0"
    "Fourqueux - Bel Air" "\0"
    "Mareil-Marly" "\0"
    "L’Etang - Les Sablons" "\0"
    "Saint-Nom-la-Bretèche - Forêt-De-Marly" "\0"
    "Noisy-le-Roi" "\0"
    "Bailly" "\0"
    "Allée Royale" "\0"
    "Les Portes de Saint-Cyr" "\0"
    "Esbly" "\0"
    "Montry-Condé" "\0"
    "Couilly-Saint-Germain-Quincy" "\0"
    "Villiers-Montbarbin" "\0"
    "Crécy-la-Chapelle" "\0"
    ;

// zda : zone d'arrêt (MonitoringRef PRIM « STIF:StopArea:SP:<zda>: »)
// name : position du nom dans NET_NAMES ; flags : NET_BRANCH | NET_TERMINUS ;
// pos : place le long de la ligne (branches : depuis leur embranchement) ;
// year : année d'ouverture prévue (0 : en service ; zda 0 : pas encore référencée)
static const NetStop NET_STOPS[] = {
    {470548, 0, 2, 0, 0},  // La Défense
    {42962, 12, 0, 1, 0},  // Esplanade de la Défense
    {44479, 37, 0, 2, 0},  // Pont de Neuilly
    {43785, 53, 0, 3, 0},  // Les Sablons
    {44533, 65, 0, 4, 0},  // Porte Maillot
    {42213, 79, 0, 5, 0},  // Argentine
    {477682, 89, 0, 6, 0},  // Charles De Gaulle-Étoile
    {43283, 115, 0, 7, 0},  // George V
    {43026, 124, 0, 8, 0},  // Franklin D. Roosevelt
    {42531, 146, 0, 9, 0},  // Champs-Élysées – Clemenceau
    {45676, 178, 0, 10, 0},  // Concorde
    {45664, 187, 0, 11, 0},  // Tuileries
    {44236, 197, 0, 12, 0},  // Palais Royal - Musée du Louvre
    {44645, 229, 0, 13, 0},  // Louvre-Rivoli
    {42587, 243, 0, 14, 0},  // Châtelet
    {470524, 253, 0, 15, 0},  // Hôtel de Ville
    {44881, 269, 0, 16, 0},  // Saint-Paul
    {42288, 280, 0, 17, 0},  // Bastille
    {43153, 289, 0, 18, 0},  // Gare de Lyon
    {44637, 302, 0, 19, 0},  // Reuilly-Diderot
    {462358, 318, 0, 20, 0},  // Nation
    {44528, 325, 0, 21, 0},  // Porte de Vincennes
    {47247, 344, 0, 22, 0},  // Saint-Mandé
    {42318, 357, 0, 23, 0},  // Bérault
    {42582, 366, 2, 24, 0},  // Château de Vincennes
    {44509, 388, 2, 0, 0},  // Porte Dauphine
    {45020, 403, 0, 1, 0},  // Victor Hugo
    {477682, 89, 0, 2, 0},  // Charles De Gaulle-Étoile
    {44931, 415, 0, 3, 0},  // Ternes
    {42771, 422, 0, 4, 0},  // Courcelles
    {44146, 433, 0, 5, 0},  // Monceau
    {45056, 441, 0, 6, 0},  // Villiers
    {44662, 450, 0, 7, 0},  // Rome
    {44372, 455, 0, 8, 0},  // Place de Clichy
    {42337, 471, 0, 9, 0},  // Blanche
    {44345, 479, 0, 10, 0},  // Pigalle
    {42210, 487, 0, 11, 0},  // Anvers
    {42280, 494, 0, 12, 0},  // Barbès-Rochechouart
    {43524, 515, 0, 13, 0},  // La Chapelle
    {44901, 527, 0, 14, 0},  // Stalingrad
    {43425, 538, 0, 15, 0},  // Jaurès
    {42732, 546, 0, 16, 0},  // Colonel Fabien
    {42311, 561, 0, 17, 0},  // Belleville
    {42772, 572, 0, 18, 0},  // Couronnes
    {44133, 582, 0, 19, 0},  // Ménilmontant
    {44310, 596, 0, 20, 0},  // Père Lachaise
    {44332, 611, 0, 21, 0},  // Philippe Auguste
    {45196, 628, 0, 22, 0},  // Alexandre Dumas
    {42271, 644, 0, 23, 0},  // Avron
    {462358, 318, 2, 24, 0},  // Nation
    {473676, 650, 2, 0, 0},  // Pont de Levallois
    {42195, 668, 0, 1, 0},  // Anatole France
    {51613, 683, 0, 2, 0},  // Louise Michel
    {44511, 697, 0, 3, 0},  // Porte de Champerret
    {462376, 717, 0, 4, 0},  // Pereire-Levallois
    {45508, 735, 0, 5, 0},  // Wagram
    {59370, 742, 0, 6, 0},  // Malesherbes
    {45056, 441, 0, 7, 0},  // Villiers
    {42972, 754, 0, 8, 0},  // Europe
    {462374, 761, 0, 9, 0},  // Saint-Lazare
    {462375, 774, 0, 10, 0},  // Havre-Caumartin
    {42237, 790, 0, 11, 0},  // Opéra
    {44587, 797, 0, 12, 0},  // Quatre Septembre
    {45101, 814, 0, 13, 0},  // Bourse
    {44825, 821, 0, 14, 0},  // Sentier
    {44600, 829, 0, 15, 0},  // Réaumur Sébastopol
    {42230, 850, 0, 16, 0},  // Arts et Métiers
    {44924, 867, 0, 17, 0},  // Temple
    {44617, 874, 0, 18, 0},  // République
    {58725, 886, 0, 19, 0},  // Parmentier
    {45194, 897, 0, 20, 0},  // Rue Saint-Maur
    {44310, 596, 0, 21, 0},  // Père Lachaise
    {43055, 912, 0, 22, 0},  // Gambetta
    {44510, 921, 0, 23, 0},  // Porte de Bagnolet
    {47890, 939, 2, 24, 0},  // Gallieni
    {44529, 948, 2, 0, 0},  // Porte des Lilas
    {44864, 964, 0, 1, 0},  // Saint-Fargeau
    {45630, 978, 0, 2, 0},  // Pelleport
    {43055, 912, 2, 3, 0},  // Gambetta
    {44515, 988, 2, 0, 0},  // Porte de Clignancourt
    {44833, 1010, 0, 1, 0},  // Simplon
    {44075, 1018, 0, 2, 0},  // Marcadet-Poissonniers
    {42585, 1040, 0, 3, 0},  // Château Rouge
    {42280, 494, 0, 4, 0},  // Barbès-Rochechouart
    {462941, 1055, 0, 5, 0},  // Gare du Nord
    {43138, 1068, 0, 6, 0},  // Gare de l’Est
    {42579, 1084, 0, 7, 0},  // Château d’Eau
    {44909, 1101, 0, 8, 0},  // Strasbourg-Saint-Denis
    {44600, 829, 0, 9, 0},  // Réaumur Sébastopol
    {44970, 1124, 0, 10, 0},  // Étienne Marcel
    {417675, 1140, 0, 11, 0},  // Les Halles
    {42587, 243, 0, 12, 0},  // Châtelet
    {42639, 1151, 0, 13, 0},  // Cité
    {462369, 1157, 0, 14, 0},  // Saint-Michel
    {58716, 1170, 0, 15, 0},  // Odéon
    {44799, 1177, 0, 16, 0},  // Saint-Germain-des-Prés
    {45120, 1201, 0, 17, 0},  // Saint-Sulpice
    {44885, 1215, 0, 18, 0},  // Saint-Placide
    {58578, 1229, 0, 19, 0},  // Montparnasse-Bienvenüe
    {44997, 1253, 0, 20, 0},  // Vavin
    {45314, 1259, 0, 21, 0},  // Raspail
    {462351, 1267, 0, 22, 0},  // Denfert-Rochereau
    {44183, 1285, 0, 23, 0},  // Mouton-Duvernet
    {42175, 1301, 0, 24, 0},  // Alésia
    {44508, 1309, 0, 25, 0},  // Porte d’Orléans
    {45904, 1328, 2, 26, 0},  // Mairie de Montrouge
    {483278, 1348, 0, 27, 0},  // Barbara
    {483280, 1356, 2, 28, 0},  // Bagneux - Lucie Aubrac
    {42345, 1379, 2, 0, 0},  // Bobigny Pablo Picasso
    {42344, 1401, 0, 1, 0},  // Bobigny-Pantin - Raymond Queneau
    {42949, 1434, 0, 2, 0},  // Église de Pantin
    {43364, 1452, 0, 3, 0},  // Hoche
    {44521, 1458, 0, 4, 0},  // Porte de Pantin
    {44227, 1474, 0, 5, 0},  // Ourcq
    {45581, 1480, 0, 6, 0},  // Laumière
    {43425, 538, 0, 7, 0},  // Jaurès
    {44901, 527, 0, 8, 0},  // Stalingrad
    {462941, 1055, 0, 9, 0},  // Gare du Nord
    {43138, 1068, 0, 10, 0},  // Gare de l’Est
    {43417, 1490, 0, 11, 0},  // Jacques Bonsergent
    {44617, 874, 0, 12, 0},  // République
    {45193, 1509, 0, 13, 0},  // Oberkampf
    {44641, 1519, 0, 14, 0},  // Richard Lenoir
    {42390, 1534, 0, 15, 0},  // Bréguet-Sabin
    {42288, 280, 0, 16, 0},  // Bastille
    {44583, 1549, 0, 17, 0},  // Quai de la Rapée
    {462352, 1567, 0, 18, 0},  // Gare d’Austerlitz
    {44875, 1587, 0, 19, 0},  // Saint-Marcel
    {42426, 1600, 0, 20, 0},  // Campo Formio
    {44371, 1613, 2, 21, 0},  // Place d’Italie
    {477682, 89, 2, 0, 0},  // Charles De Gaulle-Étoile
    {43498, 1630, 0, 1, 0},  // Kléber
    {42359, 1638, 0, 2, 0},  // Boissière
    {44962, 1649, 0, 3, 0},  // Trocadéro
    {45417, 1660, 0, 4, 0},  // Passy
    {45340, 1666, 0, 5, 0},  // Bir-Hakeim
    {42871, 1677, 0, 6, 0},  // Dupleix
    {43566, 1685, 0, 7, 0},  // La Motte Picquet-Grenelle
    {42419, 1711, 0, 8, 0},  // Cambronne
    {44829, 1721, 0, 9, 0},  // Sèvres-Lecourbe
    {44273, 1738, 0, 10, 0},  // Pasteur
    {58578, 1229, 0, 11, 0},  // Montparnasse-Bienvenüe
    {45313, 1746, 0, 12, 0},  // Edgar Quinet
    {45314, 1259, 0, 13, 0},  // Raspail
    {462351, 1267, 0, 14, 0},  // Denfert-Rochereau
    {45315, 1759, 0, 15, 0},  // Saint-Jacques
    {45276, 1773, 0, 16, 0},  // Glacière
    {45274, 1783, 0, 17, 0},  // Corvisart
    {44371, 1613, 0, 18, 0},  // Place d’Italie
    {44192, 1793, 0, 19, 0},  // Nationale
    {45267, 1803, 0, 20, 0},  // Chevaleret
    {44582, 1814, 0, 21, 0},  // Quai de la Gare
    {58728, 1830, 0, 22, 0},  // Bercy
    {42869, 1836, 0, 23, 0},  // Dugommier
    {45229, 1846, 0, 24, 0},  // Daumesnil
    {45222, 1856, 0, 25, 0},  // Bel-Air
    {44334, 1864, 0, 26, 0},  // Picpus
    {462358, 318, 2, 27, 0},  // Nation
    {470546, 1871, 2, 0, 0},  // La Courneuve 8 Mai 1945
    {47891, 1895, 0, 1, 0},  // Fort d’Aubervilliers
    {46492, 1918, 0, 2, 0},  // Aubervilliers Pantin - Quatre Chemins
    {473461, 1956, 0, 3, 0},  // Porte de la Villette
    {45598, 1977, 0, 4, 0},  // Corentin Cariou
    {42780, 1993, 0, 5, 0},  // Crimée
    {44643, 2001, 0, 6, 0},  // Riquet
    {44901, 527, 0, 7, 0},  // Stalingrad
    {43823, 2008, 0, 8, 0},  // Louis Blanc
    {479053, 2020, 0, 9, 0},  // Château-Landon
    {43138, 1068, 0, 10, 0},  // Gare de l’Est
    {45165, 2036, 0, 11, 0},  // Poissonnière
    {42414, 2050, 0, 12, 0},  // Cadet
    {43656, 2056, 0, 13, 0},  // Le Peletier
    {42589, 2068, 0, 14, 0},  // Chaussée d’Antin - La Fayette
    {42237, 790, 0, 15, 0},  // Opéra
    {44579, 2101, 0, 16, 0},  // Pyramides
    {44236, 197, 0, 17, 0},  // Palais Royal - Musée du Louvre
    {44491, 2111, 0, 18, 0},  // Pont Neuf
    {42587, 243, 0, 19, 0},  // Châtelet
    {44487, 2121, 0, 20, 0},  // Pont Marie
    {44911, 2132, 0, 21, 0},  // Sully-Morland
    {43493, 2146, 0, 22, 0},  // Jussieu
    {44435, 2154, 0, 23, 0},  // Place Monge
    {42461, 2166, 0, 24, 0},  // Censier-Daubenton
    {43737, 2184, 0, 25, 0},  // Les Gobelins
    {44371, 1613, 0, 26, 0},  // Place d’Italie
    {43411, 2197, 0, 27, 0},  // Tolbiac
    {44057, 2205, 0, 28, 0},  // Maison Blanche
    {473456, 2220, 0, 29, 0},  // Porte d’Italie
    {43643, 2237, 0, 30, 0},  // Le Kremlin-Bicêtre
    {47826, 2257, 0, 31, 0},  // Villejuif - Léo Lagrange
    {58471, 2283, 0, 32, 0},  // Villejuif Paul Vaillant-Couturier
    {45049, 2317, 2, 33, 0},  // Villejuif - Louis Aragon
    {44513, 2342, 1, 30, 0},  // Porte de Choisy
    {44507, 2358, 0, 31, 0},  // Porte d’Ivry
    {44343, 2373, 0, 32, 0},  // Pierre et Marie Curie
    {44051, 2395, 2, 33, 0},  // Mairie d’Ivry
    {43823, 2008, 2, 0, 0},  // Louis Blanc
    {43425, 538, 0, 1, 0},  // Jaurès
    {58767, 2411, 0, 2, 0},  // Bolivar
    {45609, 2419, 0, 3, 0},  // Buttes-Chaumont
    {42369, 2435, 0, 4, 0},  // Botzaris
    {42822, 2444, 0, 5, 0},  // Danube
    {45617, 2451, 2, 6, 0},  // Pré Saint-Gervais
    {44396, 2470, 0, 7, 0},  // Place des Fêtes
    {463354, 2487, 2, 0, 0},  // Balard
    {45363, 2494, 0, 1, 0},  // Lourmel
    {42370, 2502, 0, 2, 0},  // Boucicaut
    {42984, 2512, 0, 3, 0},  // Félix Faure
    {42734, 2525, 0, 4, 0},  // Commerce
    {43566, 1685, 0, 5, 0},  // La Motte Picquet-Grenelle
    {42885, 2534, 0, 6, 0},  // École Militaire
    {43409, 2551, 0, 7, 0},  // La Tour Maubourg
    {43407, 2568, 0, 8, 0},  // Invalides
    {45676, 178, 0, 9, 0},  // Concorde
    {43898, 2578, 0, 10, 0},  // Madeleine
    {42237, 790, 0, 11, 0},  // Opéra
    {44642, 2588, 0, 12, 0},  // Richelieu-Drouot
    {43310, 2605, 0, 13, 0},  // Grands Boulevards
    {45100, 2623, 0, 14, 0},  // Bonne Nouvelle
    {44909, 1101, 0, 15, 0},  // Strasbourg-Saint-Denis
    {44617, 874, 0, 16, 0},  // République
    {58727, 2638, 0, 17, 0},  // Filles du Calvaire
    {45208, 2657, 0, 18, 0},  // Saint-Sébastien-Froissart
    {45097, 2684, 0, 19, 0},  // Chemin Vert
    {42288, 280, 0, 20, 0},  // Bastille
    {43674, 2696, 0, 21, 0},  // Ledru Rollin
    {42980, 2709, 0, 22, 0},  // Faidherbe Chaligny
    {44637, 302, 0, 23, 0},  // Reuilly-Diderot
    {44160, 2728, 0, 24, 0},  // Montgallet
    {45229, 1846, 0, 25, 0},  // Daumesnil
    {44140, 2739, 0, 26, 0},  // Michel Bizot
    {44532, 2752, 0, 27, 0},  // Porte Dorée
    {44512, 2765, 0, 28, 0},  // Porte de Charenton
    {43811, 2784, 0, 29, 0},  // Liberté
    {42545, 2793, 0, 30, 0},  // Charenton-Écoles
    {42889, 2811, 0, 31, 0},  // École Vétérinaire de Maisons-Alfort
    {44066, 2850, 0, 32, 0},  // Maisons-Alfort-Stade
    {44067, 2871, 0, 33, 0},  // Maisons-Alfort-Les Juilliottes
    {45712, 2902, 0, 34, 0},  // Créteil–L’Échat
    {42779, 2924, 0, 35, 0},  // Créteil-Université
    {42778, 2945, 0, 36, 0},  // Créteil–Préfecture
    {44453, 2968, 2, 37, 0},  // Créteil - Pointe du Lac
    {44481, 2993, 2, 0, 0},  // Pont de Sèvres
    {42334, 3009, 0, 1, 0},  // Billancourt
    {47021, 3021, 0, 2, 0},  // Marcel Sembat
    {44524, 3035, 0, 3, 0},  // Porte de Saint-Cloud
    {42977, 3056, 0, 4, 0},  // Exelmans
    {44139, 3065, 0, 5, 0},  // Michel Ange-Molitor
    {44138, 3085, 0, 6, 0},  // Michel Ange-Auteuil
    {43423, 3105, 0, 7, 0},  // Jasmin
    {44594, 3112, 0, 8, 0},  // Ranelagh
    {43567, 3121, 0, 9, 0},  // La Muette
    {44461, 3131, 0, 10, 0},  // Rue de la Pompe
    {44962, 1649, 0, 11, 0},  // Trocadéro
    {43395, 3147, 0, 12, 0},  // Iéna
    {42182, 3153, 0, 13, 0},  // Alma-Marceau
    {43026, 124, 0, 14, 0},  // Franklin D. Roosevelt
    {44884, 3166, 0, 15, 0},  // Saint-Philippe du Roule
    {43511, 3190, 0, 16, 0},  // Miromesnil
    {44857, 3201, 0, 17, 0},  // Saint-Augustin
    {462375, 774, 0, 18, 0},  // Havre-Caumartin
    {42589, 2068, 0, 19, 0},  // Chaussée d’Antin - La Fayette
    {44642, 2588, 0, 20, 0},  // Richelieu-Drouot
    {43310, 2605, 0, 21, 0},  // Grands Boulevards
    {45100, 2623, 0, 22, 0},  // Bonne Nouvelle
    {44909, 1101, 0, 23, 0},  // Strasbourg-Saint-Denis
    {44617, 874, 0, 24, 0},  // République
    {45193, 1509, 0, 25, 0},  // Oberkampf
    {45205, 3216, 0, 26, 0},  // Saint-Ambroise
    {45065, 3231, 0, 27, 0},  // Voltaire
    {42560, 3240, 0, 28, 0},  // Charonne
    {425987, 3249, 0, 29, 0},  // Rue des Boulets
    {462358, 318, 0, 30, 0},  // Nation
    {42410, 3265, 0, 31, 0},  // Buzenval
    {44072, 3274, 0, 32, 0},  // Maraîchers
    {44520, 3286, 0, 33, 0},  // Porte de Montreuil
    {44650, 3305, 0, 34, 0},  // Robespierre
    {42789, 3317, 0, 35, 0},  // Croix de Chavaux
    {44052, 3334, 2, 36, 0},  // Mairie de Montreuil
    {42947, 3354, 0, 0, 0},  // Église d’Auteuil
    {44138, 3085, 0, 1, 0},  // Michel Ange-Auteuil
    {44504, 3374, 0, 2, 0},  // Porte d’Auteuil
    {42380, 3392, 0, 3, 0},  // Boulogne - Jean Jaurès
    {44139, 3065, 0, 4, 0},  // Michel Ange-Molitor
    {42543, 3416, 0, 5, 0},  // Chardon Lagache
    {45430, 3432, 0, 6, 0},  // Mirabeau
    {462371, 3441, 0, 7, 0},  // Javel - André Citroën
    {42557, 3465, 0, 8, 0},  // Charles Michels
    {42265, 3481, 0, 9, 0},  // Avenue Émile Zola
    {43566, 1685, 0, 10, 0},  // La Motte Picquet-Grenelle
    {51612, 3500, 0, 11, 0},  // Ségur
    {42874, 3507, 0, 12, 0},  // Duroc
    {45116, 3513, 0, 13, 0},  // Vaneau
    {44828, 3520, 0, 14, 0},  // Sèvres-Babylone
    {59379, 3537, 0, 15, 0},  // Mabillon
    {58716, 1170, 0, 16, 0},  // Odéon
    {42674, 3546, 0, 17, 0},  // Cluny-La Sorbonne
    {44119, 3564, 0, 18, 0},  // Maubert-Mutualité
    {42435, 3583, 0, 19, 0},  // Cardinal Lemoine
    {43493, 2146, 0, 20, 0},  // Jussieu
    {462352, 1567, 2, 21, 0},  // Gare d’Austerlitz
    {42381, 3600, 3, 2, 0},  // Boulogne - Pont de Saint-Cloud
    {42587, 243, 2, 0, 0},  // Châtelet
    {470524, 253, 0, 1, 0},  // Hôtel de Ville
    {44593, 3631, 0, 2, 0},  // Rambuteau
    {42230, 850, 0, 3, 0},  // Arts et Métiers
    {44617, 874, 0, 4, 0},  // République
    {43298, 3641, 0, 5, 0},  // Goncourt
    {42311, 561, 0, 6, 0},  // Belleville
    {44580, 3650, 0, 7, 0},  // Pyrénées
    {43483, 3661, 0, 8, 0},  // Jourdain
    {44396, 2470, 0, 9, 0},  // Place des Fêtes
    {44923, 3670, 0, 10, 0},  // Télégraphe
    {44529, 948, 0, 11, 0},  // Porte des Lilas
    {44055, 3683, 0, 12, 0},  // Mairie des Lilas
    {490763, 3700, 0, 13, 0},  // Serge Gainsbourg
    {490765, 3717, 0, 14, 0},  // Romainville - Carnot
    {490767, 3738, 0, 15, 0},  // Montreuil - Hôpital
    {490769, 3759, 0, 16, 0},  // La Dhuys
    {490771, 3768, 0, 17, 0},  // Coteaux Beauclair
    {490773, 3786, 2, 18, 0},  // Rosny-Bois-Perrier
    {483899, 3805, 2, 0, 0},  // Mairie d’Aubervilliers
    {483901, 3830, 0, 1, 0},  // Aimé Césaire
    {58497, 3845, 0, 2, 0},  // Front Populaire
    {473459, 3861, 0, 3, 0},  // Porte de la Chapelle
    {44113, 3882, 0, 4, 0},  // Marx Dormoy
    {44075, 1018, 0, 5, 0},  // Marcadet-Poissonniers
    {479055, 3894, 0, 6, 0},  // Jules Joffrin
    {43600, 3908, 0, 7, 0},  // Lamarck-Caulaincourt
    {42155, 3929, 0, 8, 0},  // Abbesses
    {44345, 479, 0, 9, 0},  // Pigalle
    {44867, 3938, 0, 10, 0},  // Saint-Georges
    {44210, 3952, 0, 11, 0},  // Notre-Dame-de-Lorette
    {45170, 3974, 0, 12, 0},  // Trinité d’Estienne d’Orves
    {462374, 761, 0, 13, 0},  // Saint-Lazare
    {43898, 2578, 0, 14, 0},  // Madeleine
    {45676, 178, 0, 15, 0},  // Concorde
    {45704, 4006, 0, 16, 0},  // Assemblée Nationale
    {44840, 4027, 0, 17, 0},  // Solférino
    {58717, 4038, 0, 18, 0},  // Rue du Bac
    {44828, 3520, 0, 19, 0},  // Sèvres-Babylone
    {44609, 4049, 0, 20, 0},  // Rennes
    {44211, 4056, 0, 21, 0},  // Notre-Dame-des-Champs
    {58578, 4078, 0, 22, 0},  // Montparnasse Bienvenüe
    {42981, 4102, 0, 23, 0},  // Falguière
    {44273, 1738, 0, 24, 0},  // Pasteur
    {45060, 4113, 0, 25, 0},  // Volontaires
    {44995, 4125, 0, 26, 0},  // Vaugirard
    {42751, 4135, 0, 27, 0},  // Convention
    {49515, 4146, 0, 28, 0},  // Porte de Versailles
    {42759, 4166, 0, 29, 0},  // Corentin Celton
    {44050, 4182, 2, 30, 0},  // Mairie d’Issy
    {42231, 4198, 2, 0, 0},  // Les Courtilles
    {43693, 4213, 0, 1, 0},  // Les Agnettes
    {43042, 4226, 0, 2, 0},  // Gabriel Péri
    {44012, 4240, 0, 3, 0},  // Mairie de Clichy
    {473477, 4257, 0, 4, 0},  // Porte de Clichy
    {45487, 4273, 0, 5, 0},  // Brochant
    {43548, 4282, 0, 6, 0},  // La Fourche
    {44372, 455, 0, 7, 0},  // Place de Clichy
    {43814, 4293, 0, 8, 0},  // Liège
    {462374, 761, 0, 9, 0},  // Saint-Lazare
    {43511, 3190, 0, 10, 0},  // Miromesnil
    {42531, 146, 0, 11, 0},  // Champs-Élysées – Clemenceau
    {43407, 2568, 0, 12, 0},  // Invalides
    {45131, 4300, 0, 13, 0},  // Varenne
    {45139, 4308, 0, 14, 0},  // Saint-François-Xavier
    {42874, 3507, 0, 15, 0},  // Duroc
    {58578, 1229, 0, 16, 0},  // Montparnasse-Bienvenüe
    {43046, 4331, 0, 17, 0},  // Gaîté
    {44316, 4339, 0, 18, 0},  // Pernety
    {45322, 4347, 0, 19, 0},  // Plaisance
    {44526, 4357, 0, 20, 0},  // Porte de Vanves
    {413433, 4373, 0, 21, 0},  // Malakoff - Plateau de Vanves
    {44069, 4402, 0, 22, 0},  // Malakoff - Rue Étienne Dolet
    {42588, 4432, 2, 23, 0},  // Châtillon-Montrouge
    {43333, 4453, 1, 5, 0},  // Guy Môquet
    {44525, 4465, 0, 4, 0},  // Porte de Saint-Ouen
    {43252, 4485, 0, 3, 0},  // Garibaldi
    {43969, 4495, 0, 2, 0},  // Mairie de Saint-Ouen
    {42447, 4516, 0, 1, 0},  // Carrefour Pleyel
    {473458, 4533, 0, 0, 0},  // Saint-Denis - Porte de Paris
    {463329, 4562, 0, -1, 0},  // Basilique de Saint-Denis
    {44860, 4587, 2, -2, 0},  // Saint-Denis - Université
    {490775, 4613, 0, 0, 0},  // Saint-Denis Pleyel
    {43969, 4495, 0, 1, 0},  // Mairie de Saint-Ouen
    {480289, 4632, 0, 2, 0},  // Saint-Ouen
    {473477, 4257, 0, 3, 0},  // Porte de Clichy
    {480287, 4643, 0, 4, 0},  // Pont Cardinet
    {462374, 761, 0, 5, 0},  // Saint-Lazare
    {43898, 2578, 0, 6, 0},  // Madeleine
    {44579, 2101, 0, 7, 0},  // Pyramides
    {42587, 243, 0, 8, 0},  // Châtelet
    {43153, 289, 0, 9, 0},  // Gare de Lyon
    {58728, 1830, 0, 10, 0},  // Bercy
    {45234, 4657, 0, 11, 0},  // Cour Saint-Émilion
    {473111, 4677, 0, 12, 0},  // Bibliothèque François Mitterrand
    {479056, 4712, 2, 13, 0},  // Olympiades
    {44057, 2205, 0, 14, 0},  // Maison Blanche
    {490840, 4723, 0, 15, 0},  // Hôpital Bicêtre
    {490907, 4741, 0, 16, 0},  // Villejuif - Gustave Roussy
    {490838, 4768, 0, 17, 0},  // L’Haÿ-les-Roses
    {490836, 4787, 0, 18, 0},  // Chevilly-Larue
    {490834, 4802, 0, 19, 0},  // Thiais - Orly
    {490909, 4816, 2, 20, 0},  // Aéroport d’Orly
    {0, 4613, 2, 0, 2027},  // Saint-Denis Pleyel
    {0, 4835, 0, 1, 2027},  // La Courneuve-Six Routes
    {0, 4859, 0, 2, 2027},  // Le Bourget
    {0, 4870, 0, 3, 2027},  // Le Blanc-Mesnil
    {0, 4886, 0, 4, 2027},  // Aulnay
    {0, 4893, 0, 5, 2027},  // Sevran-Beaudottes
    {0, 4911, 0, 6, 2027},  // Sevran-Livry
    {0, 4924, 0, 7, 2027},  // Clichy-Montfermeil
    {0, 4943, 0, 8, 2028},  // Chelles Gournay
    {0, 4959, 2, 9, 2028},  // Noisy-Champs
    {0, 4613, 2, 0, 2027},  // Saint-Denis Pleyel
    {0, 4835, 0, 1, 2027},  // La Courneuve-Six Routes
    {0, 4859, 0, 2, 2027},  // Le Bourget
    {0, 4972, 0, 3, 2027},  // Le Bourget Aéroport
    {0, 4993, 0, 4, 2028},  // Triangle de Gonesse
    {0, 5013, 0, 5, 2028},  // Parc des expositions
    {0, 5034, 0, 6, 2028},  // Aéroport Charles de Gaulle T2
    {0, 5065, 0, 7, 2030},  // Aéroport Charles de Gaulle 2e gare
    {0, 5101, 2, 8, 2030},  // Le Mesnil-Amelot
    {0, 5118, 2, 0, 2030},  // Versailles Chantiers
    {0, 5139, 0, 1, 2030},  // Satory
    {0, 5146, 0, 2, 2030},  // Guyancourt
    {494279, 5157, 0, 3, 0},  // Christ de Saclay
    {494277, 5174, 0, 4, 0},  // Université Paris-Saclay
    {494275, 5199, 0, 5, 0},  // Polytechnique
    {494273, 5213, 0, 6, 0},  // Massy – Palaiseau
    {0, 5233, 0, 7, 2027},  // Massy Opéra
    {0, 5246, 0, 8, 2027},  // Antonypole Wissous Centre
    {0, 4816, 2, 9, 2027},  // Aéroport d’Orly
    {43104, 5272, 2, 0, 0},  // Cergy Le Haut
    {47897, 5286, 0, 1, 0},  // Cergy-Saint-Christophe
    {44559, 5309, 0, 2, 0},  // Cergy-Préfecture
    {47879, 5327, 0, 3, 0},  // Neuville Université
    {43114, 5348, 0, 4, 0},  // Conflans-Fin-d’Oise
    {46647, 5370, 0, 5, 0},  // Achères-Ville
    {47915, 5385, 0, 6, 0},  // Achères-Grand-Cormier
    {473109, 5408, 0, 7, 0},  // Maisons-Laffitte
    {43191, 5425, 0, 8, 0},  // Sartrouville
    {43082, 5438, 0, 9, 0},  // Houilles-Carrières-sur-Seine
    {43170, 5468, 0, 10, 0},  // Nanterre-Université
    {43169, 5489, 0, 11, 0},  // Nanterre-Préfecture
    {470549, 0, 0, 12, 0},  // La Défense
    {58759, 89, 0, 13, 0},  // Charles De Gaulle-Étoile
    {45873, 5510, 0, 14, 0},  // Auber
    {45102, 5516, 0, 15, 0},  // Châtelet-Les Halles
    {470195, 289, 0, 16, 0},  // Gare de Lyon
    {473875, 318, 0, 17, 0},  // Nation
    {43224, 5537, 0, 18, 0},  // Vincennes
    {47238, 5547, 0, 19, 0},  // Fontenay-sous-Bois
    {47900, 5566, 0, 20, 0},  // Val de Fontenay
    {43172, 5582, 0, 21, 0},  // Neuilly-Plaisance
    {43101, 5600, 0, 22, 0},  // Bry-sur-Marne
    {474082, 5614, 0, 23, 0},  // Noisy-le-Grand-Mont d’Est
    {58937, 5642, 0, 24, 0},  // Noisy - Champs
    {44199, 5657, 0, 25, 0},  // Noisiel
    {43152, 5665, 0, 26, 0},  // Lognes
    {43207, 5672, 0, 27, 0},  // Torcy
    {58874, 5678, 0, 28, 0},  // Bussy-Saint-Georges
    {43213, 5698, 0, 29, 0},  // Val d’Europe
    {43239, 5713, 2, 30, 0},  // Marne-la-Vallée-Chessy
    {47874, 5737, 3, 5, 0},  // Poissy
    {43171, 5744, 1, 9, 0},  // Nanterre-Ville
    {58875, 5759, 0, 8, 0},  // Rueil-Malmaison
    {53783, 5775, 0, 7, 0},  // Châtou-Croissy
    {53784, 5791, 0, 6, 0},  // Le Vésinet-Centre
    {43237, 5810, 0, 5, 0},  // Le Vésinet-Le Pecq
    {43198, 5830, 2, 4, 0},  // Saint-Germain-en-Laye
    {47886, 5852, 1, 20, 0},  // Nogent-sur-Marne
    {43135, 5869, 0, 21, 0},  // Joinville-le-Pont
    {44801, 5887, 0, 22, 0},  // Saint-Maur-Créteil
    {43234, 5907, 0, 23, 0},  // Le Parc de Saint-Maur
    {58270, 5929, 0, 24, 0},  // Champigny
    {43146, 5939, 0, 25, 0},  // La Varenne-Chennevières
    {58792, 5964, 0, 26, 0},  // Sucy-Bonneuil
    {43094, 5978, 2, 27, 0},  // Boissy-Saint-Léger
    {47889, 5998, 2, 0, 0},  // Saint-Rémy-lès-Chevreuse
    {47052, 6025, 0, 1, 0},  // Courcelle-sur-Yvette
    {47888, 6046, 0, 2, 0},  // Gif-sur-Yvette
    {47046, 6061, 0, 3, 0},  // La Hacquinière
    {43103, 6077, 0, 4, 0},  // Bures-sur-Yvette
    {43086, 6094, 0, 5, 0},  // Orsay-Ville
    {43232, 6106, 0, 6, 0},  // Le Guichet
    {474069, 6117, 0, 7, 0},  // Lozère
    {43175, 6125, 0, 8, 0},  // Palaiseau-Villebon
    {47009, 6144, 0, 9, 0},  // Palaiseau
    {58774, 6154, 0, 10, 0},  // Massy-Palaiseau
    {47940, 6170, 0, 11, 0},  // Massy-Verrières
    {43228, 6187, 0, 12, 0},  // Les Baconnets
    {43124, 6201, 0, 13, 0},  // Fontaine-Michalon
    {43066, 6219, 0, 14, 0},  // Antony
    {46007, 6226, 0, 15, 0},  // La Croix-de-Berny-Fresnes
    {43177, 6252, 0, 16, 0},  // Parc de Sceaux
    {43097, 6267, 0, 17, 0},  // Bourg-la-Reine
    {44493, 6282, 0, 18, 0},  // Bagneux
    {43067, 6290, 0, 19, 0},  // Arcueil - Cachan
    {43607, 6307, 0, 20, 0},  // Laplace
    {45877, 6315, 0, 21, 0},  // Gentilly
    {473843, 6324, 0, 22, 0},  // Cité Universitaire
    {473890, 1267, 0, 23, 0},  // Denfert-Rochereau
    {44500, 6344, 0, 24, 0},  // Port Royal
    {43833, 6355, 0, 25, 0},  // Luxembourg
    {44877, 1157, 0, 26, 0},  // Saint-Michel
    {45102, 5516, 0, 27, 0},  // Châtelet-Les Halles
    {462394, 1055, 0, 28, 0},  // Gare du Nord
    {43145, 6366, 0, 29, 0},  // La Plaine-Stade de France
    {43140, 6392, 0, 30, 0},  // La Courneuve-Aubervilliers
    {43231, 4859, 0, 31, 0},  // Le Bourget
    {43122, 6419, 0, 32, 0},  // Drancy
    {46163, 4870, 0, 33, 0},  // Le Blanc-Mesnil
    {43071, 6426, 0, 34, 0},  // Aulnay-sous-Bois
    {43194, 4911, 0, 35, 0},  // Sevran-Livry
    {43193, 4893, 0, 36, 0},  // Sevran-Beaudottes
    {58793, 6443, 0, 37, 0},  // Villepinte
    {47878, 6454, 0, 38, 0},  // Parc des Expositions
    {462398, 6475, 0, 39, 0},  // Aéroport Charles De Gaulle 1
    {473364, 6505, 2, 40, 0},  // Aéroport Charles De Gaulle 2-TGV
    {59206, 6539, 1, 16, 0},  // Sceaux
    {43125, 6546, 0, 15, 0},  // Fontenay-aux-Roses
    {43186, 6565, 2, 14, 0},  // Robinson
    {46222, 6574, 1, 36, 0},  // Vert-Galant
    {46725, 6586, 0, 37, 0},  // Villeparisis-Mitry-le-Neuf
    {43164, 6613, 2, 38, 0},  // Mitry-Claye
    {44411, 6625, 2, 0, 0},  // Pontoise
    {47599, 6634, 0, 1, 0},  // Saint-Ouen-l’Aumône
    {43150, 6657, 0, 2, 0},  // Saint-Ouen-l’Aumône-Liesse
    {43180, 6687, 0, 3, 0},  // Pierrelaye
    {43166, 6698, 0, 4, 0},  // Montigny-Beauchamp
    {43126, 6717, 0, 5, 0},  // Franconville-Le Plessis-Bouchard
    {43105, 6750, 0, 6, 0},  // Cernay
    {47898, 6757, 0, 7, 0},  // Ermont-Eaubonne
    {43201, 6773, 0, 8, 0},  // Saint-Gratien
    {43077, 6787, 0, 9, 0},  // Épinay-sur-Seine
    {43130, 6805, 0, 10, 0},  // Gennevilliers
    {43230, 6819, 0, 11, 0},  // Les Grésillons
    {43203, 4632, 0, 12, 0},  // Saint-Ouen
    {44514, 4257, 0, 13, 0},  // Porte de Clichy
    {44314, 717, 0, 14, 0},  // Pereire-Levallois
    {415093, 6835, 0, 15, 0},  // Neuilly Porte Maillot
    {45437, 6857, 0, 16, 0},  // Avenue Foch
    {45447, 6869, 0, 17, 0},  // Avenue Henri Martin
    {45453, 6889, 0, 18, 0},  // Boulainvilliers
    {45419, 6905, 0, 19, 0},  // Avenue du Président Kennedy
    {45346, 6934, 0, 20, 0},  // Javel
    {58757, 6940, 0, 21, 0},  // Champ de Mars
    {415091, 6954, 0, 22, 0},  // Pont de l’Alma
    {470540, 2568, 0, 23, 0},  // Invalides
    {45705, 6971, 0, 24, 0},  // Musée d’Orsay
    {44877, 1157, 0, 25, 0},  // Saint-Michel
    {43072, 6988, 0, 26, 0},  // Paris Austerlitz
    {45301, 4677, 0, 27, 0},  // Bibliothèque François Mitterrand
    {46366, 7005, 0, 28, 0},  // Ivry-sur-Seine
    {46375, 7020, 0, 29, 0},  // Vitry-sur-Seine
    {43227, 7036, 0, 30, 0},  // Les Ardoines
    {43110, 7049, 0, 31, 0},  // Choisy-le-Roi
    {46298, 7063, 0, 32, 0},  // Les Saules
    {46307, 7074, 0, 33, 0},  // Villeneuve-le-Roi
    {46387, 7092, 0, 34, 0},  // Ablon
    {46395, 7098, 0, 35, 0},  // Athis-Mons
    {45739, 7109, 0, 36, 0},  // Juvisy
    {43192, 7116, 0, 37, 0},  // Savigny-sur-Orge
    {43076, 7133, 0, 38, 0},  // Épinay-sur-Orge
    {43188, 7150, 0, 39, 0},  // Sainte-Geneviève-des-Bois
    {43202, 7177, 0, 40, 0},  // Saint-Michel-sur-Orge
    {43098, 7199, 0, 41, 0},  // Brétigny
    {43159, 7209, 0, 42, 0},  // Marolles-en-Hurepoix
    {47939, 7230, 0, 43, 0},  // La Norville
    {43068, 7242, 0, 44, 0},  // Arpajon
    {415484, 7250, 0, 45, 0},  // Égly
    {47925, 7256, 0, 46, 0},  // Breuillet
    {43099, 7266, 0, 47, 0},  // Breuillet-Village
    {47952, 7284, 0, 48, 0},  // Saint-Chéron
    {47950, 7298, 0, 49, 0},  // Sermaise
    {43121, 7307, 0, 50, 0},  // Dourdan
    {47095, 7315, 2, 51, 0},  // Dourdan-la-Forêt
    {58798, 7333, 1, 21, 0},  // Pont du Garigliano
    {462357, 7352, 0, 22, 0},  // Issy-Val-de-Seine
    {43084, 7370, 0, 23, 0},  // Issy
    {43163, 7375, 0, 24, 0},  // Meudon-Val-Fleury
    {47883, 7393, 0, 25, 0},  // Chaville-Vélizy
    {462388, 7410, 0, 26, 0},  // Viroflay Rive Gauche
    {43183, 7431, 0, 27, 0},  // Porchefontaine
    {43219, 7446, 0, 28, 0},  // Versailles-Chantiers
    {46990, 7467, 0, 29, 0},  // Saint-Cyr
    {461504, 7477, 2, 30, 0},  // Saint-Quentin-en-Yvelines
    {46299, 7503, 1, 33, 0},  // Orly Ville
    {47907, 7514, 0, 34, 0},  // Pont de Rungis
    {44787, 7529, 0, 35, 0},  // Rungis-La Fraternelle
    {46035, 7551, 0, 36, 0},  // Chemin d’Antony
    {47940, 6170, 0, 37, 0},  // Massy-Verrières
    {58774, 6154, 2, 38, 0},  // Massy-Palaiseau
    {47905, 7569, 1, 43, 0},  // Bouray
    {43149, 7576, 0, 44, 0},  // Lardy
    {47927, 7582, 0, 45, 0},  // Chamarande
    {43081, 7593, 0, 46, 0},  // Étréchy
    {43080, 7603, 0, 47, 0},  // Étampes
    {47115, 7612, 2, 48, 0},  // Saint-Martin-d’Étampes
    {43220, 7638, 3, 26, 0},  // Versailles Château
    {411440, 7658, 2, 0, 0},  // Creil
    {411428, 7664, 0, 1, 0},  // Chantilly-Gouvieux
    {411421, 7683, 0, 2, 0},  // Orry-la-Ville-Coye-la-Forêt
    {411418, 7712, 0, 3, 0},  // La Borne Blanche
    {47958, 7729, 0, 4, 0},  // Survilliers-Fosses
    {44628, 7748, 0, 5, 0},  // Louvres
    {47968, 7756, 0, 6, 0},  // Les Noues
    {47876, 7766, 0, 7, 0},  // Goussainville
    {43222, 7780, 0, 8, 0},  // Villiers-le-Bel-Gonesse-Arnouville
    {43251, 7815, 0, 9, 0},  // Garges-Sarcelles
    {43179, 7832, 0, 10, 0},  // Pierrefitte Stains
    {412743, 7851, 0, 11, 0},  // Saint-Denis
    {43204, 7863, 0, 12, 0},  // Stade de France-Saint-Denis
    {462394, 1055, 0, 13, 0},  // Gare du Nord
    {45102, 5516, 0, 14, 0},  // Châtelet-Les Halles
    {470195, 289, 0, 15, 0},  // Gare de Lyon
    {43154, 7891, 0, 16, 0},  // Maisons-Alfort-Alfortville
    {464040, 7918, 0, 17, 0},  // Le Vert de Maisons
    {46286, 7937, 0, 18, 0},  // Créteil Pompadour
    {46304, 7956, 0, 19, 0},  // Villeneuve-Triage
    {45067, 7974, 0, 20, 0},  // Villeneuve-Saint-Georges
    {47684, 7999, 0, 21, 0},  // Montgeron-Crosne
    {43226, 8016, 0, 22, 0},  // Yerres
    {58873, 8023, 0, 23, 0},  // Brunoy
    {47924, 8030, 0, 24, 0},  // Boussy-Saint-Antoine
    {45771, 8051, 0, 25, 0},  // Combs-la-Ville - Quincy
    {47669, 8075, 0, 26, 0},  // Lieusaint-Moissy
    {47665, 8092, 0, 27, 0},  // Savigny-le-Temple-Nandy
    {42516, 8116, 0, 28, 0},  // Cesson
    {45784, 8123, 0, 29, 0},  // Le Mée
    {47909, 8131, 2, 30, 0},  // Melun
    {47967, 8137, 0, 31, 0},  // Vosves
    {45763, 8144, 0, 32, 0},  // Boissise-le-Roi
    {45812, 8160, 0, 33, 0},  // Ponthierry-Pringy
    {45814, 964, 0, 34, 0},  // Saint-Fargeau
    {45824, 8178, 0, 35, 0},  // Le Coudray-Montceaux
    {45820, 8199, 0, 36, 0},  // Le Plessis-Chenet
    {47904, 8217, 0, 37, 0},  // Villabé
    {45759, 8226, 0, 38, 0},  // Essonnes-Robinson
    {43115, 8244, 0, 39, 0},  // Corbeil-Essonnes
    {45761, 8261, 0, 40, 0},  // Moulin-Galant
    {47899, 8275, 0, 41, 0},  // Mennecy
    {43087, 8283, 0, 42, 0},  // Ballancourt
    {43142, 8295, 0, 43, 0},  // La Ferté-Alais
    {47895, 8311, 0, 44, 0},  // Boutigny
    {43156, 8320, 0, 45, 0},  // Maisse
    {43102, 8327, 0, 46, 0},  // Buno-Gironville
    {43092, 8343, 0, 47, 0},  // Boigneville
    {411485, 742, 2, 48, 0},  // Malesherbes
    {45735, 8355, 1, 21, 0},  // Vigneux-sur-Seine
    {45739, 7109, 0, 22, 0},  // Juvisy
    {43225, 8373, 0, 23, 0},  // Viry-Châtillon
    {47948, 8389, 0, 24, 0},  // Ris-Orangis
    {45860, 8401, 0, 25, 0},  // Grand-Bourg
    {45745, 8413, 0, 26, 0},  // Évry
    {45850, 8419, 0, 27, 0},  // Le Bras-de-Fer
    {47906, 8434, 0, 28, 0},  // Évry Courcouronnes
    {48489, 8454, 0, 29, 0},  // Orangis-Bois de l’Epine
    {43132, 8480, 0, 30, 0},  // Grigny-Centre
    {487011, 8494, 2, 0, 0},  // Nanterre - La Folie
    {470549, 0, 0, 1, 0},  // La Défense
    {415093, 65, 0, 2, 0},  // Porte Maillot
    {58718, 8514, 0, 3, 0},  // Haussmann-Saint-Lazare
    {58572, 8537, 0, 4, 0},  // Magenta
    {58498, 8545, 0, 5, 0},  // Rosa Parks
    {43176, 8556, 0, 6, 0},  // Pantin
    {44201, 8563, 0, 7, 0},  // Noisy-le-Sec
    {43096, 8576, 0, 8, 0},  // Bondy
    {42356, 3786, 0, 9, 0},  // Rosny-Bois-Perrier
    {47877, 8582, 0, 10, 0},  // Rosny-sous-Bois
    {47900, 5566, 0, 11, 0},  // Val de Fontenay
    {46552, 8598, 0, 12, 0},  // Nogent-le-Perreux
    {58267, 8616, 0, 13, 0},  // Les Boullereaux-Champigny
    {43223, 8642, 0, 14, 0},  // Villiers-sur-Marne - Le Plessis-Trévise
    {58288, 8683, 0, 15, 0},  // Les Yvris-Noisy-le-Grand
    {43074, 8708, 0, 16, 0},  // Émerainville-Pontault-Combault
    {46568, 8740, 0, 17, 0},  // Roissy-en-Brie
    {462901, 8755, 0, 18, 0},  // Ozoir-la-Ferrière
    {43131, 8774, 0, 19, 0},  // Gretz-Armainvilliers
    {43208, 8795, 2, 20, 0},  // Tournan
    {47901, 8803, 1, 9, 0},  // Le Raincy-Villemomble-Montfermeil
    {43044, 8837, 0, 10, 0},  // Gagny
    {43127, 8843, 0, 11, 0},  // Le Chénay-Gagny
    {58879, 8860, 2, 12, 0},  // Chelles-Gournay
    {42142, 8876, 2, 0, 0},  // Asnières Quatre Routes
    {462328, 4198, 0, 1, 0},  // Les Courtilles
    {46452, 8900, 0, 2, 0},  // Le Luth
    {50664, 8908, 0, 3, 0},  // Le Village
    {46450, 8919, 0, 4, 0},  // Timbaud
    {462332, 6805, 0, 5, 0},  // Gennevilliers
    {44252, 8927, 0, 6, 0},  // Parc des Chanteraines
    {42594, 8949, 0, 7, 0},  // Chemin des Reniers
    {47355, 8968, 0, 8, 0},  // La Noue
    {46332, 8976, 0, 9, 0},  // Mairie de Villeneuve-la-Garenne
    {46331, 9008, 0, 10, 0},  // L’Ile-Saint-Denis
    {462392, 7851, 0, 11, 0},  // Saint-Denis
    {44937, 9028, 0, 12, 0},  // Théâtre Gérard Philipe
    {44095, 9054, 0, 13, 0},  // Marché de Saint-Denis
    {44859, 4562, 0, 14, 0},  // Basilique de Saint-Denis
    {46327, 9077, 0, 15, 0},  // Cimetière de Saint-Denis
    {43375, 9103, 0, 16, 0},  // Hôpital Delafontaine
    {42763, 9125, 0, 17, 0},  // Cosmonautes
    {46100, 9137, 0, 18, 0},  // Six Routes
    {46120, 9148, 0, 19, 0},  // Hôtel de Ville de La Courneuve
    {46119, 9180, 0, 20, 0},  // Stade Géo André
    {46115, 9198, 0, 21, 0},  // Danton
    {42151, 1871, 0, 22, 0},  // La Courneuve 8 Mai 1945
    {44125, 9205, 0, 23, 0},  // Maurice Lachâtre
    {42866, 9223, 0, 24, 0},  // Drancy-Avenir
    {43373, 9237, 0, 25, 0},  // Hôpital Avicenne
    {47349, 9255, 0, 26, 0},  // Gaston Roulaud
    {47347, 9270, 0, 27, 0},  // Escadrille Normandie-Niémen
    {52382, 9299, 0, 28, 0},  // La Ferme
    {43805, 9308, 0, 29, 0},  // Libération
    {43390, 9320, 0, 30, 0},  // Hôtel de Ville de Bobigny
    {462329, 1379, 0, 31, 0},  // Bobigny Pablo Picasso
    {47334, 9347, 0, 32, 0},  // Jean Rostand
    {42245, 9360, 0, 33, 0},  // Auguste Delaune
    {44471, 9376, 0, 34, 0},  // Pont de Bondy
    {44325, 9390, 0, 35, 0},  // Petit Noisy
    {473310, 8563, 2, 36, 0},  // Noisy-le-Sec
    {50622, 9402, 2, 0, 0},  // Pont de Bezons
    {44678, 9417, 0, 1, 0},  // Parc Pierre Lagravère
    {58494, 9440, 0, 2, 0},  // Victor Basch
    {47413, 9453, 0, 3, 0},  // Jacqueline Auriol
    {47411, 9471, 0, 4, 0},  // Charlebourg
    {43729, 9483, 0, 5, 0},  // Les Fauvelles
    {479058, 9497, 0, 6, 0},  // Faubourg de l’Arche
    {462395, 0, 0, 7, 0},  // La Défense
    {462353, 9519, 0, 8, 0},  // Puteaux
    {42315, 9527, 0, 9, 0},  // Belvédère
    {44914, 9539, 0, 10, 0},  // Suresnes-Longchamp
    {47370, 9558, 0, 11, 0},  // Les Coteaux
    {51611, 9570, 0, 12, 0},  // Les Milons
    {44249, 9581, 0, 13, 0},  // Parc de Saint-Cloud
    {44189, 9601, 0, 14, 0},  // Musée de Sèvres
    {47364, 9619, 0, 15, 0},  // Brimborion
    {44137, 9630, 0, 16, 0},  // Meudon-sur-Seine
    {43765, 9647, 0, 17, 0},  // Les Moulineaux
    {43420, 9662, 0, 18, 0},  // Jacques-Henri Lartigue
    {473392, 7352, 0, 19, 0},  // Issy-Val-de-Seine
    {45358, 9685, 0, 20, 0},  // Henri Farman
    {44917, 9698, 0, 21, 0},  // Suzanne Lenglen
    {45356, 9714, 0, 22, 0},  // Porte d’Issy
    {473705, 4146, 2, 23, 0},  // Porte de Versailles
    {473331, 7333, 2, 0, 0},  // Pont du Garigliano
    {45357, 2487, 0, 1, 0},  // Balard
    {42844, 9729, 0, 2, 0},  // Desnouettes
    {473705, 4146, 0, 3, 0},  // Porte de Versailles
    {43285, 9741, 0, 4, 0},  // Georges Brassens
    {42387, 9758, 0, 5, 0},  // Brancion
    {473702, 4357, 0, 6, 0},  // Porte de Vanves
    {42847, 9767, 0, 7, 0},  // Didot
    {43462, 9773, 0, 8, 0},  // Jean Moulin
    {473694, 1309, 0, 9, 0},  // Porte d’Orléans
    {45329, 9785, 0, 10, 0},  // Montsouris
    {45339, 6324, 0, 11, 0},  // Cité Universitaire
    {44892, 9796, 0, 12, 0},  // Stade Charléty
    {44550, 9812, 0, 13, 0},  // Poterne des Peupliers
    {44506, 2220, 0, 14, 0},  // Porte d’Italie
    {473684, 2342, 0, 15, 0},  // Porte de Choisy
    {473717, 2358, 0, 16, 0},  // Porte d’Ivry
    {58493, 9834, 0, 17, 0},  // Maryse Bastié
    {45305, 9849, 0, 18, 0},  // Avenue de France
    {58730, 9866, 0, 19, 0},  // Baron Le Roy
    {473680, 2765, 0, 20, 0},  // Porte de Charenton
    {473692, 2752, 0, 21, 0},  // Porte Dorée
    {45214, 9879, 0, 22, 0},  // Montempoivre
    {45215, 9892, 0, 23, 0},  // Alexandra David-Néel
    {473707, 325, 2, 24, 0},  // Porte de Vincennes
    {490877, 388, 2, 0, 0},  // Porte Dauphine
    {490880, 9914, 0, 1, 0},  // Anna de Noailles
    {490883, 65, 0, 2, 0},  // Porte Maillot
    {490885, 9931, 0, 3, 0},  // Anny Flore
    {490888, 9942, 0, 4, 0},  // Thérèse Pierre
    {490891, 697, 0, 5, 0},  // Porte de Champerret
    {490894, 9959, 0, 6, 0},  // Square Sainte-Odile
    {45496, 9979, 0, 7, 0},  // Porte d’Asnières
    {473476, 4257, 0, 8, 0},  // Porte de Clichy
    {45482, 9999, 0, 9, 0},  // Honoré de Balzac
    {45478, 10017, 0, 10, 0},  // Épinettes - Pouchet
    {473689, 4465, 0, 11, 0},  // Porte de Saint-Ouen
    {45531, 10038, 0, 12, 0},  // Angélique Compoint
    {473685, 988, 0, 13, 0},  // Porte de Clignancourt
    {45537, 10058, 0, 14, 0},  // Diane Arbus
    {44517, 3861, 0, 15, 0},  // Porte de la Chapelle
    {45538, 10070, 0, 16, 0},  // Colette Besson
    {44503, 10085, 0, 17, 0},  // Porte d’Aubervilliers
    {473484, 8545, 0, 18, 0},  // Rosa Parks
    {45596, 10109, 0, 19, 0},  // Canal Saint-Denis
    {44518, 1956, 0, 20, 0},  // Porte de la Villette
    {58499, 10127, 0, 21, 0},  // Ella Fitzgerald
    {58500, 10143, 0, 22, 0},  // Delphine Seyrig
    {473687, 1458, 0, 23, 0},  // Porte de Pantin
    {58501, 10159, 0, 24, 0},  // Butte du Chapeau Rouge
    {45616, 10182, 0, 25, 0},  // Hôpital Robert Debré
    {473736, 948, 0, 26, 0},  // Porte des Lilas
    {58502, 10205, 0, 27, 0},  // Adrienne Bolland
    {45629, 10222, 0, 28, 0},  // Séverine
    {473453, 921, 0, 29, 0},  // Porte de Bagnolet
    {58503, 10232, 0, 30, 0},  // Marie de Miribel
    {473697, 3286, 0, 31, 0},  // Porte de Montreuil
    {473707, 325, 2, 32, 0},  // Porte de Vincennes
    {473095, 6426, 2, 0, 0},  // Aulnay-sous-Bois
    {411283, 10249, 0, 1, 0},  // Rougemont-Chanteloup
    {47933, 10270, 0, 2, 0},  // Freinville-Sevran
    {47936, 10288, 0, 3, 0},  // L’Abbaye
    {411280, 10299, 0, 4, 0},  // Lycée Henri Sellier
    {43128, 10320, 0, 5, 0},  // Gargan
    {44626, 10327, 0, 6, 0},  // République Marx Dormoy
    {43430, 10351, 0, 7, 0},  // Léon Blum
    {479052, 10362, 0, 8, 0},  // Maurice Audin
    {477287, 10376, 0, 9, 0},  // Clichy-sous-Bois Mairie
    {43043, 10400, 0, 10, 0},  // Romain Rolland
    {477277, 10415, 0, 11, 0},  // Clichy - Montfermeil
    {479060, 10436, 0, 12, 0},  // Notre-Dame-des-Anges
    {478647, 10457, 2, 13, 0},  // Arboretum
    {43978, 10467, 2, 14, 0},  // Hôpital de Montfermeil
    {46521, 10491, 1, 4, 0},  // Les Pavillons-sous-Bois
    {462929, 10515, 0, 3, 0},  // Allée de la Tour Rendez-Vous
    {46516, 10545, 0, 2, 0},  // Les Coquetiers
    {44603, 10560, 0, 1, 0},  // La Remise à Jorelle
    {473143, 8576, 2, 0, 0},  // Bondy
    {473306, 7815, 2, 0, 0},  // Garges-Sarcelles
    {43748, 10581, 0, 1, 0},  // Lochères
    {44298, 10591, 0, 2, 0},  // Paul Valéry
    {43013, 10604, 0, 3, 0},  // Les Flanades
    {59018, 10617, 0, 4, 0},  // Les Cholettes
    {58640, 10631, 0, 5, 0},  // Butte Pinson
    {58639, 10644, 0, 6, 0},  // Jacques Prévert
    {46345, 10661, 0, 7, 0},  // Alcide d’Orbigny
    {50687, 10680, 0, 8, 0},  // Mairie de Pierrefitte
    {50686, 10702, 0, 9, 0},  // Suzanne Valadon
    {58638, 10718, 0, 10, 0},  // Joncherolles
    {46338, 10731, 0, 11, 0},  // Petit Pierrefitte
    {50685, 10749, 0, 12, 0},  // Guynemer
    {50681, 10758, 0, 13, 0},  // Roger Sémat
    {58637, 10771, 0, 14, 0},  // Baudelaire
    {44095, 9054, 2, 15, 0},  // Marché de Saint-Denis
    {473567, 10782, 2, 0, 0},  // Viroflay Rive Droite
    {473569, 7410, 0, 1, 0},  // Viroflay Rive Gauche
    {44649, 10803, 0, 2, 0},  // Robert Wagner
    {43501, 10817, 0, 3, 0},  // L’Onde
    {42430, 10826, 0, 4, 0},  // Mairie de Vélizy
    {43830, 10844, 0, 5, 0},  // Louvois
    {44285, 10852, 0, 6, 0},  // Inovel Parc Nord
    {42846, 10869, 0, 7, 0},  // Dewoitine
    {59053, 10879, 0, 8, 0},  // Vélizy 2
    {42830, 10889, 0, 9, 0},  // Meudon-la-Forêt
    {59147, 10906, 0, 10, 0},  // Georges Millandy
    {50800, 10923, 0, 11, 0},  // Georges Pompidou
    {44300, 10940, 0, 12, 0},  // Pavé Blanc
    {42642, 10952, 0, 13, 0},  // Mail de la Plaine
    {47746, 10970, 0, 14, 0},  // Hôpital Béclère
    {44838, 10989, 0, 15, 0},  // Soleil Levant
    {479057, 11003, 0, 16, 0},  // Division Leclerc
    {59179, 11020, 0, 17, 0},  // Parc André Malraux
    {59221, 11040, 0, 18, 0},  // Centre de Châtillon
    {59202, 11061, 0, 19, 0},  // Vauban
    {462350, 4432, 2, 20, 0},  // Châtillon-Montrouge
    {473766, 2317, 2, 0, 0},  // Villejuif - Louis Aragon
    {53692, 11068, 0, 1, 0},  // Lamartine
    {42882, 11078, 0, 2, 0},  // Domaine Chérioux
    {47693, 11096, 0, 3, 0},  // Moulin Vert
    {47694, 11108, 0, 4, 0},  // Bretagne
    {58784, 11117, 0, 5, 0},  // Auguste Perret
    {47421, 4787, 0, 6, 0},  // Chevilly-Larue
    {47688, 11132, 0, 7, 0},  // La Belle Épine
    {57491, 11148, 0, 8, 0},  // Place de la Logistique
    {58785, 11171, 0, 9, 0},  // Porte de Rungis
    {52184, 11187, 0, 10, 0},  // Saarinen
    {44994, 11196, 0, 11, 0},  // Robert Schuman
    {473468, 11211, 0, 12, 0},  // La Fraternelle
    {58813, 11226, 0, 13, 0},  // Hélène Boucher
    {58781, 11243, 0, 14, 0},  // Caroline Aigle
    {58782, 11258, 0, 15, 0},  // Coeur d’Orly
    {462747, 4816, 0, 16, 0},  // Aéroport d’Orly
    {58786, 11273, 0, 17, 0},  // Porte de l’Essonne
    {50483, 11294, 2, 0, 0},  // Épinay-Orgemont
    {462330, 6787, 0, 1, 0},  // Épinay-sur-Seine
    {59249, 11311, 0, 2, 0},  // Gilbert Bonnemaison
    {50564, 11331, 0, 3, 0},  // Lacépède
    {415806, 11342, 0, 4, 0},  // Rose Bertin
    {50566, 11354, 0, 5, 0},  // Les Béatus
    {47362, 11366, 0, 6, 0},  // Les Mobiles
    {59325, 11378, 0, 7, 0},  // Blumenthal
    {50572, 11389, 0, 8, 0},  // César
    {59326, 11396, 0, 9, 0},  // Delaunay-Belleville
    {46334, 11416, 0, 10, 0},  // Paul Éluard
    {462392, 7851, 0, 11, 0},  // Saint-Denis
    {50567, 11429, 0, 12, 0},  // Pierre De Geyter
    {44861, 4533, 2, 13, 0},  // Saint-Denis - Porte de Paris
    {59327, 11446, 1, 7, 0},  // Jean Vilar
    {59329, 11457, 0, 6, 0},  // Pablo Neruda
    {462926, 11470, 2, 5, 0},  // Villetaneuse-Université
    {473684, 2342, 2, 0, 0},  // Porte de Choisy
    {479017, 11495, 0, 1, 0},  // Châteaudun - Barbès
    {479018, 11517, 0, 2, 0},  // Cimetière Parisien d’Ivry
    {479014, 11546, 0, 3, 0},  // La Briqueterie
    {479021, 11561, 0, 4, 0},  // Germaine Tailleferre
    {478994, 11582, 0, 5, 0},  // Beethoven - Concorde
    {479030, 11603, 0, 6, 0},  // Musée MAC VAL
    {479022, 11618, 0, 7, 0},  // Mairie de Vitry-sur-Seine
    {479015, 11644, 0, 8, 0},  // Camille Groult
    {479019, 11659, 0, 9, 0},  // Constant Coquelin
    {479028, 11677, 0, 10, 0},  // Watteau - Rondenay
    {479026, 11696, 0, 11, 0},  // Trois communes
    {479027, 11711, 0, 12, 0},  // Verdun Hoche
    {479025, 11724, 0, 13, 0},  // Rouget de Lisle
    {479011, 11740, 0, 14, 0},  // Carle-Darthe
    {479020, 11753, 0, 15, 0},  // Four-Peary
    {479016, 11764, 0, 16, 0},  // Christophe Colomb
    {478984, 7063, 0, 17, 0},  // Les Saules
    {479023, 11782, 0, 18, 0},  // Orly - Gaston Viens
    {486811, 11802, 2, 0, 0},  // Jardin Parisien
    {47746, 10970, 0, 1, 0},  // Hôpital Béclère
    {486815, 11818, 0, 2, 0},  // Le Hameau
    {486817, 11828, 0, 3, 0},  // Parc des Sports
    {486819, 11844, 0, 4, 0},  // Noveos
    {486821, 11851, 0, 5, 0},  // Malabry
    {486823, 11859, 0, 6, 0},  // Vallée aux Loups
    {486825, 11877, 0, 7, 0},  // Cité-Jardin
    {486827, 11890, 0, 8, 0},  // Les Peintres
    {486829, 11903, 0, 9, 0},  // Théâtre La Piscine
    {486831, 11924, 0, 10, 0},  // Petit-Châtenay
    {486833, 11940, 0, 11, 0},  // LaVallée
    {486835, 11950, 2, 12, 0},  // La Croix de Berny
    {462330, 6787, 2, 0, 0},  // Épinay-sur-Seine
    {462331, 11968, 0, 1, 0},  // Épinay Villetaneuse
    {462926, 11989, 0, 2, 0},  // Villetaneuse - Université
    {426089, 7832, 0, 3, 0},  // Pierrefitte Stains
    {462931, 12016, 0, 4, 0},  // Stains La Cerisaie
    {426092, 12035, 0, 5, 0},  // Dugny - La Courneuve
    {462335, 4859, 2, 6, 0},  // Le Bourget
    {486884, 6154, 2, 0, 0},  // Massy-Palaiseau
    {486961, 12056, 0, 1, 0},  // Massy Europe
    {486958, 12069, 0, 2, 0},  // Champlan
    {486896, 12078, 0, 3, 0},  // Longjumeau
    {486893, 12089, 0, 4, 0},  // Chilly-Mazarin
    {486890, 12104, 0, 5, 0},  // Gravigny-Balizy
    {486887, 12120, 0, 6, 0},  // Petit-Vaux
    {486879, 7133, 0, 7, 0},  // Épinay-sur-Orge
    {486979, 12131, 0, 8, 0},  // Parc du Château
    {486977, 12148, 0, 9, 0},  // Coteaux de l’Orge
    {486974, 12168, 0, 10, 0},  // Amédée Gordini
    {486971, 12185, 0, 11, 0},  // Ferme Neuve
    {486968, 12197, 0, 12, 0},  // Bois de Saint-Eutrope
    {486966, 12219, 0, 13, 0},  // Traité de Rome
    {486964, 12235, 0, 14, 0},  // Bois Briard
    {486901, 12247, 2, 15, 0},  // Évry - Courcouronnes
    {480905, 5830, 2, 0, 0},  // Saint-Germain-en-Laye
    {480926, 12269, 0, 1, 0},  // Camp des Loges
    {480929, 12284, 0, 2, 0},  // Lisière Pereire
    {480941, 12301, 0, 3, 0},  // Fourqueux - Bel Air
    {480943, 12321, 0, 4, 0},  // Mareil-Marly
    {480931, 12334, 0, 5, 0},  // L’Etang - Les Sablons
    {480907, 12358, 0, 6, 0},  // Saint-Nom-la-Bretèche - Forêt-De-Marly
    {480939, 12399, 0, 7, 0},  // Noisy-le-Roi
    {480933, 12412, 0, 8, 0},  // Bailly
    {480935, 12419, 0, 9, 0},  // Allée Royale
    {480937, 12433, 0, 10, 0},  // Les Portes de Saint-Cyr
    {480909, 7467, 0, 11, 0},  // Saint-Cyr
    {492417, 12457, 2, 0, 0},  // Esbly
    {492419, 12463, 0, 1, 0},  // Montry-Condé
    {492421, 12477, 0, 2, 0},  // Couilly-Saint-Germain-Quincy
    {492423, 12506, 0, 3, 0},  // Villiers-Montbarbin
    {492425, 12526, 2, 4, 0},  // Crécy-la-Chapelle
};

// year : ligne pas encore en service, ouverture prévue cette année-là (0 : en service)
static const NetLine NET_LINES[] = {
    {"C01371", 0, 25, 0},  // 1
    {"C01372", 25, 25, 0},  // 2
    {"C01373", 50, 25, 0},  // 3
    {"C01386", 75, 4, 0},  // 3B
    {"C01374", 79, 29, 0},  // 4
    {"C01375", 108, 22, 0},  // 5
    {"C01376", 130, 28, 0},  // 6
    {"C01377", 158, 38, 0},  // 7
    {"C01387", 196, 8, 0},  // 7B
    {"C01378", 204, 38, 0},  // 8
    {"C01379", 242, 37, 0},  // 9
    {"C01380", 279, 23, 0},  // 10
    {"C01381", 302, 19, 0},  // 11
    {"C01382", 321, 31, 0},  // 12
    {"C01383", 352, 32, 0},  // 13
    {"C01384", 384, 21, 0},  // 14
    {"M16", 405, 10, 2027},  // 16
    {"M17", 415, 9, 2027},  // 17
    {"C02833", 424, 10, 2026},  // 18
    {"C01742", 434, 46, 0},  // A
    {"C01743", 480, 47, 0},  // B
    {"C01727", 527, 75, 0},  // C
    {"C01728", 602, 59, 0},  // D
    {"C01729", 661, 25, 0},  // E
    {"C01389", 686, 37, 0},  // T1
    {"C01390", 723, 24, 0},  // T2
    {"C01391", 747, 25, 0},  // T3a
    {"C01679", 772, 33, 0},  // T3b
    {"C01843", 805, 20, 0},  // T4
    {"C01684", 825, 16, 0},  // T5
    {"C01794", 841, 21, 0},  // T6
    {"C01774", 862, 18, 0},  // T7
    {"C01795", 880, 17, 0},  // T8
    {"C02317", 897, 19, 0},  // T9
    {"C02528", 916, 13, 0},  // T10
    {"C01999", 929, 7, 0},  // T11
    {"C02529", 936, 16, 0},  // T12
    {"C02344", 952, 12, 0},  // T13
    {"C02732", 964, 5, 0},  // T14
};
