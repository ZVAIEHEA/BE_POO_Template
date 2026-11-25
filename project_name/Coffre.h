#pragma once
#include <Arduino.h>
#include <map>
#include <list>
#include "Rythme.h"
#include "Erreurs.h"

// ---------------------------------------------------------------
//  Le coffre : mémorise des codes rythmiques et vérifie les tentatives.
//   - STL : map (codes par nom), list (historique des tentatives)
//   - Exceptions : CodeInvalide, CoffreBloque
//   - Variable de classe : echecsTotaux
// ---------------------------------------------------------------
class Coffre {
private:
  std::map<String, Rythme> codes;       // codes enregistrés, indexés par nom
  std::list<String> historique;         // dernières tentatives (10 max)
  int echecsConsecutifs = 0;
  unsigned long finBlocage = 0;         // instant (millis) de fin de blocage

  void journaliser(const String& ligne) {
    historique.push_back(String(millis() / 1000) + " s : " + ligne);
    if (historique.size() > 10) historique.pop_front();
  }

public:
  inline static int echecsTotaux = 0;                    // variable de classe
  static constexpr int MAX_ECHECS = 3;
  static constexpr unsigned long DUREE_BLOCAGE = 30000;  // 30 s

  bool possedeCode() const { return !codes.empty(); }

  // Enregistre un nouveau code. Lève CodeInvalide s'il est trop court.
  void enregistrer(const Rythme& r) {
    if (r.nbClaquements() < 3) throw CodeInvalide("Code trop court");
    String nom = "code" + String(codes.size() + 1);
    codes[nom] = r;
    journaliser("Enregistre " + nom);
  }

  // Vérifie une tentative. Renvoie le nom du code reconnu, sinon lève :
  //   CoffreBloque  si le coffre est verrouillé (ou le devient)
  //   CodeInvalide  si aucun code ne correspond
  String verifier(const Rythme& r) {
    if (millis() < finBlocage) throw CoffreBloque();

    for (const auto& kv : codes) {
      if (kv.second == r) {                 // operator== de Rythme
        echecsConsecutifs = 0;
        journaliser("Ouvert avec " + kv.first);
        return kv.first;
      }
    }

    echecsConsecutifs++;
    echecsTotaux++;
    journaliser("Echec " + String(echecsConsecutifs) + "/" + String(MAX_ECHECS));
    if (echecsConsecutifs >= MAX_ECHECS) {
      finBlocage = millis() + DUREE_BLOCAGE;
      echecsConsecutifs = 0;
      journaliser("Coffre bloque 30 s");
      throw CoffreBloque();
    }
    throw CodeInvalide();
  }

  void afficherHistorique() const {
    Serial.println("--- Historique ---");
    for (const auto& ligne : historique) Serial.println(ligne);
    Serial.printf("Echecs totaux : %d\n", echecsTotaux);
  }
};
