// Les 145 grandes gares dont Train Nomade publie les horaires prévus sur 14 jours
// (https://api.trainnomade.fr/gares/horaires/{fichier}.json) : code UIC principal -> fichier.
// Liste relevée sur https://trainnomade.fr/api le 25 octobre 2026 (données SNCF, ODbL).
#pragma once

struct TimetableStation {
  const char* uic;
  const char* slug;
};

static const TimetableStation TIMETABLE_STATIONS[] = {
    {"87781278", "gare-d-agde"},  // Agde
    {"87586008", "gare-d-agen"},  // Agen
    {"87319012", "gare-aix-en-provence-tgv"},  // Aix-en-Provence TGV
    {"87741132", "gare-aix-les-bains-le-revard"},  // Aix-les-Bains le Revard
    {"87313874", "gare-d-amiens"},  // Amiens
    {"87484006", "gare-angers-saint-laud"},  // Angers Saint-Laud
    {"87583005", "gare-d-angouleme"},  // Angoulême
    {"87746008", "gare-d-annecy"},  // Annecy
    {"87745497", "gare-d-annemasse"},  // Annemasse
    {"87757674", "gare-d-antibes"},  // Antibes
    {"87582668", "gare-d-arcachon"},  // Arcachon
    {"87381848", "gare-d-argenteuil"},  // Argenteuil
    {"87753657", "gare-d-arles"},  // Arles
    {"87342014", "gare-d-arras"},  // Arras
    {"87751750", "gare-d-aubagne"},  // Aubagne
    {"87271411", "gare-d-aulnay-sous-bois"},  // Aulnay-sous-Bois
    {"87476200", "gare-d-auray"},  // Auray
    {"87765008", "gare-avignon-centre"},  // Avignon Centre
    {"87318964", "gare-avignon-tgv"},  // Avignon TGV
    {"87981902", "gare-avignon-tgv"},  // Avignon TGV
    {"87673004", "gare-de-bayonne"},  // Bayonne
    {"87184002", "gare-de-belfort"},  // Belfort
    {"87300822", "gare-belfort-montbeliard-tgv"},  // Belfort - Montbéliard TGV
    {"87745000", "gare-de-bellegarde-sur-valserine"},  // Bellegarde-sur-Valserine
    {"87718007", "gare-besancon-viotte"},  // Besançon Viotte
    {"87342006", "gare-de-bethune"},  // Béthune
    {"87781005", "gare-de-beziers"},  // Béziers
    {"87113407", "gare-de-bondy"},  // Bondy
    {"87581009", "gare-bordeaux-saint-jean"},  // Bordeaux Saint-Jean
    {"87743005", "gare-de-bourg-en-bresse"},  // Bourg-en-Bresse
    {"87576207", "gare-de-bourges"},  // Bourges
    {"87474007", "gare-de-brest"},  // Brest
    {"87594002", "gare-de-brive-la-gaillarde"},  // Brive-la-Gaillarde
    {"87682120", "gare-de-brunoy"},  // Brunoy
    {"87444000", "gare-de-caen"},  // Caen
    {"87757625", "gare-de-cannes"},  // Cannes
    {"87615286", "gare-de-carcassonne"},  // Carcassonne
    {"87581595", "gare-de-cenon"},  // Cenon
    {"87741009", "gare-chambery-challes-les-eaux"},  // Chambéry - Challes-les-Eaux
    {"87394007", "gare-de-chartres"},  // Chartres
    {"87575142", "gare-de-chatellerault"},  // Châtellerault
    {"87545285", "gare-de-choisy-le-roi"},  // Choisy-le-Roi
    {"87484303", "gare-de-cholet"},  // Cholet
    {"87734004", "gare-de-clermont-ferrand"},  // Clermont-Ferrand
    {"87182014", "gare-de-colmar"},  // Colmar
    {"87381087", "gare-de-colombes"},  // Colombes
    {"87276691", "gare-de-compiegne"},  // Compiègne
    {"87276006", "gare-de-creil"},  // Creil
    {"87673202", "gare-de-dax"},  // Dax
    {"87713040", "gare-de-dijon"},  // Dijon
    {"87713412", "gare-de-dole"},  // Dole
    {"87345009", "gare-de-douai"},  // Douai
    {"87281006", "gare-de-dunkerque"},  // Dunkerque
    {"87144006", "gare-d-epinal"},  // Épinal
    {"87545137", "gare-d-etampes"},  // Étampes
    {"87681387", "gare-d-evry-courcouronnes"},  // Évry-Courcouronnes
    {"87747006", "gare-de-grenoble"},  // Grenoble
    {"87213058", "gare-d-haguenau"},  // Haguenau
    {"87281899", "gare-haussmann-saint-lazare"},  // Haussmann Saint-Lazare
    {"87486019", "gare-la-roche-sur-yon"},  // La Roche-sur-Yon
    {"87485003", "gare-la-rochelle"},  // La Rochelle
    {"87723478", "gare-la-tour-du-pin"},  // La Tour-du-Pin
    {"87473108", "gare-de-lamballe"},  // Lamballe
    {"87478404", "gare-de-laval"},  // Laval
    {"87396002", "gare-le-mans"},  // Le Mans
    {"87345025", "gare-de-lens"},  // Lens
    {"87543017", "gare-les-aubrais"},  // Les Aubrais
    {"87584052", "gare-de-libourne"},  // Libourne
    {"87682153", "gare-lieusaint-moissy"},  // Lieusaint - Moissy
    {"87223263", "gare-lille-europe"},  // Lille Europe
    {"87286005", "gare-lille-flandres"},  // Lille Flandres
    {"87592006", "gare-limoges-benedictins"},  // Limoges Bénédictins
    {"87723197", "gare-lyon-part-dieu"},  // Lyon Part Dieu
    {"87722025", "gare-lyon-perrache"},  // Lyon Perrache
    {"87725689", "gare-de-macon"},  // Mâcon
    {"87381509", "gare-de-mantes-la-jolie"},  // Mantes-la-Jolie
    {"87111849", "gare-marne-la-vallee-chessy"},  // Marne-la-Vallée Chessy
    {"87751008", "gare-marseille-saint-charles"},  // Marseille Saint-Charles
    {"87393579", "gare-massy-palaiseau"},  // Massy - Palaiseau
    {"87393702", "gare-massy-tgv"},  // Massy TGV
    {"87682005", "gare-de-melun"},  // Melun
    {"87192039", "gare-de-metz"},  // Metz
    {"87753004", "gare-de-miramas"},  // Miramas
    {"87684001", "gare-de-montargis"},  // Montargis
    {"87764001", "gare-de-montelimar"},  // Montélimar
    {"87773002", "gare-montpellier-saint-roch"},  // Montpellier Saint-Roch
    {"87688887", "gare-montpellier-sud-de-france"},  // Montpellier Sud de France
    {"87696328", "gare-moulins-sur-allier"},  // Moulins sur Allier
    {"87182063", "gare-de-mulhouse"},  // Mulhouse
    {"87141002", "gare-de-nancy"},  // Nancy
    {"87481002", "gare-de-nantes"},  // Nantes
    {"87781104", "gare-de-narbonne"},  // Narbonne
    {"87696005", "gare-de-nevers"},  // Nevers
    {"87756056", "gare-de-nice"},  // Nice
    {"87756353", "gare-nice-riquier"},  // Nice Riquier
    {"87775007", "gare-de-nimes"},  // Nîmes
    {"87703975", "gare-nimes-pont-du-gard"},  // Nîmes Pont du Gard
    {"87485300", "gare-de-niort"},  // Niort
    {"87113217", "gare-de-noisy-le-sec"},  // Noisy-le-Sec
    {"87543009", "gare-d-orleans"},  // Orléans
    {"87547000", "gare-austerlitz"},  // Paris Austerlitz
    {"87547026", "gare-austerlitz"},  // Paris Austerlitz
    {"87686667", "gare-de-bercy"},  // Paris Bercy Bourgogne - Pays d'Auvergne
    {"87113001", "gare-de-l-est"},  // Paris Est
    {"87686006", "gare-de-lyon"},  // Paris Gare de Lyon
    {"87686030", "gare-de-lyon"},  // Paris Gare de Lyon
    {"87271007", "gare-du-nord"},  // Paris Gare du Nord
    {"87271023", "gare-du-nord"},  // Paris Gare du Nord
    {"87271031", "gare-du-nord"},  // Paris Gare du Nord
    {"87391003", "gare-montparnasse"},  // Paris Montparnasse
    {"87391102", "gare-montparnasse"},  // Paris Montparnasse
    {"87391011", "gare-montparnasse"},  // Paris Montparnasse
    {"87384008", "gare-saint-lazare"},  // Paris Saint-Lazare
    {"87672006", "gare-de-pau"},  // Pau
    {"87784009", "gare-de-perpignan"},  // Perpignan
    {"87575001", "gare-de-poitiers"},  // Poitiers
    {"87474098", "gare-de-quimper"},  // Quimper
    {"87471300", "gare-de-redon"},  // Redon
    {"87171009", "gare-de-reims"},  // Reims
    {"87471003", "gare-de-rennes"},  // Rennes
    {"87411017", "gare-rouen-rive-droite"},  // Rouen Rive Droite
    {"87473009", "gare-de-saint-brieuc"},  // Saint-Brieuc
    {"87271015", "gare-de-saint-denis"},  // Saint-Denis
    {"87726000", "gare-saint-etienne-chateaucreux"},  // Saint-Étienne Châteaucreux
    {"87478107", "gare-de-saint-malo"},  // Saint-Malo
    {"87481705", "gare-de-saint-nazaire"},  // Saint-Nazaire
    {"87571240", "gare-de-saint-pierre-des-corps"},  // Saint-Pierre-des-Corps
    {"87296004", "gare-de-saint-quentin"},  // Saint-Quentin
    {"87487603", "gare-de-saumur"},  // Saumur
    {"87212225", "gare-de-saverne"},  // Saverne
    {"87773200", "gare-de-sete"},  // Sète
    {"87212027", "gare-de-strasbourg"},  // Strasbourg
    {"87671008", "gare-de-tarbes"},  // Tarbes
    {"87191007", "gare-de-thionville"},  // Thionville
    {"87755009", "gare-de-toulon"},  // Toulon
    {"87611004", "gare-toulouse-matabiau"},  // Toulouse Matabiau
    {"87286542", "gare-de-tourcoing"},  // Tourcoing
    {"87571000", "gare-de-tours"},  // Tours
    {"87118000", "gare-de-troyes"},  // Troyes
    {"87113712", "gare-val-de-fontenay"},  // Val de Fontenay
    {"87761007", "gare-de-valence"},  // Valence
    {"87343004", "gare-de-valenciennes"},  // Valenciennes
    {"87476606", "gare-de-vannes"},  // Vannes
    {"87415604", "gare-vernon-giverny"},  // Vernon - Giverny
    {"87393009", "gare-versailles-chantiers"},  // Versailles Chantiers
    {"87382861", "gare-versailles-rive-droite"},  // Versailles Rive Droite
    {"87732008", "gare-de-vichy"},  // Vichy
    {"87722587", "gare-de-vienne"},  // Vienne
    {"87576009", "gare-de-vierzon"},  // Vierzon
    {"87721332", "gare-de-villefranche-sur-saone"},  // Villefranche-sur-Saône
    {"87471508", "gare-de-vitre"},  // Vitré
    {"87413385", "gare-d-yvetot"},  // Yvetot
};
