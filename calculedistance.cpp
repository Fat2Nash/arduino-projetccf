/**
 * @brief Calcule la distance entre deux points géographiques en utilisant la formule de Haversine.
 *
 * @param lat1 Latitude du premier point en degrés.
 * @param lon1 Longitude du premier point en degrés.
 * @param lat2 Latitude du deuxième point en degrés.
 * @param lon2 Longitude du deuxième point en degrés.
 * @return La distance entre les deux points en kilomètres.
 */

#include <Arduino.h>
#include "calculedistance.h"

double calculateDistance(double lat1, double lon1, double lat2, double lon2) {
    // Convertir les latitudes et longitudes en radians
    lat1 = lat1 * (PI / 180.0);
    lat2 = lat2 * (PI / 180.0);
    lon1 = lon1 * (PI / 180.0);
    lon2 = lon2 * (PI / 180.0);

    // Appliquer la formule de Haversine
    double deltaLat = lat2 - lat1;
    double deltaLon = lon2 - lon1;
    double a = sin(deltaLat / 2) * sin(deltaLat / 2) +
               cos(lat1) * cos(lat2) * sin(deltaLon / 2) * sin(deltaLon / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));

    // Rayon de la Terre en kilomètres
    double distance = EARTH_RADIUS * c;

    return distance;
}
