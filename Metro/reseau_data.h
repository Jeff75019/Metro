// Généré par tools/make_reseau.py — ne pas modifier à la main.
// Lignes de métro, RER et tramway d'Île-de-France, stations dans l'ordre de la ligne.
// Source : open data Île-de-France Mobilités (emplacement-des-gares-idf,
// traces-du-reseau-ferre-idf), licence ODbL.
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
    "Le Bourget" "\0"
    "Drancy" "\0"
    "Le Blanc-Mesnil" "\0"
    "Aulnay-sous-Bois" "\0"
    "Sevran-Livry" "\0"
    "Sevran-Beaudottes" "\0"
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
// pos : place le long de la ligne (branches : depuis leur embranchement)
static const NetStop NET_STOPS[] = {
    {470548, 0, 2, 0},  // La Défense
    {42962, 12, 0, 1},  // Esplanade de la Défense
    {44479, 37, 0, 2},  // Pont de Neuilly
    {43785, 53, 0, 3},  // Les Sablons
    {44533, 65, 0, 4},  // Porte Maillot
    {42213, 79, 0, 5},  // Argentine
    {477682, 89, 0, 6},  // Charles De Gaulle-Étoile
    {43283, 115, 0, 7},  // George V
    {43026, 124, 0, 8},  // Franklin D. Roosevelt
    {42531, 146, 0, 9},  // Champs-Élysées – Clemenceau
    {45676, 178, 0, 10},  // Concorde
    {45664, 187, 0, 11},  // Tuileries
    {44236, 197, 0, 12},  // Palais Royal - Musée du Louvre
    {44645, 229, 0, 13},  // Louvre-Rivoli
    {42587, 243, 0, 14},  // Châtelet
    {470524, 253, 0, 15},  // Hôtel de Ville
    {44881, 269, 0, 16},  // Saint-Paul
    {42288, 280, 0, 17},  // Bastille
    {43153, 289, 0, 18},  // Gare de Lyon
    {44637, 302, 0, 19},  // Reuilly-Diderot
    {462358, 318, 0, 20},  // Nation
    {44528, 325, 0, 21},  // Porte de Vincennes
    {47247, 344, 0, 22},  // Saint-Mandé
    {42318, 357, 0, 23},  // Bérault
    {42582, 366, 2, 24},  // Château de Vincennes
    {44509, 388, 2, 0},  // Porte Dauphine
    {45020, 403, 0, 1},  // Victor Hugo
    {477682, 89, 0, 2},  // Charles De Gaulle-Étoile
    {44931, 415, 0, 3},  // Ternes
    {42771, 422, 0, 4},  // Courcelles
    {44146, 433, 0, 5},  // Monceau
    {45056, 441, 0, 6},  // Villiers
    {44662, 450, 0, 7},  // Rome
    {44372, 455, 0, 8},  // Place de Clichy
    {42337, 471, 0, 9},  // Blanche
    {44345, 479, 0, 10},  // Pigalle
    {42210, 487, 0, 11},  // Anvers
    {42280, 494, 0, 12},  // Barbès-Rochechouart
    {43524, 515, 0, 13},  // La Chapelle
    {44901, 527, 0, 14},  // Stalingrad
    {43425, 538, 0, 15},  // Jaurès
    {42732, 546, 0, 16},  // Colonel Fabien
    {42311, 561, 0, 17},  // Belleville
    {42772, 572, 0, 18},  // Couronnes
    {44133, 582, 0, 19},  // Ménilmontant
    {44310, 596, 0, 20},  // Père Lachaise
    {44332, 611, 0, 21},  // Philippe Auguste
    {45196, 628, 0, 22},  // Alexandre Dumas
    {42271, 644, 0, 23},  // Avron
    {462358, 318, 2, 24},  // Nation
    {473676, 650, 2, 0},  // Pont de Levallois
    {42195, 668, 0, 1},  // Anatole France
    {51613, 683, 0, 2},  // Louise Michel
    {44511, 697, 0, 3},  // Porte de Champerret
    {462376, 717, 0, 4},  // Pereire-Levallois
    {45508, 735, 0, 5},  // Wagram
    {59370, 742, 0, 6},  // Malesherbes
    {45056, 441, 0, 7},  // Villiers
    {42972, 754, 0, 8},  // Europe
    {462374, 761, 0, 9},  // Saint-Lazare
    {462375, 774, 0, 10},  // Havre-Caumartin
    {42237, 790, 0, 11},  // Opéra
    {44587, 797, 0, 12},  // Quatre Septembre
    {45101, 814, 0, 13},  // Bourse
    {44825, 821, 0, 14},  // Sentier
    {44600, 829, 0, 15},  // Réaumur Sébastopol
    {42230, 850, 0, 16},  // Arts et Métiers
    {44924, 867, 0, 17},  // Temple
    {44617, 874, 0, 18},  // République
    {58725, 886, 0, 19},  // Parmentier
    {45194, 897, 0, 20},  // Rue Saint-Maur
    {44310, 596, 0, 21},  // Père Lachaise
    {43055, 912, 0, 22},  // Gambetta
    {44510, 921, 0, 23},  // Porte de Bagnolet
    {47890, 939, 2, 24},  // Gallieni
    {44529, 948, 2, 0},  // Porte des Lilas
    {44864, 964, 0, 1},  // Saint-Fargeau
    {45630, 978, 0, 2},  // Pelleport
    {43055, 912, 2, 3},  // Gambetta
    {44515, 988, 2, 0},  // Porte de Clignancourt
    {44833, 1010, 0, 1},  // Simplon
    {44075, 1018, 0, 2},  // Marcadet-Poissonniers
    {42585, 1040, 0, 3},  // Château Rouge
    {42280, 494, 0, 4},  // Barbès-Rochechouart
    {462941, 1055, 0, 5},  // Gare du Nord
    {43138, 1068, 0, 6},  // Gare de l’Est
    {42579, 1084, 0, 7},  // Château d’Eau
    {44909, 1101, 0, 8},  // Strasbourg-Saint-Denis
    {44600, 829, 0, 9},  // Réaumur Sébastopol
    {44970, 1124, 0, 10},  // Étienne Marcel
    {417675, 1140, 0, 11},  // Les Halles
    {42587, 243, 0, 12},  // Châtelet
    {42639, 1151, 0, 13},  // Cité
    {462369, 1157, 0, 14},  // Saint-Michel
    {58716, 1170, 0, 15},  // Odéon
    {44799, 1177, 0, 16},  // Saint-Germain-des-Prés
    {45120, 1201, 0, 17},  // Saint-Sulpice
    {44885, 1215, 0, 18},  // Saint-Placide
    {58578, 1229, 0, 19},  // Montparnasse-Bienvenüe
    {44997, 1253, 0, 20},  // Vavin
    {45314, 1259, 0, 21},  // Raspail
    {462351, 1267, 0, 22},  // Denfert-Rochereau
    {44183, 1285, 0, 23},  // Mouton-Duvernet
    {42175, 1301, 0, 24},  // Alésia
    {44508, 1309, 0, 25},  // Porte d’Orléans
    {45904, 1328, 2, 26},  // Mairie de Montrouge
    {483278, 1348, 0, 27},  // Barbara
    {483280, 1356, 2, 28},  // Bagneux - Lucie Aubrac
    {42345, 1379, 2, 0},  // Bobigny Pablo Picasso
    {42344, 1401, 0, 1},  // Bobigny-Pantin - Raymond Queneau
    {42949, 1434, 0, 2},  // Église de Pantin
    {43364, 1452, 0, 3},  // Hoche
    {44521, 1458, 0, 4},  // Porte de Pantin
    {44227, 1474, 0, 5},  // Ourcq
    {45581, 1480, 0, 6},  // Laumière
    {43425, 538, 0, 7},  // Jaurès
    {44901, 527, 0, 8},  // Stalingrad
    {462941, 1055, 0, 9},  // Gare du Nord
    {43138, 1068, 0, 10},  // Gare de l’Est
    {43417, 1490, 0, 11},  // Jacques Bonsergent
    {44617, 874, 0, 12},  // République
    {45193, 1509, 0, 13},  // Oberkampf
    {44641, 1519, 0, 14},  // Richard Lenoir
    {42390, 1534, 0, 15},  // Bréguet-Sabin
    {42288, 280, 0, 16},  // Bastille
    {44583, 1549, 0, 17},  // Quai de la Rapée
    {462352, 1567, 0, 18},  // Gare d’Austerlitz
    {44875, 1587, 0, 19},  // Saint-Marcel
    {42426, 1600, 0, 20},  // Campo Formio
    {44371, 1613, 2, 21},  // Place d’Italie
    {477682, 89, 2, 0},  // Charles De Gaulle-Étoile
    {43498, 1630, 0, 1},  // Kléber
    {42359, 1638, 0, 2},  // Boissière
    {44962, 1649, 0, 3},  // Trocadéro
    {45417, 1660, 0, 4},  // Passy
    {45340, 1666, 0, 5},  // Bir-Hakeim
    {42871, 1677, 0, 6},  // Dupleix
    {43566, 1685, 0, 7},  // La Motte Picquet-Grenelle
    {42419, 1711, 0, 8},  // Cambronne
    {44829, 1721, 0, 9},  // Sèvres-Lecourbe
    {44273, 1738, 0, 10},  // Pasteur
    {58578, 1229, 0, 11},  // Montparnasse-Bienvenüe
    {45313, 1746, 0, 12},  // Edgar Quinet
    {45314, 1259, 0, 13},  // Raspail
    {462351, 1267, 0, 14},  // Denfert-Rochereau
    {45315, 1759, 0, 15},  // Saint-Jacques
    {45276, 1773, 0, 16},  // Glacière
    {45274, 1783, 0, 17},  // Corvisart
    {44371, 1613, 0, 18},  // Place d’Italie
    {44192, 1793, 0, 19},  // Nationale
    {45267, 1803, 0, 20},  // Chevaleret
    {44582, 1814, 0, 21},  // Quai de la Gare
    {58728, 1830, 0, 22},  // Bercy
    {42869, 1836, 0, 23},  // Dugommier
    {45229, 1846, 0, 24},  // Daumesnil
    {45222, 1856, 0, 25},  // Bel-Air
    {44334, 1864, 0, 26},  // Picpus
    {462358, 318, 2, 27},  // Nation
    {470546, 1871, 2, 0},  // La Courneuve 8 Mai 1945
    {47891, 1895, 0, 1},  // Fort d’Aubervilliers
    {46492, 1918, 0, 2},  // Aubervilliers Pantin - Quatre Chemins
    {473461, 1956, 0, 3},  // Porte de la Villette
    {45598, 1977, 0, 4},  // Corentin Cariou
    {42780, 1993, 0, 5},  // Crimée
    {44643, 2001, 0, 6},  // Riquet
    {44901, 527, 0, 7},  // Stalingrad
    {43823, 2008, 0, 8},  // Louis Blanc
    {479053, 2020, 0, 9},  // Château-Landon
    {43138, 1068, 0, 10},  // Gare de l’Est
    {45165, 2036, 0, 11},  // Poissonnière
    {42414, 2050, 0, 12},  // Cadet
    {43656, 2056, 0, 13},  // Le Peletier
    {42589, 2068, 0, 14},  // Chaussée d’Antin - La Fayette
    {42237, 790, 0, 15},  // Opéra
    {44579, 2101, 0, 16},  // Pyramides
    {44236, 197, 0, 17},  // Palais Royal - Musée du Louvre
    {44491, 2111, 0, 18},  // Pont Neuf
    {42587, 243, 0, 19},  // Châtelet
    {44487, 2121, 0, 20},  // Pont Marie
    {44911, 2132, 0, 21},  // Sully-Morland
    {43493, 2146, 0, 22},  // Jussieu
    {44435, 2154, 0, 23},  // Place Monge
    {42461, 2166, 0, 24},  // Censier-Daubenton
    {43737, 2184, 0, 25},  // Les Gobelins
    {44371, 1613, 0, 26},  // Place d’Italie
    {43411, 2197, 0, 27},  // Tolbiac
    {44057, 2205, 0, 28},  // Maison Blanche
    {473456, 2220, 0, 29},  // Porte d’Italie
    {43643, 2237, 0, 30},  // Le Kremlin-Bicêtre
    {47826, 2257, 0, 31},  // Villejuif - Léo Lagrange
    {58471, 2283, 0, 32},  // Villejuif Paul Vaillant-Couturier
    {45049, 2317, 2, 33},  // Villejuif - Louis Aragon
    {44513, 2342, 1, 30},  // Porte de Choisy
    {44507, 2358, 0, 31},  // Porte d’Ivry
    {44343, 2373, 0, 32},  // Pierre et Marie Curie
    {44051, 2395, 2, 33},  // Mairie d’Ivry
    {43823, 2008, 2, 0},  // Louis Blanc
    {43425, 538, 0, 1},  // Jaurès
    {58767, 2411, 0, 2},  // Bolivar
    {45609, 2419, 0, 3},  // Buttes-Chaumont
    {42369, 2435, 0, 4},  // Botzaris
    {42822, 2444, 0, 5},  // Danube
    {45617, 2451, 2, 6},  // Pré Saint-Gervais
    {44396, 2470, 0, 7},  // Place des Fêtes
    {463354, 2487, 2, 0},  // Balard
    {45363, 2494, 0, 1},  // Lourmel
    {42370, 2502, 0, 2},  // Boucicaut
    {42984, 2512, 0, 3},  // Félix Faure
    {42734, 2525, 0, 4},  // Commerce
    {43566, 1685, 0, 5},  // La Motte Picquet-Grenelle
    {42885, 2534, 0, 6},  // École Militaire
    {43409, 2551, 0, 7},  // La Tour Maubourg
    {43407, 2568, 0, 8},  // Invalides
    {45676, 178, 0, 9},  // Concorde
    {43898, 2578, 0, 10},  // Madeleine
    {42237, 790, 0, 11},  // Opéra
    {44642, 2588, 0, 12},  // Richelieu-Drouot
    {43310, 2605, 0, 13},  // Grands Boulevards
    {45100, 2623, 0, 14},  // Bonne Nouvelle
    {44909, 1101, 0, 15},  // Strasbourg-Saint-Denis
    {44617, 874, 0, 16},  // République
    {58727, 2638, 0, 17},  // Filles du Calvaire
    {45208, 2657, 0, 18},  // Saint-Sébastien-Froissart
    {45097, 2684, 0, 19},  // Chemin Vert
    {42288, 280, 0, 20},  // Bastille
    {43674, 2696, 0, 21},  // Ledru Rollin
    {42980, 2709, 0, 22},  // Faidherbe Chaligny
    {44637, 302, 0, 23},  // Reuilly-Diderot
    {44160, 2728, 0, 24},  // Montgallet
    {45229, 1846, 0, 25},  // Daumesnil
    {44140, 2739, 0, 26},  // Michel Bizot
    {44532, 2752, 0, 27},  // Porte Dorée
    {44512, 2765, 0, 28},  // Porte de Charenton
    {43811, 2784, 0, 29},  // Liberté
    {42545, 2793, 0, 30},  // Charenton-Écoles
    {42889, 2811, 0, 31},  // École Vétérinaire de Maisons-Alfort
    {44066, 2850, 0, 32},  // Maisons-Alfort-Stade
    {44067, 2871, 0, 33},  // Maisons-Alfort-Les Juilliottes
    {45712, 2902, 0, 34},  // Créteil–L’Échat
    {42779, 2924, 0, 35},  // Créteil-Université
    {42778, 2945, 0, 36},  // Créteil–Préfecture
    {44453, 2968, 2, 37},  // Créteil - Pointe du Lac
    {44481, 2993, 2, 0},  // Pont de Sèvres
    {42334, 3009, 0, 1},  // Billancourt
    {47021, 3021, 0, 2},  // Marcel Sembat
    {44524, 3035, 0, 3},  // Porte de Saint-Cloud
    {42977, 3056, 0, 4},  // Exelmans
    {44139, 3065, 0, 5},  // Michel Ange-Molitor
    {44138, 3085, 0, 6},  // Michel Ange-Auteuil
    {43423, 3105, 0, 7},  // Jasmin
    {44594, 3112, 0, 8},  // Ranelagh
    {43567, 3121, 0, 9},  // La Muette
    {44461, 3131, 0, 10},  // Rue de la Pompe
    {44962, 1649, 0, 11},  // Trocadéro
    {43395, 3147, 0, 12},  // Iéna
    {42182, 3153, 0, 13},  // Alma-Marceau
    {43026, 124, 0, 14},  // Franklin D. Roosevelt
    {44884, 3166, 0, 15},  // Saint-Philippe du Roule
    {43511, 3190, 0, 16},  // Miromesnil
    {44857, 3201, 0, 17},  // Saint-Augustin
    {462375, 774, 0, 18},  // Havre-Caumartin
    {42589, 2068, 0, 19},  // Chaussée d’Antin - La Fayette
    {44642, 2588, 0, 20},  // Richelieu-Drouot
    {43310, 2605, 0, 21},  // Grands Boulevards
    {45100, 2623, 0, 22},  // Bonne Nouvelle
    {44909, 1101, 0, 23},  // Strasbourg-Saint-Denis
    {44617, 874, 0, 24},  // République
    {45193, 1509, 0, 25},  // Oberkampf
    {45205, 3216, 0, 26},  // Saint-Ambroise
    {45065, 3231, 0, 27},  // Voltaire
    {42560, 3240, 0, 28},  // Charonne
    {425987, 3249, 0, 29},  // Rue des Boulets
    {462358, 318, 0, 30},  // Nation
    {42410, 3265, 0, 31},  // Buzenval
    {44072, 3274, 0, 32},  // Maraîchers
    {44520, 3286, 0, 33},  // Porte de Montreuil
    {44650, 3305, 0, 34},  // Robespierre
    {42789, 3317, 0, 35},  // Croix de Chavaux
    {44052, 3334, 2, 36},  // Mairie de Montreuil
    {42947, 3354, 0, 0},  // Église d’Auteuil
    {44138, 3085, 0, 1},  // Michel Ange-Auteuil
    {44504, 3374, 0, 2},  // Porte d’Auteuil
    {42380, 3392, 0, 3},  // Boulogne - Jean Jaurès
    {44139, 3065, 0, 4},  // Michel Ange-Molitor
    {42543, 3416, 0, 5},  // Chardon Lagache
    {45430, 3432, 0, 6},  // Mirabeau
    {462371, 3441, 0, 7},  // Javel - André Citroën
    {42557, 3465, 0, 8},  // Charles Michels
    {42265, 3481, 0, 9},  // Avenue Émile Zola
    {43566, 1685, 0, 10},  // La Motte Picquet-Grenelle
    {51612, 3500, 0, 11},  // Ségur
    {42874, 3507, 0, 12},  // Duroc
    {45116, 3513, 0, 13},  // Vaneau
    {44828, 3520, 0, 14},  // Sèvres-Babylone
    {59379, 3537, 0, 15},  // Mabillon
    {58716, 1170, 0, 16},  // Odéon
    {42674, 3546, 0, 17},  // Cluny-La Sorbonne
    {44119, 3564, 0, 18},  // Maubert-Mutualité
    {42435, 3583, 0, 19},  // Cardinal Lemoine
    {43493, 2146, 0, 20},  // Jussieu
    {462352, 1567, 2, 21},  // Gare d’Austerlitz
    {42381, 3600, 3, 2},  // Boulogne - Pont de Saint-Cloud
    {42587, 243, 2, 0},  // Châtelet
    {470524, 253, 0, 1},  // Hôtel de Ville
    {44593, 3631, 0, 2},  // Rambuteau
    {42230, 850, 0, 3},  // Arts et Métiers
    {44617, 874, 0, 4},  // République
    {43298, 3641, 0, 5},  // Goncourt
    {42311, 561, 0, 6},  // Belleville
    {44580, 3650, 0, 7},  // Pyrénées
    {43483, 3661, 0, 8},  // Jourdain
    {44396, 2470, 0, 9},  // Place des Fêtes
    {44923, 3670, 0, 10},  // Télégraphe
    {44529, 948, 0, 11},  // Porte des Lilas
    {44055, 3683, 0, 12},  // Mairie des Lilas
    {490763, 3700, 0, 13},  // Serge Gainsbourg
    {490765, 3717, 0, 14},  // Romainville - Carnot
    {490767, 3738, 0, 15},  // Montreuil - Hôpital
    {490769, 3759, 0, 16},  // La Dhuys
    {490771, 3768, 0, 17},  // Coteaux Beauclair
    {490773, 3786, 2, 18},  // Rosny-Bois-Perrier
    {483899, 3805, 2, 0},  // Mairie d’Aubervilliers
    {483901, 3830, 0, 1},  // Aimé Césaire
    {58497, 3845, 0, 2},  // Front Populaire
    {473459, 3861, 0, 3},  // Porte de la Chapelle
    {44113, 3882, 0, 4},  // Marx Dormoy
    {44075, 1018, 0, 5},  // Marcadet-Poissonniers
    {479055, 3894, 0, 6},  // Jules Joffrin
    {43600, 3908, 0, 7},  // Lamarck-Caulaincourt
    {42155, 3929, 0, 8},  // Abbesses
    {44345, 479, 0, 9},  // Pigalle
    {44867, 3938, 0, 10},  // Saint-Georges
    {44210, 3952, 0, 11},  // Notre-Dame-de-Lorette
    {45170, 3974, 0, 12},  // Trinité d’Estienne d’Orves
    {462374, 761, 0, 13},  // Saint-Lazare
    {43898, 2578, 0, 14},  // Madeleine
    {45676, 178, 0, 15},  // Concorde
    {45704, 4006, 0, 16},  // Assemblée Nationale
    {44840, 4027, 0, 17},  // Solférino
    {58717, 4038, 0, 18},  // Rue du Bac
    {44828, 3520, 0, 19},  // Sèvres-Babylone
    {44609, 4049, 0, 20},  // Rennes
    {44211, 4056, 0, 21},  // Notre-Dame-des-Champs
    {58578, 4078, 0, 22},  // Montparnasse Bienvenüe
    {42981, 4102, 0, 23},  // Falguière
    {44273, 1738, 0, 24},  // Pasteur
    {45060, 4113, 0, 25},  // Volontaires
    {44995, 4125, 0, 26},  // Vaugirard
    {42751, 4135, 0, 27},  // Convention
    {49515, 4146, 0, 28},  // Porte de Versailles
    {42759, 4166, 0, 29},  // Corentin Celton
    {44050, 4182, 2, 30},  // Mairie d’Issy
    {42231, 4198, 2, 0},  // Les Courtilles
    {43693, 4213, 0, 1},  // Les Agnettes
    {43042, 4226, 0, 2},  // Gabriel Péri
    {44012, 4240, 0, 3},  // Mairie de Clichy
    {473477, 4257, 0, 4},  // Porte de Clichy
    {45487, 4273, 0, 5},  // Brochant
    {43548, 4282, 0, 6},  // La Fourche
    {44372, 455, 0, 7},  // Place de Clichy
    {43814, 4293, 0, 8},  // Liège
    {462374, 761, 0, 9},  // Saint-Lazare
    {43511, 3190, 0, 10},  // Miromesnil
    {42531, 146, 0, 11},  // Champs-Élysées – Clemenceau
    {43407, 2568, 0, 12},  // Invalides
    {45131, 4300, 0, 13},  // Varenne
    {45139, 4308, 0, 14},  // Saint-François-Xavier
    {42874, 3507, 0, 15},  // Duroc
    {58578, 1229, 0, 16},  // Montparnasse-Bienvenüe
    {43046, 4331, 0, 17},  // Gaîté
    {44316, 4339, 0, 18},  // Pernety
    {45322, 4347, 0, 19},  // Plaisance
    {44526, 4357, 0, 20},  // Porte de Vanves
    {413433, 4373, 0, 21},  // Malakoff - Plateau de Vanves
    {44069, 4402, 0, 22},  // Malakoff - Rue Étienne Dolet
    {42588, 4432, 2, 23},  // Châtillon-Montrouge
    {43333, 4453, 1, 5},  // Guy Môquet
    {44525, 4465, 0, 4},  // Porte de Saint-Ouen
    {43252, 4485, 0, 3},  // Garibaldi
    {43969, 4495, 0, 2},  // Mairie de Saint-Ouen
    {42447, 4516, 0, 1},  // Carrefour Pleyel
    {473458, 4533, 0, 0},  // Saint-Denis - Porte de Paris
    {463329, 4562, 0, -1},  // Basilique de Saint-Denis
    {44860, 4587, 2, -2},  // Saint-Denis - Université
    {490775, 4613, 0, 0},  // Saint-Denis Pleyel
    {43969, 4495, 0, 1},  // Mairie de Saint-Ouen
    {480289, 4632, 0, 2},  // Saint-Ouen
    {473477, 4257, 0, 3},  // Porte de Clichy
    {480287, 4643, 0, 4},  // Pont Cardinet
    {462374, 761, 0, 5},  // Saint-Lazare
    {43898, 2578, 0, 6},  // Madeleine
    {44579, 2101, 0, 7},  // Pyramides
    {42587, 243, 0, 8},  // Châtelet
    {43153, 289, 0, 9},  // Gare de Lyon
    {58728, 1830, 0, 10},  // Bercy
    {45234, 4657, 0, 11},  // Cour Saint-Émilion
    {473111, 4677, 0, 12},  // Bibliothèque François Mitterrand
    {479056, 4712, 2, 13},  // Olympiades
    {44057, 2205, 0, 14},  // Maison Blanche
    {490840, 4723, 0, 15},  // Hôpital Bicêtre
    {490907, 4741, 0, 16},  // Villejuif - Gustave Roussy
    {490838, 4768, 0, 17},  // L’Haÿ-les-Roses
    {490836, 4787, 0, 18},  // Chevilly-Larue
    {490834, 4802, 0, 19},  // Thiais - Orly
    {490909, 4816, 2, 20},  // Aéroport d’Orly
    {43104, 4835, 2, 0},  // Cergy Le Haut
    {47897, 4849, 0, 1},  // Cergy-Saint-Christophe
    {44559, 4872, 0, 2},  // Cergy-Préfecture
    {47879, 4890, 0, 3},  // Neuville Université
    {43114, 4911, 0, 4},  // Conflans-Fin-d’Oise
    {46647, 4933, 0, 5},  // Achères-Ville
    {47915, 4948, 0, 6},  // Achères-Grand-Cormier
    {473109, 4971, 0, 7},  // Maisons-Laffitte
    {43191, 4988, 0, 8},  // Sartrouville
    {43082, 5001, 0, 9},  // Houilles-Carrières-sur-Seine
    {43170, 5031, 0, 10},  // Nanterre-Université
    {43169, 5052, 0, 11},  // Nanterre-Préfecture
    {470549, 0, 0, 12},  // La Défense
    {58759, 89, 0, 13},  // Charles De Gaulle-Étoile
    {45873, 5073, 0, 14},  // Auber
    {45102, 5079, 0, 15},  // Châtelet-Les Halles
    {470195, 289, 0, 16},  // Gare de Lyon
    {473875, 318, 0, 17},  // Nation
    {43224, 5100, 0, 18},  // Vincennes
    {47238, 5110, 0, 19},  // Fontenay-sous-Bois
    {47900, 5129, 0, 20},  // Val de Fontenay
    {43172, 5145, 0, 21},  // Neuilly-Plaisance
    {43101, 5163, 0, 22},  // Bry-sur-Marne
    {474082, 5177, 0, 23},  // Noisy-le-Grand-Mont d’Est
    {58937, 5205, 0, 24},  // Noisy - Champs
    {44199, 5220, 0, 25},  // Noisiel
    {43152, 5228, 0, 26},  // Lognes
    {43207, 5235, 0, 27},  // Torcy
    {58874, 5241, 0, 28},  // Bussy-Saint-Georges
    {43213, 5261, 0, 29},  // Val d’Europe
    {43239, 5276, 2, 30},  // Marne-la-Vallée-Chessy
    {47874, 5300, 3, 5},  // Poissy
    {43171, 5307, 1, 9},  // Nanterre-Ville
    {58875, 5322, 0, 8},  // Rueil-Malmaison
    {53783, 5338, 0, 7},  // Châtou-Croissy
    {53784, 5354, 0, 6},  // Le Vésinet-Centre
    {43237, 5373, 0, 5},  // Le Vésinet-Le Pecq
    {43198, 5393, 2, 4},  // Saint-Germain-en-Laye
    {47886, 5415, 1, 20},  // Nogent-sur-Marne
    {43135, 5432, 0, 21},  // Joinville-le-Pont
    {44801, 5450, 0, 22},  // Saint-Maur-Créteil
    {43234, 5470, 0, 23},  // Le Parc de Saint-Maur
    {58270, 5492, 0, 24},  // Champigny
    {43146, 5502, 0, 25},  // La Varenne-Chennevières
    {58792, 5527, 0, 26},  // Sucy-Bonneuil
    {43094, 5541, 2, 27},  // Boissy-Saint-Léger
    {47889, 5561, 2, 0},  // Saint-Rémy-lès-Chevreuse
    {47052, 5588, 0, 1},  // Courcelle-sur-Yvette
    {47888, 5609, 0, 2},  // Gif-sur-Yvette
    {47046, 5624, 0, 3},  // La Hacquinière
    {43103, 5640, 0, 4},  // Bures-sur-Yvette
    {43086, 5657, 0, 5},  // Orsay-Ville
    {43232, 5669, 0, 6},  // Le Guichet
    {474069, 5680, 0, 7},  // Lozère
    {43175, 5688, 0, 8},  // Palaiseau-Villebon
    {47009, 5707, 0, 9},  // Palaiseau
    {58774, 5717, 0, 10},  // Massy-Palaiseau
    {47940, 5733, 0, 11},  // Massy-Verrières
    {43228, 5750, 0, 12},  // Les Baconnets
    {43124, 5764, 0, 13},  // Fontaine-Michalon
    {43066, 5782, 0, 14},  // Antony
    {46007, 5789, 0, 15},  // La Croix-de-Berny-Fresnes
    {43177, 5815, 0, 16},  // Parc de Sceaux
    {43097, 5830, 0, 17},  // Bourg-la-Reine
    {44493, 5845, 0, 18},  // Bagneux
    {43067, 5853, 0, 19},  // Arcueil - Cachan
    {43607, 5870, 0, 20},  // Laplace
    {45877, 5878, 0, 21},  // Gentilly
    {473843, 5887, 0, 22},  // Cité Universitaire
    {473890, 1267, 0, 23},  // Denfert-Rochereau
    {44500, 5907, 0, 24},  // Port Royal
    {43833, 5918, 0, 25},  // Luxembourg
    {44877, 1157, 0, 26},  // Saint-Michel
    {45102, 5079, 0, 27},  // Châtelet-Les Halles
    {462394, 1055, 0, 28},  // Gare du Nord
    {43145, 5929, 0, 29},  // La Plaine-Stade de France
    {43140, 5955, 0, 30},  // La Courneuve-Aubervilliers
    {43231, 5982, 0, 31},  // Le Bourget
    {43122, 5993, 0, 32},  // Drancy
    {46163, 6000, 0, 33},  // Le Blanc-Mesnil
    {43071, 6016, 0, 34},  // Aulnay-sous-Bois
    {43194, 6033, 0, 35},  // Sevran-Livry
    {43193, 6046, 0, 36},  // Sevran-Beaudottes
    {58793, 6064, 0, 37},  // Villepinte
    {47878, 6075, 0, 38},  // Parc des Expositions
    {462398, 6096, 0, 39},  // Aéroport Charles De Gaulle 1
    {473364, 6126, 2, 40},  // Aéroport Charles De Gaulle 2-TGV
    {59206, 6160, 1, 16},  // Sceaux
    {43125, 6167, 0, 15},  // Fontenay-aux-Roses
    {43186, 6186, 2, 14},  // Robinson
    {46222, 6195, 1, 36},  // Vert-Galant
    {46725, 6207, 0, 37},  // Villeparisis-Mitry-le-Neuf
    {43164, 6234, 2, 38},  // Mitry-Claye
    {44411, 6246, 2, 0},  // Pontoise
    {47599, 6255, 0, 1},  // Saint-Ouen-l’Aumône
    {43150, 6278, 0, 2},  // Saint-Ouen-l’Aumône-Liesse
    {43180, 6308, 0, 3},  // Pierrelaye
    {43166, 6319, 0, 4},  // Montigny-Beauchamp
    {43126, 6338, 0, 5},  // Franconville-Le Plessis-Bouchard
    {43105, 6371, 0, 6},  // Cernay
    {47898, 6378, 0, 7},  // Ermont-Eaubonne
    {43201, 6394, 0, 8},  // Saint-Gratien
    {43077, 6408, 0, 9},  // Épinay-sur-Seine
    {43130, 6426, 0, 10},  // Gennevilliers
    {43230, 6440, 0, 11},  // Les Grésillons
    {43203, 4632, 0, 12},  // Saint-Ouen
    {44514, 4257, 0, 13},  // Porte de Clichy
    {44314, 717, 0, 14},  // Pereire-Levallois
    {415093, 6456, 0, 15},  // Neuilly Porte Maillot
    {45437, 6478, 0, 16},  // Avenue Foch
    {45447, 6490, 0, 17},  // Avenue Henri Martin
    {45453, 6510, 0, 18},  // Boulainvilliers
    {45419, 6526, 0, 19},  // Avenue du Président Kennedy
    {45346, 6555, 0, 20},  // Javel
    {58757, 6561, 0, 21},  // Champ de Mars
    {415091, 6575, 0, 22},  // Pont de l’Alma
    {470540, 2568, 0, 23},  // Invalides
    {45705, 6592, 0, 24},  // Musée d’Orsay
    {44877, 1157, 0, 25},  // Saint-Michel
    {43072, 6609, 0, 26},  // Paris Austerlitz
    {45301, 4677, 0, 27},  // Bibliothèque François Mitterrand
    {46366, 6626, 0, 28},  // Ivry-sur-Seine
    {46375, 6641, 0, 29},  // Vitry-sur-Seine
    {43227, 6657, 0, 30},  // Les Ardoines
    {43110, 6670, 0, 31},  // Choisy-le-Roi
    {46298, 6684, 0, 32},  // Les Saules
    {46307, 6695, 0, 33},  // Villeneuve-le-Roi
    {46387, 6713, 0, 34},  // Ablon
    {46395, 6719, 0, 35},  // Athis-Mons
    {45739, 6730, 0, 36},  // Juvisy
    {43192, 6737, 0, 37},  // Savigny-sur-Orge
    {43076, 6754, 0, 38},  // Épinay-sur-Orge
    {43188, 6771, 0, 39},  // Sainte-Geneviève-des-Bois
    {43202, 6798, 0, 40},  // Saint-Michel-sur-Orge
    {43098, 6820, 0, 41},  // Brétigny
    {43159, 6830, 0, 42},  // Marolles-en-Hurepoix
    {47939, 6851, 0, 43},  // La Norville
    {43068, 6863, 0, 44},  // Arpajon
    {415484, 6871, 0, 45},  // Égly
    {47925, 6877, 0, 46},  // Breuillet
    {43099, 6887, 0, 47},  // Breuillet-Village
    {47952, 6905, 0, 48},  // Saint-Chéron
    {47950, 6919, 0, 49},  // Sermaise
    {43121, 6928, 0, 50},  // Dourdan
    {47095, 6936, 2, 51},  // Dourdan-la-Forêt
    {58798, 6954, 1, 21},  // Pont du Garigliano
    {462357, 6973, 0, 22},  // Issy-Val-de-Seine
    {43084, 6991, 0, 23},  // Issy
    {43163, 6996, 0, 24},  // Meudon-Val-Fleury
    {47883, 7014, 0, 25},  // Chaville-Vélizy
    {462388, 7031, 0, 26},  // Viroflay Rive Gauche
    {43183, 7052, 0, 27},  // Porchefontaine
    {43219, 7067, 0, 28},  // Versailles-Chantiers
    {46990, 7088, 0, 29},  // Saint-Cyr
    {461504, 7098, 2, 30},  // Saint-Quentin-en-Yvelines
    {46299, 7124, 1, 33},  // Orly Ville
    {47907, 7135, 0, 34},  // Pont de Rungis
    {44787, 7150, 0, 35},  // Rungis-La Fraternelle
    {46035, 7172, 0, 36},  // Chemin d’Antony
    {47940, 5733, 0, 37},  // Massy-Verrières
    {58774, 5717, 2, 38},  // Massy-Palaiseau
    {47905, 7190, 1, 43},  // Bouray
    {43149, 7197, 0, 44},  // Lardy
    {47927, 7203, 0, 45},  // Chamarande
    {43081, 7214, 0, 46},  // Étréchy
    {43080, 7224, 0, 47},  // Étampes
    {47115, 7233, 2, 48},  // Saint-Martin-d’Étampes
    {43220, 7259, 3, 26},  // Versailles Château
    {411440, 7279, 2, 0},  // Creil
    {411428, 7285, 0, 1},  // Chantilly-Gouvieux
    {411421, 7304, 0, 2},  // Orry-la-Ville-Coye-la-Forêt
    {411418, 7333, 0, 3},  // La Borne Blanche
    {47958, 7350, 0, 4},  // Survilliers-Fosses
    {44628, 7369, 0, 5},  // Louvres
    {47968, 7377, 0, 6},  // Les Noues
    {47876, 7387, 0, 7},  // Goussainville
    {43222, 7401, 0, 8},  // Villiers-le-Bel-Gonesse-Arnouville
    {43251, 7436, 0, 9},  // Garges-Sarcelles
    {43179, 7453, 0, 10},  // Pierrefitte Stains
    {412743, 7472, 0, 11},  // Saint-Denis
    {43204, 7484, 0, 12},  // Stade de France-Saint-Denis
    {462394, 1055, 0, 13},  // Gare du Nord
    {45102, 5079, 0, 14},  // Châtelet-Les Halles
    {470195, 289, 0, 15},  // Gare de Lyon
    {43154, 7512, 0, 16},  // Maisons-Alfort-Alfortville
    {464040, 7539, 0, 17},  // Le Vert de Maisons
    {46286, 7558, 0, 18},  // Créteil Pompadour
    {46304, 7577, 0, 19},  // Villeneuve-Triage
    {45067, 7595, 0, 20},  // Villeneuve-Saint-Georges
    {47684, 7620, 0, 21},  // Montgeron-Crosne
    {43226, 7637, 0, 22},  // Yerres
    {58873, 7644, 0, 23},  // Brunoy
    {47924, 7651, 0, 24},  // Boussy-Saint-Antoine
    {45771, 7672, 0, 25},  // Combs-la-Ville - Quincy
    {47669, 7696, 0, 26},  // Lieusaint-Moissy
    {47665, 7713, 0, 27},  // Savigny-le-Temple-Nandy
    {42516, 7737, 0, 28},  // Cesson
    {45784, 7744, 0, 29},  // Le Mée
    {47909, 7752, 2, 30},  // Melun
    {47967, 7758, 0, 31},  // Vosves
    {45763, 7765, 0, 32},  // Boissise-le-Roi
    {45812, 7781, 0, 33},  // Ponthierry-Pringy
    {45814, 964, 0, 34},  // Saint-Fargeau
    {45824, 7799, 0, 35},  // Le Coudray-Montceaux
    {45820, 7820, 0, 36},  // Le Plessis-Chenet
    {47904, 7838, 0, 37},  // Villabé
    {45759, 7847, 0, 38},  // Essonnes-Robinson
    {43115, 7865, 0, 39},  // Corbeil-Essonnes
    {45761, 7882, 0, 40},  // Moulin-Galant
    {47899, 7896, 0, 41},  // Mennecy
    {43087, 7904, 0, 42},  // Ballancourt
    {43142, 7916, 0, 43},  // La Ferté-Alais
    {47895, 7932, 0, 44},  // Boutigny
    {43156, 7941, 0, 45},  // Maisse
    {43102, 7948, 0, 46},  // Buno-Gironville
    {43092, 7964, 0, 47},  // Boigneville
    {411485, 742, 2, 48},  // Malesherbes
    {45735, 7976, 1, 21},  // Vigneux-sur-Seine
    {45739, 6730, 0, 22},  // Juvisy
    {43225, 7994, 0, 23},  // Viry-Châtillon
    {47948, 8010, 0, 24},  // Ris-Orangis
    {45860, 8022, 0, 25},  // Grand-Bourg
    {45745, 8034, 0, 26},  // Évry
    {45850, 8040, 0, 27},  // Le Bras-de-Fer
    {47906, 8055, 0, 28},  // Évry Courcouronnes
    {48489, 8075, 0, 29},  // Orangis-Bois de l’Epine
    {43132, 8101, 0, 30},  // Grigny-Centre
    {487011, 8115, 2, 0},  // Nanterre - La Folie
    {470549, 0, 0, 1},  // La Défense
    {415093, 65, 0, 2},  // Porte Maillot
    {58718, 8135, 0, 3},  // Haussmann-Saint-Lazare
    {58572, 8158, 0, 4},  // Magenta
    {58498, 8166, 0, 5},  // Rosa Parks
    {43176, 8177, 0, 6},  // Pantin
    {44201, 8184, 0, 7},  // Noisy-le-Sec
    {43096, 8197, 0, 8},  // Bondy
    {42356, 3786, 0, 9},  // Rosny-Bois-Perrier
    {47877, 8203, 0, 10},  // Rosny-sous-Bois
    {47900, 5129, 0, 11},  // Val de Fontenay
    {46552, 8219, 0, 12},  // Nogent-le-Perreux
    {58267, 8237, 0, 13},  // Les Boullereaux-Champigny
    {43223, 8263, 0, 14},  // Villiers-sur-Marne - Le Plessis-Trévise
    {58288, 8304, 0, 15},  // Les Yvris-Noisy-le-Grand
    {43074, 8329, 0, 16},  // Émerainville-Pontault-Combault
    {46568, 8361, 0, 17},  // Roissy-en-Brie
    {462901, 8376, 0, 18},  // Ozoir-la-Ferrière
    {43131, 8395, 0, 19},  // Gretz-Armainvilliers
    {43208, 8416, 2, 20},  // Tournan
    {47901, 8424, 1, 9},  // Le Raincy-Villemomble-Montfermeil
    {43044, 8458, 0, 10},  // Gagny
    {43127, 8464, 0, 11},  // Le Chénay-Gagny
    {58879, 8481, 2, 12},  // Chelles-Gournay
    {42142, 8497, 2, 0},  // Asnières Quatre Routes
    {462328, 4198, 0, 1},  // Les Courtilles
    {46452, 8521, 0, 2},  // Le Luth
    {50664, 8529, 0, 3},  // Le Village
    {46450, 8540, 0, 4},  // Timbaud
    {462332, 6426, 0, 5},  // Gennevilliers
    {44252, 8548, 0, 6},  // Parc des Chanteraines
    {42594, 8570, 0, 7},  // Chemin des Reniers
    {47355, 8589, 0, 8},  // La Noue
    {46332, 8597, 0, 9},  // Mairie de Villeneuve-la-Garenne
    {46331, 8629, 0, 10},  // L’Ile-Saint-Denis
    {462392, 7472, 0, 11},  // Saint-Denis
    {44937, 8649, 0, 12},  // Théâtre Gérard Philipe
    {44095, 8675, 0, 13},  // Marché de Saint-Denis
    {44859, 4562, 0, 14},  // Basilique de Saint-Denis
    {46327, 8698, 0, 15},  // Cimetière de Saint-Denis
    {43375, 8724, 0, 16},  // Hôpital Delafontaine
    {42763, 8746, 0, 17},  // Cosmonautes
    {46100, 8758, 0, 18},  // Six Routes
    {46120, 8769, 0, 19},  // Hôtel de Ville de La Courneuve
    {46119, 8801, 0, 20},  // Stade Géo André
    {46115, 8819, 0, 21},  // Danton
    {42151, 1871, 0, 22},  // La Courneuve 8 Mai 1945
    {44125, 8826, 0, 23},  // Maurice Lachâtre
    {42866, 8844, 0, 24},  // Drancy-Avenir
    {43373, 8858, 0, 25},  // Hôpital Avicenne
    {47349, 8876, 0, 26},  // Gaston Roulaud
    {47347, 8891, 0, 27},  // Escadrille Normandie-Niémen
    {52382, 8920, 0, 28},  // La Ferme
    {43805, 8929, 0, 29},  // Libération
    {43390, 8941, 0, 30},  // Hôtel de Ville de Bobigny
    {462329, 1379, 0, 31},  // Bobigny Pablo Picasso
    {47334, 8968, 0, 32},  // Jean Rostand
    {42245, 8981, 0, 33},  // Auguste Delaune
    {44471, 8997, 0, 34},  // Pont de Bondy
    {44325, 9011, 0, 35},  // Petit Noisy
    {473310, 8184, 2, 36},  // Noisy-le-Sec
    {50622, 9023, 2, 0},  // Pont de Bezons
    {44678, 9038, 0, 1},  // Parc Pierre Lagravère
    {58494, 9061, 0, 2},  // Victor Basch
    {47413, 9074, 0, 3},  // Jacqueline Auriol
    {47411, 9092, 0, 4},  // Charlebourg
    {43729, 9104, 0, 5},  // Les Fauvelles
    {479058, 9118, 0, 6},  // Faubourg de l’Arche
    {462395, 0, 0, 7},  // La Défense
    {462353, 9140, 0, 8},  // Puteaux
    {42315, 9148, 0, 9},  // Belvédère
    {44914, 9160, 0, 10},  // Suresnes-Longchamp
    {47370, 9179, 0, 11},  // Les Coteaux
    {51611, 9191, 0, 12},  // Les Milons
    {44249, 9202, 0, 13},  // Parc de Saint-Cloud
    {44189, 9222, 0, 14},  // Musée de Sèvres
    {47364, 9240, 0, 15},  // Brimborion
    {44137, 9251, 0, 16},  // Meudon-sur-Seine
    {43765, 9268, 0, 17},  // Les Moulineaux
    {43420, 9283, 0, 18},  // Jacques-Henri Lartigue
    {473392, 6973, 0, 19},  // Issy-Val-de-Seine
    {45358, 9306, 0, 20},  // Henri Farman
    {44917, 9319, 0, 21},  // Suzanne Lenglen
    {45356, 9335, 0, 22},  // Porte d’Issy
    {473705, 4146, 2, 23},  // Porte de Versailles
    {473331, 6954, 2, 0},  // Pont du Garigliano
    {45357, 2487, 0, 1},  // Balard
    {42844, 9350, 0, 2},  // Desnouettes
    {473705, 4146, 0, 3},  // Porte de Versailles
    {43285, 9362, 0, 4},  // Georges Brassens
    {42387, 9379, 0, 5},  // Brancion
    {473702, 4357, 0, 6},  // Porte de Vanves
    {42847, 9388, 0, 7},  // Didot
    {43462, 9394, 0, 8},  // Jean Moulin
    {473694, 1309, 0, 9},  // Porte d’Orléans
    {45329, 9406, 0, 10},  // Montsouris
    {45339, 5887, 0, 11},  // Cité Universitaire
    {44892, 9417, 0, 12},  // Stade Charléty
    {44550, 9433, 0, 13},  // Poterne des Peupliers
    {44506, 2220, 0, 14},  // Porte d’Italie
    {473684, 2342, 0, 15},  // Porte de Choisy
    {473717, 2358, 0, 16},  // Porte d’Ivry
    {58493, 9455, 0, 17},  // Maryse Bastié
    {45305, 9470, 0, 18},  // Avenue de France
    {58730, 9487, 0, 19},  // Baron Le Roy
    {473680, 2765, 0, 20},  // Porte de Charenton
    {473692, 2752, 0, 21},  // Porte Dorée
    {45214, 9500, 0, 22},  // Montempoivre
    {45215, 9513, 0, 23},  // Alexandra David-Néel
    {473707, 325, 2, 24},  // Porte de Vincennes
    {490877, 388, 2, 0},  // Porte Dauphine
    {490880, 9535, 0, 1},  // Anna de Noailles
    {490883, 65, 0, 2},  // Porte Maillot
    {490885, 9552, 0, 3},  // Anny Flore
    {490888, 9563, 0, 4},  // Thérèse Pierre
    {490891, 697, 0, 5},  // Porte de Champerret
    {490894, 9580, 0, 6},  // Square Sainte-Odile
    {45496, 9600, 0, 7},  // Porte d’Asnières
    {473476, 4257, 0, 8},  // Porte de Clichy
    {45482, 9620, 0, 9},  // Honoré de Balzac
    {45478, 9638, 0, 10},  // Épinettes - Pouchet
    {473689, 4465, 0, 11},  // Porte de Saint-Ouen
    {45531, 9659, 0, 12},  // Angélique Compoint
    {473685, 988, 0, 13},  // Porte de Clignancourt
    {45537, 9679, 0, 14},  // Diane Arbus
    {44517, 3861, 0, 15},  // Porte de la Chapelle
    {45538, 9691, 0, 16},  // Colette Besson
    {44503, 9706, 0, 17},  // Porte d’Aubervilliers
    {473484, 8166, 0, 18},  // Rosa Parks
    {45596, 9730, 0, 19},  // Canal Saint-Denis
    {44518, 1956, 0, 20},  // Porte de la Villette
    {58499, 9748, 0, 21},  // Ella Fitzgerald
    {58500, 9764, 0, 22},  // Delphine Seyrig
    {473687, 1458, 0, 23},  // Porte de Pantin
    {58501, 9780, 0, 24},  // Butte du Chapeau Rouge
    {45616, 9803, 0, 25},  // Hôpital Robert Debré
    {473736, 948, 0, 26},  // Porte des Lilas
    {58502, 9826, 0, 27},  // Adrienne Bolland
    {45629, 9843, 0, 28},  // Séverine
    {473453, 921, 0, 29},  // Porte de Bagnolet
    {58503, 9853, 0, 30},  // Marie de Miribel
    {473697, 3286, 0, 31},  // Porte de Montreuil
    {473707, 325, 2, 32},  // Porte de Vincennes
    {473095, 6016, 2, 0},  // Aulnay-sous-Bois
    {411283, 9870, 0, 1},  // Rougemont-Chanteloup
    {47933, 9891, 0, 2},  // Freinville-Sevran
    {47936, 9909, 0, 3},  // L’Abbaye
    {411280, 9920, 0, 4},  // Lycée Henri Sellier
    {43128, 9941, 0, 5},  // Gargan
    {44626, 9948, 0, 6},  // République Marx Dormoy
    {43430, 9972, 0, 7},  // Léon Blum
    {479052, 9983, 0, 8},  // Maurice Audin
    {477287, 9997, 0, 9},  // Clichy-sous-Bois Mairie
    {43043, 10021, 0, 10},  // Romain Rolland
    {477277, 10036, 0, 11},  // Clichy - Montfermeil
    {479060, 10057, 0, 12},  // Notre-Dame-des-Anges
    {478647, 10078, 2, 13},  // Arboretum
    {43978, 10088, 2, 14},  // Hôpital de Montfermeil
    {46521, 10112, 1, 4},  // Les Pavillons-sous-Bois
    {462929, 10136, 0, 3},  // Allée de la Tour Rendez-Vous
    {46516, 10166, 0, 2},  // Les Coquetiers
    {44603, 10181, 0, 1},  // La Remise à Jorelle
    {473143, 8197, 2, 0},  // Bondy
    {473306, 7436, 2, 0},  // Garges-Sarcelles
    {43748, 10202, 0, 1},  // Lochères
    {44298, 10212, 0, 2},  // Paul Valéry
    {43013, 10225, 0, 3},  // Les Flanades
    {59018, 10238, 0, 4},  // Les Cholettes
    {58640, 10252, 0, 5},  // Butte Pinson
    {58639, 10265, 0, 6},  // Jacques Prévert
    {46345, 10282, 0, 7},  // Alcide d’Orbigny
    {50687, 10301, 0, 8},  // Mairie de Pierrefitte
    {50686, 10323, 0, 9},  // Suzanne Valadon
    {58638, 10339, 0, 10},  // Joncherolles
    {46338, 10352, 0, 11},  // Petit Pierrefitte
    {50685, 10370, 0, 12},  // Guynemer
    {50681, 10379, 0, 13},  // Roger Sémat
    {58637, 10392, 0, 14},  // Baudelaire
    {44095, 8675, 2, 15},  // Marché de Saint-Denis
    {473567, 10403, 2, 0},  // Viroflay Rive Droite
    {473569, 7031, 0, 1},  // Viroflay Rive Gauche
    {44649, 10424, 0, 2},  // Robert Wagner
    {43501, 10438, 0, 3},  // L’Onde
    {42430, 10447, 0, 4},  // Mairie de Vélizy
    {43830, 10465, 0, 5},  // Louvois
    {44285, 10473, 0, 6},  // Inovel Parc Nord
    {42846, 10490, 0, 7},  // Dewoitine
    {59053, 10500, 0, 8},  // Vélizy 2
    {42830, 10510, 0, 9},  // Meudon-la-Forêt
    {59147, 10527, 0, 10},  // Georges Millandy
    {50800, 10544, 0, 11},  // Georges Pompidou
    {44300, 10561, 0, 12},  // Pavé Blanc
    {42642, 10573, 0, 13},  // Mail de la Plaine
    {47746, 10591, 0, 14},  // Hôpital Béclère
    {44838, 10610, 0, 15},  // Soleil Levant
    {479057, 10624, 0, 16},  // Division Leclerc
    {59179, 10641, 0, 17},  // Parc André Malraux
    {59221, 10661, 0, 18},  // Centre de Châtillon
    {59202, 10682, 0, 19},  // Vauban
    {462350, 4432, 2, 20},  // Châtillon-Montrouge
    {473766, 2317, 2, 0},  // Villejuif - Louis Aragon
    {53692, 10689, 0, 1},  // Lamartine
    {42882, 10699, 0, 2},  // Domaine Chérioux
    {47693, 10717, 0, 3},  // Moulin Vert
    {47694, 10729, 0, 4},  // Bretagne
    {58784, 10738, 0, 5},  // Auguste Perret
    {47421, 4787, 0, 6},  // Chevilly-Larue
    {47688, 10753, 0, 7},  // La Belle Épine
    {57491, 10769, 0, 8},  // Place de la Logistique
    {58785, 10792, 0, 9},  // Porte de Rungis
    {52184, 10808, 0, 10},  // Saarinen
    {44994, 10817, 0, 11},  // Robert Schuman
    {473468, 10832, 0, 12},  // La Fraternelle
    {58813, 10847, 0, 13},  // Hélène Boucher
    {58781, 10864, 0, 14},  // Caroline Aigle
    {58782, 10879, 0, 15},  // Coeur d’Orly
    {462747, 4816, 0, 16},  // Aéroport d’Orly
    {58786, 10894, 0, 17},  // Porte de l’Essonne
    {50483, 10915, 2, 0},  // Épinay-Orgemont
    {462330, 6408, 0, 1},  // Épinay-sur-Seine
    {59249, 10932, 0, 2},  // Gilbert Bonnemaison
    {50564, 10952, 0, 3},  // Lacépède
    {415806, 10963, 0, 4},  // Rose Bertin
    {50566, 10975, 0, 5},  // Les Béatus
    {47362, 10987, 0, 6},  // Les Mobiles
    {59325, 10999, 0, 7},  // Blumenthal
    {50572, 11010, 0, 8},  // César
    {59326, 11017, 0, 9},  // Delaunay-Belleville
    {46334, 11037, 0, 10},  // Paul Éluard
    {462392, 7472, 0, 11},  // Saint-Denis
    {50567, 11050, 0, 12},  // Pierre De Geyter
    {44861, 4533, 2, 13},  // Saint-Denis - Porte de Paris
    {59327, 11067, 1, 7},  // Jean Vilar
    {59329, 11078, 0, 6},  // Pablo Neruda
    {462926, 11091, 2, 5},  // Villetaneuse-Université
    {473684, 2342, 2, 0},  // Porte de Choisy
    {479017, 11116, 0, 1},  // Châteaudun - Barbès
    {479018, 11138, 0, 2},  // Cimetière Parisien d’Ivry
    {479014, 11167, 0, 3},  // La Briqueterie
    {479021, 11182, 0, 4},  // Germaine Tailleferre
    {478994, 11203, 0, 5},  // Beethoven - Concorde
    {479030, 11224, 0, 6},  // Musée MAC VAL
    {479022, 11239, 0, 7},  // Mairie de Vitry-sur-Seine
    {479015, 11265, 0, 8},  // Camille Groult
    {479019, 11280, 0, 9},  // Constant Coquelin
    {479028, 11298, 0, 10},  // Watteau - Rondenay
    {479026, 11317, 0, 11},  // Trois communes
    {479027, 11332, 0, 12},  // Verdun Hoche
    {479025, 11345, 0, 13},  // Rouget de Lisle
    {479011, 11361, 0, 14},  // Carle-Darthe
    {479020, 11374, 0, 15},  // Four-Peary
    {479016, 11385, 0, 16},  // Christophe Colomb
    {478984, 6684, 0, 17},  // Les Saules
    {479023, 11403, 0, 18},  // Orly - Gaston Viens
    {486811, 11423, 2, 0},  // Jardin Parisien
    {47746, 10591, 0, 1},  // Hôpital Béclère
    {486815, 11439, 0, 2},  // Le Hameau
    {486817, 11449, 0, 3},  // Parc des Sports
    {486819, 11465, 0, 4},  // Noveos
    {486821, 11472, 0, 5},  // Malabry
    {486823, 11480, 0, 6},  // Vallée aux Loups
    {486825, 11498, 0, 7},  // Cité-Jardin
    {486827, 11511, 0, 8},  // Les Peintres
    {486829, 11524, 0, 9},  // Théâtre La Piscine
    {486831, 11545, 0, 10},  // Petit-Châtenay
    {486833, 11561, 0, 11},  // LaVallée
    {486835, 11571, 2, 12},  // La Croix de Berny
    {462330, 6408, 2, 0},  // Épinay-sur-Seine
    {462331, 11589, 0, 1},  // Épinay Villetaneuse
    {462926, 11610, 0, 2},  // Villetaneuse - Université
    {426089, 7453, 0, 3},  // Pierrefitte Stains
    {462931, 11637, 0, 4},  // Stains La Cerisaie
    {426092, 11656, 0, 5},  // Dugny - La Courneuve
    {462335, 5982, 2, 6},  // Le Bourget
    {486884, 5717, 2, 0},  // Massy-Palaiseau
    {486961, 11677, 0, 1},  // Massy Europe
    {486958, 11690, 0, 2},  // Champlan
    {486896, 11699, 0, 3},  // Longjumeau
    {486893, 11710, 0, 4},  // Chilly-Mazarin
    {486890, 11725, 0, 5},  // Gravigny-Balizy
    {486887, 11741, 0, 6},  // Petit-Vaux
    {486879, 6754, 0, 7},  // Épinay-sur-Orge
    {486979, 11752, 0, 8},  // Parc du Château
    {486977, 11769, 0, 9},  // Coteaux de l’Orge
    {486974, 11789, 0, 10},  // Amédée Gordini
    {486971, 11806, 0, 11},  // Ferme Neuve
    {486968, 11818, 0, 12},  // Bois de Saint-Eutrope
    {486966, 11840, 0, 13},  // Traité de Rome
    {486964, 11856, 0, 14},  // Bois Briard
    {486901, 11868, 2, 15},  // Évry - Courcouronnes
    {480905, 5393, 2, 0},  // Saint-Germain-en-Laye
    {480926, 11890, 0, 1},  // Camp des Loges
    {480929, 11905, 0, 2},  // Lisière Pereire
    {480941, 11922, 0, 3},  // Fourqueux - Bel Air
    {480943, 11942, 0, 4},  // Mareil-Marly
    {480931, 11955, 0, 5},  // L’Etang - Les Sablons
    {480907, 11979, 0, 6},  // Saint-Nom-la-Bretèche - Forêt-De-Marly
    {480939, 12020, 0, 7},  // Noisy-le-Roi
    {480933, 12033, 0, 8},  // Bailly
    {480935, 12040, 0, 9},  // Allée Royale
    {480937, 12054, 0, 10},  // Les Portes de Saint-Cyr
    {480909, 7088, 0, 11},  // Saint-Cyr
    {492417, 12078, 2, 0},  // Esbly
    {492419, 12084, 0, 1},  // Montry-Condé
    {492421, 12098, 0, 2},  // Couilly-Saint-Germain-Quincy
    {492423, 12127, 0, 3},  // Villiers-Montbarbin
    {492425, 12147, 2, 4},  // Crécy-la-Chapelle
};

static const NetLine NET_LINES[] = {
    {"C01371", 0, 25},  // 1
    {"C01372", 25, 25},  // 2
    {"C01373", 50, 25},  // 3
    {"C01386", 75, 4},  // 3B
    {"C01374", 79, 29},  // 4
    {"C01375", 108, 22},  // 5
    {"C01376", 130, 28},  // 6
    {"C01377", 158, 38},  // 7
    {"C01387", 196, 8},  // 7B
    {"C01378", 204, 38},  // 8
    {"C01379", 242, 37},  // 9
    {"C01380", 279, 23},  // 10
    {"C01381", 302, 19},  // 11
    {"C01382", 321, 31},  // 12
    {"C01383", 352, 32},  // 13
    {"C01384", 384, 21},  // 14
    {"C01742", 405, 46},  // A
    {"C01743", 451, 47},  // B
    {"C01727", 498, 75},  // C
    {"C01728", 573, 59},  // D
    {"C01729", 632, 25},  // E
    {"C01389", 657, 37},  // T1
    {"C01390", 694, 24},  // T2
    {"C01391", 718, 25},  // T3a
    {"C01679", 743, 33},  // T3b
    {"C01843", 776, 20},  // T4
    {"C01684", 796, 16},  // T5
    {"C01794", 812, 21},  // T6
    {"C01774", 833, 18},  // T7
    {"C01795", 851, 17},  // T8
    {"C02317", 868, 19},  // T9
    {"C02528", 887, 13},  // T10
    {"C01999", 900, 7},  // T11
    {"C02529", 907, 16},  // T12
    {"C02344", 923, 12},  // T13
    {"C02732", 935, 5},  // T14
};
