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
    double distance;
    bool reconnu;       // false si distance > seuil
};

class RecognitionPipeline {
public:
    RecognitionPipeline(const std::string& cheminImages,
                        int taille = 128,
                        double seuil = 1500.0);

    void initialiser();   // Charge les images, entraîne PCA+LDA
    ResultatReconnaissance reconnaitre(const std::string& cheminImageTest);

    void setSeuil(double s) { seuil_ = s; }
    double getSeuil() const { return seuil_; }

private:
    std::string cheminImages_;
    std::vector<std::string> nomsClasses_;
    int taille_;
    double seuil_;

    Preprocesseur prep_;
    PCA* pca_ = nullptr;
    LDA* lda_ = nullptr;

    int nbClasses_;
    std::vector<Eigen::VectorXd> centresClasses_;
    Eigen::VectorXi labels_;
};
