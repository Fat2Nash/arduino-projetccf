#include <Arduino.h>

const int analogPin = A0; // Entrée analogique pour lire la tension
const float Vref = 3.3;   // Tension de référence de l'ADC (3.3V sur l'Arduino MKR WAN 1310)
const int R1 = 3300;      // Résistance R1 du diviseur de tension (en ohms)
const int R2 = 470;       // Résistance R2 du diviseur de tension (en ohms)

// Déclaration de la fonction sendLoRaWANMessage
extern void sendLoRaWANMessage(String data);
extern String idEngin;
extern double currentLat;
extern double currentLon;
bool alertSent = false;
extern unsigned long tempsFonctionnement;

// Déclaration de la variable typeAlerte
int typeAlerte = 0;

void checkBatteryVoltage()
{
  int analogValue = analogRead(analogPin);                        // Lire la valeur analogique definie
  float voltage = analogValue * (Vref / 1023.0) * (R1 + R2) / R2; // Convertir la valeur analogique en tension

  if (voltage < 20)
  {
    typeAlerte = 2;
    String Alerte = "Alerte: Tension batterie trop faible, valeur = " + String(voltage, 1) + " V";
    String dataToSend = "[" + idEngin + " , " + String(currentLat, 6) + " , " + String(currentLon, 6) + " , " + String(tempsFonctionnement) + " ," + typeAlerte + "]";

    if (alertSent == false)
    {

      Serial.println(Alerte);
      sendLoRaWANMessage(dataToSend); // Envoyer l'alerte sur LoRaWAN
      alertSent = true;               // Mettre à jour la variable d'état pour indiquer que l'alerte a été envoyée
    }
  }
  else
  {
    typeAlerte = 0;
    alertSent = false; // Réinitialiser la variable d'état si la tension est de nouveau normale
  }
}