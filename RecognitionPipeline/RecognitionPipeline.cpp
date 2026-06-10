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

void RecognitionPipeline::initialiser() {
    // --- Chargement ---
    prep_.chargerDossier(cheminImages_);
    MatrixXd X      = prep_.getXEigen();
    labels_         = prep_.getLabelsNumeriquesEigen();
    nbClasses_      = labels_.maxCoeff() + 1;
    
    // --- Reconstruction des noms de classes dans l'ordre des indices ---
	std::map<std::string, int> correspondance = prep_.getCorrespondanceLabels();
	nomsClasses_.resize(nbClasses_);
	for (const auto& [nom, indice] : correspondance) {
		nomsClasses_[indice] = nom;
	}

	cout << "Classes détectées (" << nbClasses_ << ") :" << endl;
	for (int c = 0; c < nbClasses_; c++)
		cout << "  [" << c << "] " << nomsClasses_[c] << endl;

	cout << "Matrice X : " << X.rows() << " x " << X.cols() << endl;
	cout << "Nombre de labels : " << labels_.size() << endl;

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
    cout << "Projection ACP Z : " << Z.rows() << " x " << Z.cols() << endl;

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
    cout << "Projection LDA Y : " << Y.rows() << " x " << Y.cols() << endl;

    // --- Calcul des centres de classe ---
    centresClasses_.assign(nbClasses_, VectorXd::Zero(Y.rows()));
    vector<int> compteurs(nbClasses_, 0);

    for (int i = 0; i < Y.cols(); i++) {
        int c = labels_(i);
        centresClasses_[c] += Y.col(i);
        compteurs[c]++;
    }
    for (int c = 0; c < nbClasses_; c++) {
        if (compteurs[c] > 0)
            centresClasses_[c] /= compteurs[c];
    }

    cout << "Pipeline initialise avec " << nbClasses_ << " classes." << endl;
}

ResultatReconnaissance RecognitionPipeline::reconnaitre(const string& cheminImageTest) {
    if (!pca_ || !lda_)
        throw runtime_error("Pipeline non initialise. Appeler initialiser() d'abord.");

    // --- Chargement et prétraitement de l'image test ---
    cv::Mat imageTest = cv::imread(cheminImageTest);
    if (imageTest.empty())
        throw runtime_error("Impossible de charger l'image : " + cheminImageTest);

    cv::Mat vecteurTestCV = prep_.pretraiterImage(imageTest);

    VectorXd xTest(vecteurTestCV.cols);
    for (int j = 0; j < vecteurTestCV.cols; j++)
        xTest(j) = vecteurTestCV.at<double>(0, j);

    // --- Projection PCA puis LDA ---
    VectorXd zTest = pca_->projeterImageACP(xTest);
    VectorXd yTest = lda_->projeterImageLDA(zTest);

    // --- Recherche du plus proche centre ---
    double distanceMin = numeric_limits<double>::max();
    int indiceMin = -1;

    for (int c = 0; c < nbClasses_; c++) {
        double d = (yTest - centresClasses_[c]).norm();
        cout << "Distance vers " << nomsClasses_[c] << " : " << d << endl;
        if (d < distanceMin) {
            distanceMin = d;
            indiceMin = c;
        }
    }

    ResultatReconnaissance res;
    res.distance = distanceMin;
    res.reconnu  = (distanceMin <= seuil_);
    res.nom      = res.reconnu ? nomsClasses_[indiceMin] : "Inconnu";

    return res;
}
