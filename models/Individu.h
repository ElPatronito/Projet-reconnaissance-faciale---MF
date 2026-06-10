#ifndef INDIVIDU_H
#define INDIVIDU_H

#include <string>
using namespace std;

class Individu {
private:
    int    idPersonne;    
    string nom;
    string prenom;

public:
    Individu() : idPersonne(0), nom(""), prenom("") {}
    Individu(const string& nom, const string& prenom)
        : idPersonne(0), nom(nom), prenom(prenom) {}

    int    getId()     const { return idPersonne; }
    string getNom()    const { return nom; }
    string getPrenom() const { return prenom; }

    void setId    (int i)           { idPersonne = i; }
    void setNom   (const string& n) { nom        = n; }
    void setPrenom(const string& p) { prenom     = p; }
};

#endif

