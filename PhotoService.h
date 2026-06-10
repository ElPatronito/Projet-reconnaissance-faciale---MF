#ifndef PHOTOSERVICE_H
#define PHOTOSERVICE_H

#include <iostream>
#include <filesystem>
#include <opencv2/opencv.hpp>
#include "../DatabaseManager.h"
#include "../models/Individu.h"
#include "../models/Photo.h"
#include "../repositories/IndividuSQL.h"
#include "../repositories/PhotoSQL.h"

const std::string CHEMIN_IMAGES = "/home/cytech/ING1/Projet_Reconnaissance_Faciale/Image/";

class PhotoService {
public:
    explicit PhotoService(DatabaseManager& db) : db_(db) {}

    // ── Ajoute une photo : vérifie, convertit, sauvegarde, insère en BDD ─
    bool ajouterPhoto(const std::string& cheminSource,
                      const std::string& nom,
                      const std::string& prenom,
                      const std::string& description) {

        if (!std::filesystem::exists(cheminSource)) {
            std::cerr << "Erreur : fichier introuvable → " << cheminSource << std::endl;
            return false;
        }

        IndividuSQL individuRepo(db_);
        PhotoSQL    photoRepo(db_);

        // 1. L'individu existe-t-il ?
        int individuId = individuRepo.findIdByNom(nom, prenom);
        if (individuId == -1) {
            Individu ind(nom, prenom);
            individuId = individuRepo.save(ind);
            std::cout << "Nouvel individu créé : " << prenom << " " << nom
                      << " (id=" << individuId << ")" << std::endl;
        }

        // 2. Crée le sous-dossier si nécessaire
        std::string dossier = CHEMIN_IMAGES + prenom + "/";
        std::filesystem::create_directories(dossier);

        // 3. Numéro automatique
        int numero = photoRepo.countByIndividu(individuId) + 1;
        std::string numero2 = (numero < 10) ? "0" + std::to_string(numero)
                                            : std::to_string(numero);
        std::string nomFichier  = "IMG_" + prenom + "_" + numero2 + ".png";
        std::string destination = dossier + nomFichier;

        // 4. Vérification doublon
        if (photoRepo.exists(destination)) {
            std::cout << "Photo déjà en BDD, insertion ignorée : " << destination << std::endl;
            return false;
        }

        // 5. Lecture JPG → sauvegarde PNG
        cv::Mat image = cv::imread(cheminSource);
        if (image.empty()) {
            std::cerr << "Erreur : impossible de lire → " << cheminSource << std::endl;
            return false;
        }
        cv::imwrite(destination, image);
        std::cout << "Image convertie et sauvegardée : " << destination << std::endl;

        // 6. Insertion en BDD
        Photo p(destination, description, individuId);
        photoRepo.save(p);
        std::cout << "Photo ajoutée en BDD." << std::endl;

        return true;
    }

    // ── Affiche les photos d'un individu dans le terminal ────────────────
    void afficherPhotos(const std::string& nom, const std::string& prenom) {
        IndividuSQL individuRepo(db_);
        PhotoSQL    photoRepo(db_);

        int individuId = individuRepo.findIdByNom(nom, prenom);
        if (individuId == -1) {
            std::cerr << "Individu introuvable : " << prenom << " " << nom << std::endl;
            return;
        }

        std::vector<Photo> photos = photoRepo.findByIndividu(individuId);
        if (photos.empty()) {
            std::cout << "Aucune photo trouvée pour " << prenom << " " << nom << std::endl;
            return;
        }

        std::cout << "\n── Photos de " << prenom << " " << nom << " ──" << std::endl;
        for (size_t i = 0; i < photos.size(); i++) {
            std::cout << "[" << i + 1 << "] "
                      << photos[i].getDescription() << " → "
                      << photos[i].getChemin() << std::endl;
        }

        // Boucle d'affichage OpenCV
        int choix = -1;
        while (choix != 0) {
            std::string input = lireLigne("\nNuméro de la photo à afficher (0 pour quitter) : ");
            try {
                choix = std::stoi(input);
            } catch (...) {
                std::cout << "Saisie invalide, entrez un nombre." << std::endl;
                continue;
            }

            if (choix == 0) break;

            if (choix < 1 || choix > static_cast<int>(photos.size())) {
                std::cout << "Numéro invalide." << std::endl;
                continue;
            }

            cv::Mat image = cv::imread(photos[choix - 1].getChemin());
            if (image.empty()) {
                std::cerr << "Impossible de charger l'image." << std::endl;
                continue;
            }

            std::cout << "\n── Infos ───────────────────────────" << std::endl;
            std::cout << "Individu    : " << prenom << " " << nom << std::endl;
            std::cout << "Description : " << photos[choix - 1].getDescription() << std::endl;
            std::cout << "Chemin      : " << photos[choix - 1].getChemin() << std::endl;
            std::cout << "────────────────────────────────────" << std::endl;

            cv::Mat imageAffichage;
            int largeurMax = 800;
            double ratio   = static_cast<double>(largeurMax) / image.cols;
            cv::Size taille(largeurMax, static_cast<int>(image.rows * ratio));
            cv::resize(image, imageAffichage, taille, 0, 0, cv::INTER_LINEAR);

            cv::imshow(prenom + " " + nom + " - " + photos[choix - 1].getDescription(),
                       imageAffichage);
            cv::waitKey(0);
            cv::destroyAllWindows();
        }
    }

private:
    DatabaseManager& db_;

    // ── Utilitaire de saisie ─────────────────────────────────────────────
    std::string lireLigne(const std::string& invite) {
        std::string valeur;
        std::cout << invite;
        std::getline(std::cin, valeur);
        return valeur;
    }
};

#endif
