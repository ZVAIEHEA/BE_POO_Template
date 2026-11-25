#pragma once
#include <Arduino.h>
#include <vector>
#include <cmath>

// ---------------------------------------------------------------
//  Un Rythme = la liste des durées (en ms) entre claquements successifs.
//  Exemple : 4 claquements "court-court-long"  ->  {300, 300, 800}
// ---------------------------------------------------------------
class Rythme {
private:
  std::vector<unsigned long> intervalles;

public:
  // Écart maximal toléré entre deux rythmes. Il est mesuré sur la PROPORTION
  // de chaque intervalle dans la durée totale : on peut donc taper plus vite
  // ou plus lentement que lors de l'enregistrement, seul le "motif" compte.
  static constexpr float TOLERANCE = 0.12f;

  Rythme() {}

  // Ajoute un intervalle :  rythme += 350;
  Rythme& operator+=(unsigned long ms) {
    intervalles.push_back(ms);
    return *this;
  }

  // Deux rythmes sont égaux s'ils ont le même nombre d'intervalles
  // et les mêmes proportions (à la tolérance près).
  bool operator==(const Rythme& autre) const {
    if (intervalles.empty() || intervalles.size() != autre.intervalles.size()) return false;
    float t1 = (float)total();
    float t2 = (float)autre.total();
    for (size_t i = 0; i < intervalles.size(); i++) {
      float ecart = std::fabs(intervalles[i] / t1 - autre.intervalles[i] / t2);
      if (ecart > TOLERANCE) return false;
    }
    return true;
  }
  bool operator!=(const Rythme& autre) const { return !(*this == autre); }

  unsigned long total() const {
    unsigned long somme = 0;
    for (unsigned long v : intervalles) somme += v;
    return somme;
  }
  size_t nbIntervalles() const { return intervalles.size(); }
  size_t nbClaquements() const { return intervalles.empty() ? 0 : intervalles.size() + 1; }
  bool vide() const { return intervalles.empty(); }
  const std::vector<unsigned long>& get() const { return intervalles; }
};

// Affichage :  Serial << rythme;   ->   [310, 290, 820] ms
inline Print& operator<<(Print& out, const Rythme& r) {
  out.print("[");
  const std::vector<unsigned long>& v = r.get();
  for (size_t i = 0; i < v.size(); i++) {
    if (i) out.print(", ");
    out.print(v[i]);
  }
  out.print("] ms");
  return out;
}
