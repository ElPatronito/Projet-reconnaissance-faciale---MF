#include <opencv2/opencv.hpp> //inclut la bibliothèque open cv pour avoir accès à cv::Mat 
#include <vector> //donne accès aux tableaux dynamiques
#include <string> //pour manipuler les chaînes de caractère
#include <filesystem>// pour parcourir les dossiers et lister les fichier > un seul dossier pour toutes les images
#include <Eigen/Dense> 
#include <map> //dictionnaire

namespace fs = std::filesystem;

class Preprocesseur {
private:
    int taille;     //taille cible 
    cv::Mat visageMoyen;       
    cv::Mat X;                      // matrice de données (une ligne = un visage)
    std::vector<std::string> labels; // label (nom) associé à chaque image
    std::vector<int> labelsNumeriques;
    std::map<std::string, int> correspondanceLabels;

public:
    Preprocesseur(int taille = 128);

    // ajoute les images du dossier dans la matrice
    void chargerDossier(const std::string& cheminDossier);

    // prétraite une seule image
    cv::Mat pretraiterImage(const cv::Mat& img);

    // Getters
    cv::Mat getX() const { return X; }
    cv::Mat getVisageMoyen() const { return visageMoyen; }
    std::vector<std::string> getLabels() const { return labels; }
    int getTaille() const { return taille; }

    Eigen::MatrixXd getXEigen() const;

    Eigen::VectorXi getLabelsNumeriquesEigen() const;

};