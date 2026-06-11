#include "RecognitionPipeline.h"
#include <iostream>
#include <limits>
#include <stdexcept>
#include <filesystem>

namespace fs = std::filesystem;
using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXd;
using Eigen::VectorXi;

RecognitionPipeline::RecognitionPipeline(const string& cheminImages,
                                         int taille,
                                         double seuil)
    : cheminImages_(cheminImages),
      taille_(taille),
      seuil_(seuil),
      prep_(taille) {}

RecognitionPipeline::~RecognitionPipeline() {
    delete pca_;
    delete lda_;
}

void RecognitionPipeline::initialiser() {
    prep_.chargerDossier(cheminImages_);
    MatrixXd X  = prep_.getXEigen();
    labels_     = prep_.getLabelsNumeriquesEigen();
    nbClasses_  = labels_.maxCoeff() + 1;

    auto correspondance = prep_.getCorrespondanceLabels();
    nomsClasses_.resize(nbClasses_);
    for (const auto& [nom, indice] : correspondance)
        nomsClasses_[indice] = nom;

    // Chemins des fichiers par classe
    cheminsFichiers_.resize(nbClasses_);
    for (int c = 0; c < nbClasses_; c++) {
        string dossier = cheminImages_ + "/" + nomsClasses_[c];
        if (!fs::exists(dossier)) continue;
        for (const auto& entry : fs::directory_iterator(dossier)) {
            string ext = entry.path().extension().string();
            if (ext==".jpg"||ext==".jpeg"||ext==".png"||ext==".pgm")
                cheminsFichiers_[c].push_back(entry.path().string());
        }
    }

    // Chemins dans l'ordre du preprocesseur
    cheminsApprentissage_.clear();
    labelsApprentissage_.clear();
    for (const auto& entry : fs::recursive_directory_iterator(cheminImages_)) {
        string ext = entry.path().extension().string();
        if (ext!=".jpg"&&ext!=".jpeg"&&ext!=".png"&&ext!=".pgm") continue;
        string nomClasse = entry.path().parent_path().filename().string();
        if (correspondance.count(nomClasse)) {
            cheminsApprentissage_.push_back(entry.path().string());
            labelsApprentissage_.push_back(correspondance.at(nomClasse));
        }
    }

    cout << "Classes détectées (" << nbClasses_ << ") :" << endl;
    for (int c = 0; c < nbClasses_; c++)
        cout << "  [" << c << "] " << nomsClasses_[c] << endl;

    // PCA
    pca_ = new PCA(X);
    pca_->VisageMoyen();
    pca_->Centrage();
    pca_->MatriceDuale();
    pca_->ValeursPropres();
    int kMax = X.cols() - nbClasses_;
    pca_->choixK(0.95, kMax);
    pca_->EigenFaces();
    pca_->projectionACP();
    MatrixXd Z = pca_->getProjectionACP();

    // LDA
    lda_ = new LDA(Z, labels_);
    lda_->ClasseMoyenne();
    lda_->MoyenneGlobale();
    lda_->MatriceDispersionIntra();
    lda_->MatriceDispersionInter();
    lda_->solutionLDA();
    lda_->Fisherfaces();
    lda_->projectionLDA();
    MatrixXd Y = lda_->getProjectionLDA();

    // Centres de classe
    centresClasses_.assign(nbClasses_, VectorXd::Zero(Y.rows()));
    vector<int> compteurs(nbClasses_, 0);
    for (int i = 0; i < Y.cols(); i++) {
        int c = labels_(i);
        centresClasses_[c] += Y.col(i);
        compteurs[c]++;
    }
    for (int c = 0; c < nbClasses_; c++)
        if (compteurs[c] > 0)
            centresClasses_[c] /= compteurs[c];

    // Projections individuelles pour la photo de référence
    projectionsApprentissage_.clear();
    for (int i = 0; i < Y.cols(); i++)
        projectionsApprentissage_.push_back(Y.col(i));

    cout << "Pipeline initialisé avec " << nbClasses_ << " classes." << endl;
    cout << "Seuil de reconnaissance : " << seuil_ << endl;
}

std::string RecognitionPipeline::trouverPhotoReference(
        const VectorXd& yTest, int classeMin) const {
    double distMin = numeric_limits<double>::max();
    string cheminRef;
    for (int i = 0; i < (int)projectionsApprentissage_.size(); i++) {
        if (labelsApprentissage_[i] != classeMin) continue;
        double d = (yTest - projectionsApprentissage_[i]).norm();
        if (d < distMin) {
            distMin = d;
            if (i < (int)cheminsApprentissage_.size())
                cheminRef = cheminsApprentissage_[i];
        }
    }
    return cheminRef;
}

ResultatReconnaissance RecognitionPipeline::reconnaitre(
        const string& cheminImageTest) {

    if (!pca_ || !lda_)
        throw runtime_error("Pipeline non initialisé. Appeler initialiser() d'abord.");

    cv::Mat imageTest = cv::imread(cheminImageTest);
    if (imageTest.empty())
        throw runtime_error("Impossible de charger l'image : " + cheminImageTest);

    cv::Mat vecteurTestCV = prep_.pretraiterImage(imageTest);

    if(vecteurTestCV.empty()){

        throw runtime_error(
            "Aucun visage detecte dans l'image."
        );
    }
    
    VectorXd xTest(vecteurTestCV.cols);
    
    for (int j = 0; j < vecteurTestCV.cols; j++)
        xTest(j) = vecteurTestCV.at<double>(0, j);

    VectorXd zTest = pca_->projeterImageACP(xTest);
    VectorXd yTest = lda_->projeterImageLDA(zTest);

    // Recherche des deux meilleures distances (comme dans mainFinal.cpp)
    double distanceMin      = numeric_limits<double>::max();
    double deuxiemeDistance = numeric_limits<double>::max();
    int    indiceMin        = -1;

    for (int c = 0; c < nbClasses_; c++) {
        double d = (yTest - centresClasses_[c]).norm();
        cout << "Distance vers " << nomsClasses_[c] << " : " << d << endl;
        if (d < distanceMin) {
            deuxiemeDistance = distanceMin;
            distanceMin      = d;
            indiceMin        = c;
        } else if (d < deuxiemeDistance) {
            deuxiemeDistance = d;
        }
    }

    // Marge et confiance — logique identique à mainFinal.cpp
    double marge = deuxiemeDistance - distanceMin;
    string confiance;
    if      (marge > 1500) confiance = "forte";
    else if (marge > 700)  confiance = "moyenne";
    else                   confiance = "faible";

    ResultatReconnaissance res;
    res.distance        = distanceMin;
    res.deuxiemeDistance= deuxiemeDistance;
    res.marge           = marge;
    res.confiance       = confiance;
    res.reconnu         = (distanceMin <= seuil_);
    res.nom             = res.reconnu ? nomsClasses_[indiceMin] : "Inconnu";
    res.cheminPhotoRef  = res.reconnu
                            ? trouverPhotoReference(yTest, indiceMin)
                            : "";
    return res;
}