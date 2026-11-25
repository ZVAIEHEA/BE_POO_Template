/*
  Coffre à code rythmique
  BE C++ / objets connectés — ESP8266 + shield Grove

  Principe : le capteur de son détecte des claquements de mains ; l'intervalle
  entre les claquements forme un "rythme" qui sert de mot de passe.
  Bon rythme  -> LCD vert, bips, le servo ouvre le verrou 5 s.
  Mauvais     -> LCD rouge, bip grave. 3 échecs -> coffre bloqué 30 s.

  Utilisation :
   - Au premier démarrage, la première séquence de claquements (3 minimum)
     est enregistrée comme code.
   - Moniteur série (115200) :  n = enregistrer un nouveau code
                                h = afficher l'historique des tentatives

  Réglages indispensables :
   - Outils > C++ Exceptions > Enabled
   - Installer la bibliothèque « Grove - LCD RGB Backlight » (Seeed)
   - Adapter les broches ci-dessous aux connecteurs de TON shield
*/
#include <vector>
#include "Peripheriques.h"
#include "Rythme.h"
#include "Coffre.h"

// --- Broches (à adapter aux étiquettes de ton shield Grove) ---
const uint8_t PIN_SON    = A0;   // capteur de son : entrée analogique
const uint8_t PIN_BUZZER = D5;
const uint8_t PIN_SERVO  = D7;
// L'écran LCD RGB se branche sur le connecteur I2C (D1/D2)

// --- Paramètres de détection (en ms) ---
const unsigned long SILENCE_FIN = 1500;   // sans claquement pendant ce temps = fin du rythme
const unsigned long ANTI_REBOND = 120;    // on ignore les échos juste après un claquement

// --- Objets ---
CapteurSon  son(PIN_SON);
Buzzer      buzzer(PIN_BUZZER);
Verrou      verrou(PIN_SERVO);
LedIntegree led;
EcranLCD    ecran;
Coffre      coffre;
std::vector<Peripherique*> inventaire;    // polymorphisme : tous vus comme des Peripherique

bool modeEnregistrement = true;           // vrai tant qu'aucun code n'existe

void afficherAccueil() {
  ecran.colorer(255, 255, 255);
  if (modeEnregistrement) ecran.afficher("Nouveau code :", "Claquez rythme");
  else                    ecran.afficher("Coffre ferme", "Claquez le code");
}

// Appelée juste après la détection du PREMIER claquement : enregistre les
// suivants jusqu'à un silence de SILENCE_FIN. Renvoie le rythme mesuré.
// (Le buzzer reste muet pendant l'écoute : il serait entendu par le micro.)
Rythme capturerSuite() {
  Rythme r;
  unsigned long dernier = millis();
  led.activer();
  while (millis() - dernier < SILENCE_FIN) {
    unsigned long maintenant = millis();
    if (maintenant - dernier > ANTI_REBOND) {
      led.desactiver();
      if (son.claquement()) {
        r += millis() - dernier;          // operator+= de Rythme
        dernier = millis();
        led.activer();                    // la LED confirme chaque claquement détecté
      }
    }
    yield();
  }
  led.desactiver();
  return r;
}

void ouvrirCoffre(const String& qui) {
  ecran.colorer(0, 255, 0);
  ecran.afficher("OUVERT", qui);
  buzzer.bip(1200, 120);
  delay(60);
  buzzer.bip(1800, 200);
  verrou.activer();
  delay(5000);
  verrou.desactiver();
}

void alerte(const char* titre, const char* detail) {
  ecran.colorer(255, 0, 0);
  ecran.afficher(titre, detail);
  buzzer.bip(200, 400);
}

void setup() {
  Serial.begin(115200);
  delay(200);

  inventaire = { &son, &buzzer, &verrou, &led, &ecran };
  for (Peripherique* p : inventaire) p->begin();

  Serial.printf("\n%d peripheriques :\n", Peripherique::nombre());
  for (Peripherique* p : inventaire) p->afficherCaracteristiques();   // liaison dynamique

  ecran.afficher("Calibrage...", "Silence svp");
  try {
    son.calibrer();
    Serial.printf("Calibrage OK, seuil = %d\n", son.getSeuil());
  } catch (const CapteurHorsService& e) {
    Serial.println(e.what());
    alerte("ERREUR", e.what());
    while (true) delay(1000);             // on s'arrête : inutile de continuer
  }
  afficherAccueil();
}

void loop() {
  // Commandes depuis le moniteur série
  while (Serial.available()) {
    char c = Serial.read();
    if (c == 'n') { modeEnregistrement = true; afficherAccueil(); }
    if (c == 'h') coffre.afficherHistorique();
  }

  if (!son.claquement()) return;          // rien d'audible : on continue d'écouter

  Rythme r = capturerSuite();
  Serial.print("Rythme capte : ");
  Serial << r;
  Serial.println();

  try {
    if (modeEnregistrement) {
      coffre.enregistrer(r);
      modeEnregistrement = false;
      ecran.colorer(0, 0, 255);
      ecran.afficher("Code enregistre", String(r.nbClaquements()) + " claquements");
      buzzer.bip(1500, 150);
      delay(2500);
    } else {
      String qui = coffre.verifier(r);
      ouvrirCoffre(qui);
    }
  }
  // On attrape d'abord les erreurs les plus précises, puis la classe de base
  catch (const CoffreBloque& e) {
    Serial.println(e.what());
    alerte("COFFRE BLOQUE", "Attendez 30 s");
    delay(3000);
  }
  catch (const CodeInvalide& e) {
    Serial.println(e.what());
    alerte("Code incorrect", Coffre::echecsTotaux > 0 ? "Reessayez" : "");
    delay(2000);
  }
  catch (const ErreurCoffre& e) {
    Serial.println(e.what());
    alerte("ERREUR", e.what());
    delay(2000);
  }

  afficherAccueil();
  delay(400);                             // laisse mourir les échos avant la prochaine écoute
}
