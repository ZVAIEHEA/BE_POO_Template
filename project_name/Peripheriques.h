#pragma once
#include <Arduino.h>
#include <Servo.h>
#include <rgb_lcd.h>      // bibliothèque « Grove - LCD RGB Backlight » (Seeed)
#include "Erreurs.h"

// ===============================================================
//  Hiérarchie des périphériques
//
//  Peripherique (abstraite)
//   ├─ Capteur (abstraite)      lire() = 0
//   │    └─ CapteurSon
//   └─ Actionneur (abstraite)   activer() = 0 / desactiver() = 0
//        ├─ LedIntegree
//        ├─ Buzzer
//        ├─ Verrou (servomoteur)
//        └─ EcranLCD  (hérite aussi de Coloriable : héritage multiple)
// ===============================================================

// ---------- Racine : tout ce qui est branché sur la carte ----------
class Peripherique {
protected:
  const char* nom;
  uint8_t broche;
  inline static int compteur = 0;           // variable de classe : nombre de périphériques

public:
  Peripherique(const char* n, uint8_t b) : nom(n), broche(b) { compteur++; }
  virtual ~Peripherique() { compteur--; }

  virtual void begin() {}                   // initialisation matérielle (appelée dans setup)
  virtual void afficherCaracteristiques() const = 0;   // virtuelle pure => classe abstraite
  static int nombre() { return compteur; }
};

// Corps de la méthode pure : les classes filles la réutilisent puis la complètent
inline void Peripherique::afficherCaracteristiques() const {
  Serial.printf("- %s (broche %d)", nom, broche);
}

// ---------- Capteurs ----------
class Capteur : public Peripherique {
public:
  Capteur(const char* n, uint8_t b) : Peripherique(n, b) {}
  virtual int lire() = 0;                   // valeur brute du capteur
};

class CapteurSon : public Capteur {
private:
  int seuil = 150;                          // amplitude minimale d'un claquement

public:
  static constexpr int MARGE = 60;          // marge ajoutée au bruit ambiant

  CapteurSon(uint8_t b) : Capteur("Capteur de son", b) {}

  int lire() override { return analogRead(broche); }

  // Mesure le bruit ambiant (~0,5 s, silence demandé) et règle le seuil.
  // Lève CapteurHorsService si la valeur lue est aberrante (capteur débranché).
  void calibrer() {
    long somme = 0;
    int n = 0, mn = 1023, mx = 0;
    unsigned long debut = millis();
    while (millis() - debut < 500) {
      int v = lire();
      somme += v; n++;
      mn = min(mn, v);
      mx = max(mx, v);
      delay(1);
    }
    int moyenne = somme / n;
    if (moyenne < 5 || moyenne > 1018) throw CapteurHorsService();
    seuil = (mx - mn) + MARGE;
  }

  // Amplitude crête à crête du son sur une courte fenêtre (~5 ms)
  int amplitude() {
    int mn = 1023, mx = 0;
    unsigned long debut = millis();
    while (millis() - debut < 5) {
      int v = lire();
      mn = min(mn, v);
      mx = max(mx, v);
    }
    return mx - mn;
  }

  bool claquement() { return amplitude() > seuil; }
  int getSeuil() const { return seuil; }

  void afficherCaracteristiques() const override {
    Peripherique::afficherCaracteristiques();
    Serial.printf(" | seuil de claquement : %d\n", seuil);
  }
};

// ---------- Actionneurs ----------
class Actionneur : public Peripherique {
public:
  Actionneur(const char* n, uint8_t b) : Peripherique(n, b) {}
  virtual void activer() = 0;
  virtual void desactiver() = 0;
};

class LedIntegree : public Actionneur {
public:
  LedIntegree() : Actionneur("LED integree", LED_BUILTIN) {}
  void begin() override { pinMode(broche, OUTPUT); desactiver(); }
  void activer() override { digitalWrite(broche, LOW); }      // logique inversée sur ESP8266
  void desactiver() override { digitalWrite(broche, HIGH); }
  void afficherCaracteristiques() const override {
    Peripherique::afficherCaracteristiques();
    Serial.println(" | logique inversee");
  }
};

class Buzzer : public Actionneur {
public:
  Buzzer(uint8_t b) : Actionneur("Buzzer", b) {}
  void begin() override { pinMode(broche, OUTPUT); desactiver(); }
  void activer() override { tone(broche, 1000); }
  void desactiver() override { noTone(broche); }
  void bip(unsigned int frequence, unsigned int dureeMs) {
    tone(broche, frequence);
    delay(dureeMs);
    noTone(broche);
  }
  void afficherCaracteristiques() const override {
    Peripherique::afficherCaracteristiques();
    Serial.println(" | sortie sonore");
  }
};

// Le servomoteur joue le rôle de verrou : activer = ouvrir, desactiver = fermer
class Verrou : public Actionneur {
private:
  Servo servo;
public:
  static constexpr int ANGLE_FERME = 0;
  static constexpr int ANGLE_OUVERT = 90;

  Verrou(uint8_t b) : Actionneur("Verrou (servo)", b) {}
  void begin() override { servo.attach(broche); desactiver(); }
  void activer() override { servo.write(ANGLE_OUVERT); }
  void desactiver() override { servo.write(ANGLE_FERME); }
  void afficherCaracteristiques() const override {
    Peripherique::afficherCaracteristiques();
    Serial.printf(" | ferme=%d deg, ouvert=%d deg\n", ANGLE_FERME, ANGLE_OUVERT);
  }
};

// ---------- Héritage multiple : l'écran est un actionneur ET est coloriable ----------
class Coloriable {
protected:
  uint8_t r = 255, g = 255, b = 255;
public:
  void setCouleur(uint8_t R, uint8_t G, uint8_t B) { r = R; g = G; b = B; }
  uint8_t getR() const { return r; }
  uint8_t getG() const { return g; }
  uint8_t getB() const { return b; }
};

class EcranLCD : public Actionneur, public Coloriable {
private:
  rgb_lcd lcd;
public:
  EcranLCD() : Actionneur("Ecran LCD RGB", 0) {}      // relié au bus I2C (D1/D2)

  void begin() override { lcd.begin(16, 2); activer(); }
  void activer() override { lcd.setRGB(r, g, b); }    // applique la couleur courante
  void desactiver() override { lcd.setRGB(0, 0, 0); }

  void colorer(uint8_t R, uint8_t G, uint8_t B) { setCouleur(R, G, B); activer(); }

  // Affiche deux lignes (16 caractères max chacune)
  void afficher(const String& ligne1, const String& ligne2 = "") {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(ligne1.substring(0, 16));
    lcd.setCursor(0, 1);
    lcd.print(ligne2.substring(0, 16));
  }

  void afficherCaracteristiques() const override {
    Peripherique::afficherCaracteristiques();
    Serial.printf(" | I2C, couleur (%d,%d,%d)\n", r, g, b);
  }
};
