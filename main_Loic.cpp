#include <iostream>
#include <Eigen/Dense>
#include <limits>

#include "Preprocesseur.h"
#include "PCA.h"
#include "LDA.h"

using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXi;

int main(){

    vector<string> nomsClasses = {
    "Alexy", 
    "Aurianne",
    "Julien",
    "Loic",
    "Justine",
    "Mathis",
    "Paul"
};

    string cheminImages = "/home/cytech/ING1/Projet_Reconnaissance_Faciale/Images";

    Preprocesseur prep(128);

    prep.chargerDossier(cheminImages);

    vector<string> nomsImages = prep.getLabels();

    MatrixXd X = prep.getXEigen();
    VectorXi labels = prep.getLabelsNumeriquesEigen();

    cout << "Matrice X : "
         << X.rows()
         << " x "
         << X.cols()
         << endl;

    cout << "Nombre de labels : "
         << labels.size()
         << endl;

    PCA pca(X);

    pca.VisageMoyen();
    pca.Centrage();
    pca.MatriceDuale();
    pca.ValeursPropres();
    
    int nbClasses = labels.maxCoeff() + 1;
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

    cv::Mat imageTest = cv::imread("/home/cytech/C++/Projet/TestJulien.jpg");

    cv::Mat vecteurTestCV = prep.pretraiterImage(imageTest);

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

        cout << "Distance vers classe " << c << " : " << distance << endl;

        if(distance < distanceMin){
            distanceMin = distance;
            indiceMin = c;
        }
    }

    cout << "Personne reconnue : " << nomsClasses[indiceMin] << endl;
    cout << "Distance minimale : " << distanceMin << endl;
    cout << "Projection finale LDA Y : " << Y.rows() << " x " << Y.cols() << endl;

    cout << "Pipeline PCA + LDA termine avec succes." << endl;

    return 0;
}
