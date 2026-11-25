#pragma once
#include <exception>

// ---------------------------------------------------------------
//  Exceptions du projet.
//  ATTENTION : dans l'IDE Arduino, mettre  Outils > C++ Exceptions > Enabled
//  sinon try/catch ne compile pas pour l'ESP8266.
// ---------------------------------------------------------------

// Classe de base de toutes les erreurs du coffre
class ErreurCoffre : public std::exception {
protected:
  const char* message;
public:
  explicit ErreurCoffre(const char* msg) : message(msg) {}
  const char* what() const noexcept override { return message; }
};

// Le rythme tapé ne correspond à aucun code enregistré (ou il est inutilisable)
class CodeInvalide : public ErreurCoffre {
public:
  explicit CodeInvalide(const char* msg = "Code invalide") : ErreurCoffre(msg) {}
};

// Trop d'échecs de suite : le coffre est verrouillé un moment
class CoffreBloque : public ErreurCoffre {
public:
  CoffreBloque() : ErreurCoffre("Coffre bloque") {}
};

// Le capteur de son ne répond pas (débranché, mauvais connecteur...)
class CapteurHorsService : public ErreurCoffre {
public:
  CapteurHorsService() : ErreurCoffre("Capteur son HS") {}
};
