#ifndef CALCULATEDISTANCE_H
#define CALCULATEDISTANCE_H

#include <Arduino.h>

// Définition de la constante pour le rayon de la Terre en kilomètres

const double EARTH_RADIUS = 6371.0; // Rayon de la Terre en kilomètres

/**
 * @brief Calcule la distance entre deux points géographiques en utilisant la formule de Haversine.
 *
 * @param lat1 Latitude du premier point en degrés.
 * @param lon1 Longitude du premier point en degrés.
 * @param lat2 Latitude du deuxième point en degrés.
 * @param lon2 Longitude du deuxième point en degrés.
 * @return La distance entre les deux points en kilomètres.
 */
double calculateDistance(double lat1, double lon1, double lat2, double lon2);

#endif  // CALCULATEDISTANCE_H
