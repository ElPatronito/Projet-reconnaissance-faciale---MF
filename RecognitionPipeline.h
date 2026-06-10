#pragma once
#include <string>
#include <vector>
#include <Eigen/Dense>
#include <opencv2/opencv.hpp>
#include "Preprocesseur.h"
#include "PCA.h"
#include "LDA.h"

struct ResultatReconnaissance {
    std::string nom;
    double      distance;
    bool        reconnu;
    double      seuilQuantile95;   // seuil calculé sur les distances connues
    double      confiance;         // pourcentage de confiance (0-100)
    std::string cheminPhotoRef;    // chemin vers la photo de référence la plus proche
};

class RecognitionPipeline {
public:
    RecognitionPipeline(const std::string& cheminImages,
                        int taille = 128,
                        double seuil = 1500.0);
    ~RecognitionPipeline();

    void initialiser();
    ResultatReconnaissance reconnaitre(const std::string& cheminImageTest);

    void setSeuil(double s) { seuil_ = s; }
    double getSeuil() const { return seuil_; }

private:
    std::string cheminImages_;
    std::vector<std::string> nomsClasses_;
    // chemins de toutes les images par classe : cheminsFichiers_[classe][image]
    std::vector<std::vector<std::string>> cheminsFichiers_;
    int taille_;
    double seuil_;

    Preprocesseur prep_;
    PCA* pca_ = nullptr;
    LDA* lda_ = nullptr;

    int nbClasses_;
    std::vector<Eigen::VectorXd> centresClasses_;
    // projections LDA de chaque image d'apprentissage
    std::vector<Eigen::VectorXd> projectionsApprentissage_;
    std::vector<int>             labelsApprentissage_;
    std::vector<std::string>     cheminsApprentissage_;
    Eigen::VectorXi labels_;

    double seuilQuantile95_ = 0.0;

    void calculerSeuilQuantile();
    std::string trouverPhotoReference(const Eigen::VectorXd& yTest, int classeMin) const;
    double calculerConfiance(double distance) const;
};