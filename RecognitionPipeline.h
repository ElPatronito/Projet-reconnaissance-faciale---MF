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
    double      deuxiemeDistance;
    double      marge;
    std::string confiance;       // "forte", "moyenne" ou "faible"
    bool        reconnu;
    std::string cheminPhotoRef;
};

class RecognitionPipeline {
public:
    RecognitionPipeline(const std::string& cheminImages,
                        int taille = 128,
                        double seuil = 1968.83);
    ~RecognitionPipeline();

    void initialiser();
    ResultatReconnaissance reconnaitre(const std::string& cheminImageTest);

    void   setSeuil(double s) { seuil_ = s; }
    double getSeuil()   const { return seuil_; }

private:
    std::string cheminImages_;
    std::vector<std::string> nomsClasses_;
    std::vector<std::vector<std::string>> cheminsFichiers_;
    int    taille_;
    double seuil_;

    Preprocesseur prep_;
    PCA* pca_ = nullptr;
    LDA* lda_ = nullptr;

    int nbClasses_;
    std::vector<Eigen::VectorXd> centresClasses_;
    std::vector<Eigen::VectorXd> projectionsApprentissage_;
    std::vector<int>             labelsApprentissage_;
    std::vector<std::string>     cheminsApprentissage_;
    Eigen::VectorXi labels_;

    std::string trouverPhotoReference(const Eigen::VectorXd& yTest, int classeMin) const;
};