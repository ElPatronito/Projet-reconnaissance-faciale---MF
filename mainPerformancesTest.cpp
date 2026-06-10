#include <iostream>
#include <Eigen/Dense>
#include <limits>

#include "Preprocesseur.h"
#include "PCA.h"
#include "LDA.h"

#include <filesystem>

namespace fs = std::filesystem;

using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXi;

int main(){

    vector<string> nomsClasses;

    string cheminImages = "/home/cytech/C++/Projet/Images";

    Preprocesseur prep(128);

    prep.chargerDossier(cheminImages);

    nomsClasses = prep.getNomsClasses();

    vector<string> nomsImages = prep.getLabels();

    MatrixXd X = prep.getXEigen();
    VectorXi labels = prep.getLabelsNumeriquesEigen();
    
    int nbClasses = labels.maxCoeff() + 1;

    cout << "Matrice X : "
         << X.rows()
         << " x "
         << X.cols()
         << endl;

    cout << "Nombre de labels : "
         << labels.size()
         << endl;

    cout << "Nombre de classes : "
     << nbClasses
     << endl;

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

    cout << "Projection ACP Z : "
         << Z.rows()
         << " x "
         << Z.cols()
         << endl;

    LDA lda(Z, labels);

    lda.ClasseMoyenne();
    lda.MoyenneGlobale();
    lda.MatriceDispersionIntra();
    lda.MatriceDispersionInter();
    lda.solutionLDA();
    lda.Fisherfaces();
    lda.projectionLDA();

    MatrixXd Y = lda.getProjectionLDA();

    vector<VectorXd> centresClasses(nbClasses,VectorXd::Zero(Y.rows()));

    vector<int> compteurs(nbClasses,0);

    for(int i = 0; i < Y.cols(); i++){

        int c = labels(i);

        centresClasses[c] += Y.col(i);

        compteurs[c]++;
    }

    for(int c = 0; c < nbClasses; c++){

        centresClasses[c] /= compteurs[c];
    }
    
    string dossierTests = "/home/cytech/C++/Projet/ImageTest";


    double seuilReconnaissance = 1968.83;

    int nbTests = 0;
    int nbCorrects = 0;

    int nbConnus = 0;
    int nbConnusCorrects = 0;

    int nbInconnus = 0;
    int nbInconnusCorrects = 0;

    for(const auto& entree : fs::directory_iterator(dossierTests)){

        string cheminImage = entree.path().string();
        string nomFichier = entree.path().stem().string();

        cv::Mat imageTest = cv::imread(cheminImage);

        if(imageTest.empty()){
            continue;
        }

        cv::Mat vecteurTestCV = prep.pretraiterImage(imageTest);

        if(vecteurTestCV.empty()){
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

        bool predictionCorrecte =
            nomFichier.find(personnePredite) != string::npos;

        bool accepte =
            distanceMin <= seuilReconnaissance;

        bool correct = false;

        if(estConnu){

            nbConnus++;

            correct = accepte && predictionCorrecte;

            if(correct){
                nbConnusCorrects++;
            }
        }
        else{

            nbInconnus++;

            correct = !accepte;

            if(correct){
                nbInconnusCorrects++;
            }
        }

        if(correct){
            nbCorrects++;
        }

        nbTests++;

        cout << endl;
        cout << "Image test : " << nomFichier << endl;
        cout << "Type attendu : " << (estConnu ? "connu" : "inconnu") << endl;
        cout << "Prediction candidate : " << personnePredite << endl;
        cout << "Distance minimale : " << distanceMin << endl;

        if(accepte){
            cout << "Decision : visage accepte" << endl;
        }
        else{
            cout << "Decision : visage rejete comme inconnu" << endl;
        }

        if(correct){
            cout << "RESULTAT : CORRECT" << endl;
        }
        else{
            cout << "RESULTAT : ERREUR" << endl;
        }
    }

    cout << endl;
    cout << "=========================" << endl;

    cout << "Nombre total de tests : "
        << nbTests
        << endl;

    cout << "Nombre total de succes : "
        << nbCorrects
        << endl;

    if(nbTests > 0){

        double tauxGlobal =
            100.0 * nbCorrects / nbTests;

        cout << "Taux global de decision correcte : "
            << tauxGlobal
            << " %"
            << endl;
    }

    cout << endl;

    cout << "Visages connus testes : "
        << nbConnus
        << endl;

    cout << "Visages connus correctement identifies : "
        << nbConnusCorrects
        << endl;

    if(nbConnus > 0){

        double tauxIdentification =
            100.0 * nbConnusCorrects / nbConnus;

        cout << "Taux d'identification : "
            << tauxIdentification
            << " %"
            << endl;
    }

    cout << endl;

    cout << "Visages inconnus testes : "
        << nbInconnus
        << endl;

    cout << "Visages inconnus correctement rejetes : "
        << nbInconnusCorrects
        << endl;

    if(nbInconnus > 0){

        double tauxRejet =
            100.0 * nbInconnusCorrects / nbInconnus;

        cout << "Taux de rejet : "
            << tauxRejet
            << " %"
            << endl;
    }

    cout << "=========================" << endl;

    return 0;
    }