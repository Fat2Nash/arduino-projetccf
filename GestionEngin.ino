// Last update: 18/06/2024
// Author:
//      - Quentin Legros
// Description:
//      - Code permettant de récupérer les données GPS et de les envoyer sur le réseau LoRaWAN
//      - Le code permet également de calculer le temps de fonctionnement du module
//      - Le code est basé sur le code de base fourni par Arduino pour le module MKR WAN 1310
//      - Le code est basé sur le code fourni par TinyGPS++ pour récupérer les données GPS
//      - Le code est basé sur le code fourni par Arduino pour envoyer des données sur le réseau LoRaWAN
//

#include <MKRWAN.h>
#include <TinyGPS++.h>
#include <String.h>
#include "calculedistance.h"
#include "alerte.h"
#include "arduino_secrets.h"

TinyGPSPlus gps;
LoRaModem modem;

// Sensitive data from arduino_secrets.h
String appEui = SECRET_APP_EUI;
String appKey = SECRET_APP_KEY;

// Variables pour calculer la distance
double previousLat = 0.0;
double previousLon = 0.0;
double currentLat = 0.0;
double currentLon = 0.0;
double distance = 0.0;

// Bouton variables
const int pinBouton = A1;
unsigned long tempsHaut = 0;
unsigned long tempsBas = 0;
unsigned long tempsFonctionnement = 0; // Temps de fonctionnement total en secondes
bool dernierEtatBouton = LOW;


// ID de l'engin
String idEngin = "1";

void setup()
{
    Serial.begin(9600);
    pinMode(pinBouton, INPUT);

    while (!Serial);

    // Initialisation du module LoRa et connexion au réseau
    if (!modem.begin(EU868))
    {
        Serial.println("Échec du démarrage du module");
        while (1);
    };

    // Initialisation du GPS
    Serial1.begin(9600);
    Serial.println("GPS initialisé");

    // Affichage des informations du module LoRa
    Serial.print("La version de votre module est : ");
    Serial.println(modem.version());
    Serial.print("L'EUI de votre appareil est le suivant : ");
    Serial.println(modem.deviceEUI());

    // Connexion au réseau LoRaWAN
    int connected = modem.joinOTAA(appEui, appKey);
    if (!connected)
    {
        Serial.println("Quelque chose s'est mal passé ; êtes-vous à l'intérieur ? Rapprochez-vous d'une fenêtre et réessayez.");
        while (1);
    }
    // Régler l'intervalle d'interrogation à 60 secondes.
    modem.minPollInterval(60);
}

void loop()
{
    verifierEtatBouton();
    // Attendez que les données soient disponibles
    while (Serial1.available() > 0)
    {
        // Obtenir les données du GPS
        if (gps.encode(Serial1.read()))
        {
            verifierEtatBouton(); // Vérifier à nouveau l'état du bouton après la mise à jour GPS
            displayInfo();
            checkBatteryVoltage();
            delay(5000); // Attente pour éviter la surcharge de messages
        }
    }
    // Vérifier la tension de la batterie régulièrement
    checkBatteryVoltage();
}

void displayInfo()
{
    if (gps.location.isValid())
    {
        // Obtenir la latitude et la longitude actuelles
        currentLat = gps.location.lat();
        currentLon = gps.location.lng();

        // Calculer la distance en utilisant les coordonnées précédentes et actuelles
        distance = calculateDistance(previousLat, previousLon, currentLat, currentLon);

        // Mise à jour des coordonnées précédentes pour le calcul suivant
        previousLat = currentLat;
        previousLon = currentLon;

        // Imprimer la distance calculée sur le moniteur série
        Serial.print("Distance: ");
        Serial.println(distance, 2);

        // Vérifier si la distance est supérieure à 50 mètres
        if (distance > 50.0)
        {
            // Préparer les données à envoyer [idEngin, latitude, longitude, temps_fonctionnement, alerte]
            String dataToSend = "[" + idEngin + " , " + String(currentLat, 6) + " , " + String(currentLon, 6) + " , " + String(tempsFonctionnement) + " , "+typeAlerte+"]";

            // Envoi de données sur LoRaWAN
            sendLoRaWANMessage(dataToSend);
        }
        else
        {
            // La distance est inférieure à 50 mètres, il n'est pas nécessaire d'envoyer des données.
            Serial.println("La distance est inférieure à 50 mètres, les données ne sont pas envoyées.");
            return;
        }
    }
    else
    {
        Serial.println("Location: Not Available");
    }
}

void sendLoRaWANMessage(String data)
{
    int err;
    // Envoyer un message sur le réseau LoRaWAN
    modem.beginPacket();
    modem.print(data);
    err = modem.endPacket(true);
    if (err > 0)
    {
        String message = " Message envoyé Correctement! {";
        message += data;
        message += "}";
        Serial.println(message);
        return;
    }
    else
    {
        Serial.println(" ------------------------ ");
        Serial.println("Erreur dans l'envoi du message :(");
        Serial.println("(vous pouvez envoyer un nombre limité de messages par minute, en fonction de la puissance du signal");
        Serial.println("il peut varier de 1 message toutes les quelques secondes à 1 message toutes les minutes)");
        Serial.println(" ------------------------ ");
        return;
    }
}

void verifierEtatBouton()
{
    unsigned long tempsActuel = millis();
    int etatBouton = analogRead(pinBouton) > 512 ? HIGH : LOW;

    if (etatBouton != dernierEtatBouton)
    {
        if (etatBouton == HIGH)
        {
            tempsHaut = tempsActuel;
            Serial.println(" ------------------------ ");
            Serial.println("Etat engin ON: ");
            Serial.println(" ------------------------ ");
        }
        else
        {
            tempsBas = tempsActuel;
            Serial.println(" ------------------------ ");
            Serial.print("Etat engin OFF: ");
            unsigned long intervalle = tempsBas - tempsHaut;
            tempsFonctionnement = intervalle / 1000; // Ajouter le temps de fonctionnement en secondes
            Serial.print("Temps total de fonctionnement : ");
            Serial.print(tempsFonctionnement);
            Serial.println(" secondes");
            Serial.println(" ------------------------ ");

            // Préparer les données à envoyer [idEngin, latitude, longitude, temps_fonctionnement, alerte]
            String dataToSend = "[" + idEngin + " , " + String(currentLat, 6) + " , " + String(currentLon, 6) + " , " + String(tempsFonctionnement) + " , "+typeAlerte+"]";
            sendLoRaWANMessage(dataToSend);
            tempsFonctionnement = 0;
        }
        dernierEtatBouton = etatBouton;
    }
}
