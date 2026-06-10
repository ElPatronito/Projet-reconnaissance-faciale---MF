#include <iostream>
#include <Eigen/Dense>
#include <limits>
#include <filesystem>
#include <algorithm>
#include <cmath>

#include "Preprocesseur.h"
#include "PCA.h"
#include "LDA.h"

namespace fs = std::filesystem;

using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXd;
using Eigen::VectorXi;

int main(){

    vector<string> nomsClasses;

    string cheminImages = "/home/cytech/C++/Projet/Images";
    string dossierTests = "/home/cytech/C++/Projet/ImageTest";

    Preprocesseur prep(128);

    prep.chargerDossier(cheminImages);

    nomsClasses = prep.getNomsClasses();

    MatrixXd X = prep.getXEigen();
    VectorXi labels = prep.getLabelsNumeriquesEigen();
    
    int nbClasses = labels.maxCoeff() + 1;

    PCA pca(X);

    pca.VisageMoyen();
    pca.Centrage();
    pca.MatriceDuale();
    pca.ValeursPropres();
    
    int kMax = X.cols() - nbClasses;

    pca.choixK(0.95, kMax);

    pca.EigenFaces();
    pca.projectionACP();

    MatrixXd Z = pca.getProjectionACP();

    LDA lda(Z, labels);

    lda.ClasseMoyenne();
    lda.MoyenneGlobale();
    lda.MatriceDispersionIntra();
    lda.MatriceDispersionInter();
    lda.solutionLDA();
    lda.Fisherfaces();
    lda.projectionLDA();

    MatrixXd Y = lda.getProjectionLDA();

    vector<VectorXd> centresClasses(
        nbClasses,
        VectorXd::Zero(Y.rows())
    );

    vector<int> compteurs(nbClasses,0);

    for(int i = 0; i < Y.cols(); i++){

        int c = labels(i);

        centresClasses[c] += Y.col(i);

        compteurs[c]++;
    }

    for(int c = 0; c < nbClasses; c++){

        centresClasses[c] /= compteurs[c];
    }

    vector<double> distancesConnues;
    vector<double> distancesInconnues;

    int nbTests = 0;
    int nbImagesIgnorees = 0;

    cout << endl;
    cout << "===== VALIDATION DU SEUIL =====" << endl;

    for(const auto& entree : fs::directory_iterator(dossierTests)){

        string cheminImage = entree.path().string();
        string nomFichier = entree.path().stem().string();

        cv::Mat imageTest = cv::imread(cheminImage);

        if(imageTest.empty()){
            nbImagesIgnorees++;
            continue;
        }

        cv::Mat vecteurTestCV = prep.pretraiterImage(imageTest);

        if(vecteurTestCV.empty()){
            nbImagesIgnorees++;
            continue;
        }

        VectorXd xTest(vecteurTestCV.cols);

        for(int j = 0; j < vecteurTestCV.cols; j++){
            xTest(j) = vecteurTestCV.at<double>(0,j);
        }

        VectorXd zTest = pca.projeterImageACP(xTest);
        VectorXd yTest = lda.projeterImageLDA(zTest);

        double distanceMin = numeric_limits<double>::max();
        int indiceMin = -1;

        for(int c = 0; c < nbClasses; c++){

            double distance = (yTest - centresClasses[c]).norm();

            if(distance < distanceMin){
                distanceMin = distance;
                indiceMin = c;
            }
        }

        string personnePredite = nomsClasses[indiceMin];

        bool estConnu = false;

        for(const string& nomClasse : nomsClasses){

            if(nomFichier.find(nomClasse) != string::npos){
                estConnu = true;
                break;
            }
        }

        if(estConnu){
            distancesConnues.push_back(distanceMin);
        }
        else{
            distancesInconnues.push_back(distanceMin);
        }

        cout << endl;
        cout << "Image : " << nomFichier << endl;
        cout << "Prediction : " << personnePredite << endl;
        cout << "Distance minimale : " << distanceMin << endl;

        if(estConnu){
            cout << "Type : visage connu" << endl;
        }
        else{
            cout << "Type : visage inconnu" << endl;
        }

        nbTests++;
    }

    sort(distancesConnues.begin(), distancesConnues.end());
    sort(distancesInconnues.begin(), distancesInconnues.end());

    cout << endl;
    cout << "===== DISTANCES VISAGES CONNUS =====" << endl;

    for(double d : distancesConnues){
        cout << d << endl;
    }

    cout << endl;
    cout << "===== DISTANCES VISAGES INCONNUS =====" << endl;

    for(double d : distancesInconnues){
        cout << d << endl;
    }

    cout << endl;
    cout << "===== BILAN VALIDATION =====" << endl;

    cout << "Nombre total d'images testees : "
         << nbTests
         << endl;

    cout << "Images ignorees : "
         << nbImagesIgnorees
         << endl;

    cout << "Nombre de visages connus : "
         << distancesConnues.size()
         << endl;

    cout << "Nombre de visages inconnus : "
         << distancesInconnues.size()
         << endl;

    if(!distancesConnues.empty()){

        int indiceQuantile =
            ceil(0.95 * distancesConnues.size()) - 1;

        if(indiceQuantile < 0){
            indiceQuantile = 0;
        }

        if(indiceQuantile >= distancesConnues.size()){
            indiceQuantile = distancesConnues.size() - 1;
        }

        double seuil95 = distancesConnues[indiceQuantile];

        cout << "Seuil au quantile 95% : "
             << seuil95
             << endl;

        cout << endl;
        cout << "Decision conseillee :" << endl;
        cout << "Si distanceMin <= "
             << seuil95
             << " alors visage accepte." << endl;

        cout << "Si distanceMin > "
             << seuil95
             << " alors visage rejete comme inconnu." << endl;
    }

    cout << "==============================" << endl;

    return 0;
}