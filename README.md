# area_matching

## Context

Open Maps For Europe 2 est un projet qui a pour objectif de développer un nouveau processus de production dont la finalité est la construction d'un référentiel cartographique pan-européen à grande échelle (1:10 000).

L'élaboration de la chaîne de production a nécessité le développement d'un ensemble de composants logiciels qui constituent le projet [OME2](https://github.com/openmapsforeurope2/OME2).


## Description

Cette application fournit des fonctions utilitaires qui traite du calcul différentiel entre deux tables Postgresql.


## Fonctionnement

Deux traitements peuvent être réalisés grâce à cette application:

### 1. Le calcul du differentiel

Cette fonction réalise une comparaison entre deux versions d'une même table et identifie les objets ajoutés, supprimés et modifiés. Le résultat de ce calcul est enregistré dans une table Postgresql.

### 2. l'application du différentiel sur la table cible

Cette opération consiste à réaliser la mise à jour de la table cible en 'jouant' le différentiel précédemment calculé, c'est à dire en supprimant les objets obsolètes, en ajoutant les nouveaux objets et en mettant à jour les objets modifiés.
 

## Configuration

Les paramètres de configuration permettent de déterminer le comportement en définissant la distance seuil de détection d'une différence géométrique ainsi que les champs à ignorer lors de la comparaison attributaire. Il est possible de définir un paramètrage différent pour chaque thème.

Certains champs peuvent être défini comme impactant pour le processus de raccordement au frontières. Si une différence est détectée sur l'un de ces champs cela est renseigné dans la table résultat. Les objets concernés seront par la suite utilisés pour le calcul des zones de mise à jour et le processus de raccordement leur sera donc appliqué.

On trouve dans le [dossier de configuration](https://github.com/openmapsforeurope2/area_matching/tree/main/config) les fichiers suivants :
- epg_parameters.ini : regroupe des paramètres de base issus de la bibliothèque libepg qui constitue le socle de développement l'outil. Ce fichier est aussi le fichier chapeau qui pointe vers les autres fichiers de configurations.
- db_conf.ini : informations de connexion à la base de données.
- theme_parameters.ini : configuration des paramètres spécifiques à l'application.


## Utilisation

L'outil s'utilise en ligne de commande.

Paramètres communs à toutes les opérations :

* c [obligatoire] : chemin vers le fichier de configuration
* T [obligatoire] : thème (doit être parmi les valeurs : tn, hy, au, ib)
* op [obligatoire] : opération (doit être parmi les valeurs : diff, apply_diff)
* f [obligatoire] : feature à traiter
* cc [obligatoire] : code pays simple

<br>

Paramètres spécifiques à l'opération 'apply_diff' :

* ref [obligatoire] : schéma de la table de référence
* up [obligatoire] : schéma de la table mise à jour


Exemples d'appel pour calculer un différentiel entre la table de référence et la table de mise à jour :
~~~
bin/change_detection --c path/to/config/epg_parmaters.ini --op diff --T tn --f road_link --cc lu
~~~

Exemples d'appel pour appliquer le différentiel sur la table de référence :
~~~
bin/change_detection --c path/to/config/epg_parmaters.ini --op apply_diff --T tn --f road_link --cc lu --ref ref --up up
~~~

