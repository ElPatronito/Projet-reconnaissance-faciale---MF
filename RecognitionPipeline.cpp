#include "RecognitionPipeline.h"
#include <iostream>
#include <limits>
#include <stdexcept>
#include <filesystem>
#include <algorithm>
#include <cmath>

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
    // --- Chargement ---
    prep_.chargerDossier(cheminImages_);
    MatrixXd X  = prep_.getXEigen();
    labels_     = prep_.getLabelsNumeriquesEigen();
    nbClasses_  = labels_.maxCoeff() + 1;

    // --- Noms de classes ---
    auto correspondance = prep_.getCorrespondanceLabels();
    nomsClasses_.resize(nbClasses_);
    for (const auto& [nom, indice] : correspondance)
        nomsClasses_[indice] = nom;

    // --- Chemins des fichiers par classe ---
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

    // --- Chemins de toutes les images d'apprentissage (dans l'ordre du prep) ---
    // On reconstruit l'ordre via le parcours récursif identique au Preprocesseur
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

    // --- PCA ---
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

    // --- LDA ---
    lda_ = new LDA(Z, labels_);
    lda_->ClasseMoyenne();
    lda_->MoyenneGlobale();
    lda_->MatriceDispersionIntra();
    lda_->MatriceDispersionInter();
    lda_->solutionLDA();
    lda_->Fisherfaces();
    lda_->projectionLDA();

    MatrixXd Y = lda_->getProjectionLDA();

    // --- Centres de classe ---
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

    // --- Projections individuelles pour la photo de référence ---
    projectionsApprentissage_.clear();
    for (int i = 0; i < Y.cols(); i++)
        projectionsApprentissage_.push_back(Y.col(i));

    // --- Seuil quantile 95% ---
    calculerSeuilQuantile();

    cout << "Pipeline initialisé. Seuil quantile 95% : " << seuilQuantile95_ << endl;
}

void RecognitionPipeline::calculerSeuilQuantile() {
    // On projette chaque image d'apprentissage et on calcule sa distance
    // au centre de SA classe → distribution des distances "connues"
    MatrixXd Y = lda_->getProjectionLDA();
    vector<double> distancesConnues;

    for (int i = 0; i < Y.cols(); i++) {
        int c = labels_(i);
        double d = (Y.col(i) - centresClasses_[c]).norm();
        distancesConnues.push_back(d);
    }

    sort(distancesConnues.begin(), distancesConnues.end());

    int idx = (int)ceil(0.95 * distancesConnues.size()) - 1;
    idx = max(0, min(idx, (int)distancesConnues.size() - 1));
    seuilQuantile95_ = distancesConnues[idx];
    seuil_ = seuilQuantile95_;
}

std::string RecognitionPipeline::trouverPhotoReference(
        const VectorXd& yTest, int classeMin) const {

    // Parmi toutes les images d'apprentissage de la classe gagnante,
    // on retourne celle dont la projection LDA est la plus proche de yTest
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

double RecognitionPipeline::calculerConfiance(double distance) const {
    // Confiance basée sur la position relative par rapport au seuil :
    // distance=0 → 100%, distance=seuil → 50%, distance>>seuil → 0%
    if (seuilQuantile95_ <= 0) return 0.0;
    double ratio = distance / seuilQuantile95_;
    double confiance = 100.0 * exp(-ratio * ratio / 2.0);
    return max(0.0, min(100.0, confiance));
}

ResultatReconnaissance RecognitionPipeline::reconnaitre(
        const string& cheminImageTest) {

    if (!pca_ || !lda_)
        throw runtime_error("Pipeline non initialisé. Appeler initialiser() d'abord.");

    cv::Mat imageTest = cv::imread(cheminImageTest);
    if (imageTest.empty())
        throw runtime_error("Impossible de charger l'image : " + cheminImageTest);

    cv::Mat vecteurTestCV = prep_.pretraiterImage(imageTest);

    VectorXd xTest(vecteurTestCV.cols);
    for (int j = 0; j < vecteurTestCV.cols; j++)
        xTest(j) = vecteurTestCV.at<double>(0, j);

    VectorXd zTest = pca_->projeterImageACP(xTest);
    VectorXd yTest = lda_->projeterImageLDA(zTest);

    double distanceMin = numeric_limits<double>::max();
    int    indiceMin   = -1;

    for (int c = 0; c < nbClasses_; c++) {
        double d = (yTest - centresClasses_[c]).norm();
        cout << "Distance vers " << nomsClasses_[c] << " : " << d << endl;
        if (d < distanceMin) {
            distanceMin = d;
            indiceMin   = c;
        }
    }

    ResultatReconnaissance res;
    res.distance        = distanceMin;
    res.seuilQuantile95 = seuilQuantile95_;
    res.reconnu         = (distanceMin <= seuil_);
    res.nom             = res.reconnu ? nomsClasses_[indiceMin] : "Inconnu";
    res.confiance       = calculerConfiance(distanceMin);
    res.cheminPhotoRef  = res.reconnu
                            ? trouverPhotoReference(yTest, indiceMin)
                            : "";
    return res;
}