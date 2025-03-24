# change_detection

## Context: Ome2


## Description


## Configuration


## Utilisation


Paramètres :

* c [obligatoire] : chemin vers le fichier de configuration
* T [obligatoire] : thème (doit être parmi les valeurs : tn, hy, au, ib)
* op [obligatoire] : opération (doit être parmi les valeurs : diff, apply_diff)
* f [obligatoire] : feature à traiter
* cc [obligatoire] : code pays simple


Paramètres spécifiques à l'opération 'apply_diff' :

* ref [obligatoire] : schéma de la table de référence
* up [obligatoire] : schéma de la table mise à jour


Exemples d'appels:

~~~
bin/change_detection --c path/to/config/epg_parmaters.ini --op diff --T tn --f road_link --cc lu
bin/change_detection --c path/to/config/epg_parmaters.ini --op apply_diff --T tn --f road_link --cc lu --ref ref --up up
~~~