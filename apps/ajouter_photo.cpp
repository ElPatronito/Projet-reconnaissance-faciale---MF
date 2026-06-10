#include <iostream>
#include "../DatabaseManager.h"
#include "../services/PhotoService.h"

// ── Utilitaire de saisie ─────────────────────────────────────────────
std::string lireLigne(const std::string& invite) {
    std::string valeur;
    std::cout << invite;
    std::getline(std::cin, valeur);
    return valeur;
}

// ── Menu : ajouter une photo ─────────────────────────────────────────
void menuAjouter(PhotoService& service) {
    std::cout << "\n── Ajouter une photo ───────────────" << std::endl;
    std::string chemin      = lireLigne("Chemin complet de la photo  : ");
    std::string nom         = lireLigne("Nom                         : ");
    std::string prenom      = lireLigne("Prénom                      : ");
    std::string description = lireLigne("Description (neutre, lunettes...) : ");

    service.ajouterPhoto(chemin, nom, prenom, description);
}

// ── Menu : afficher les photos d'un individu ─────────────────────────
void menuAfficher(PhotoService& service) {
    std::cout << "\n── Afficher les photos ─────────────" << std::endl;
    std::string nom    = lireLigne("Nom    : ");
    std::string prenom = lireLigne("Prénom : ");

    service.afficherPhotos(nom, prenom);
}

// ── Point d'entrée ───────────────────────────────────────────────────
int main() {
    DatabaseManager db;
    PhotoService    service(db);

    int choix = -1;
    do {
        std::cout << "\n── Menu Photos ─────────────────────" << std::endl;
        std::cout << "[1] Ajouter une photo"                  << std::endl;
        std::cout << "[2] Afficher les photos d'un individu"  << std::endl;
        std::cout << "[0] Quitter"                            << std::endl;

        std::string input = lireLigne("Votre choix : ");
        try {
            choix = std::stoi(input);
        } catch (...) {
            std::cout << "Saisie invalide, entrez 0, 1 ou 2." << std::endl;
            continue;
        }

        switch (choix) {
            case 1: menuAjouter(service);  break;
            case 2: menuAfficher(service); break;
            case 0: std::cout << "Au revoir !" << std::endl; break;
            default: std::cout << "Choix invalide." << std::endl;
        }
    } while (choix != 0);

    return 0;
}
