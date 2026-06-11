#include <iostream>
#include <Eigen/Dense>
#include <limits>

#include "Preprocesseur.h"
#include "PCA.h"
#include "LDA.h"

using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXi;

int main(int argc, char* argv[]){

    /* ==========================================
     * Reconnaissance faciale par PCA + LDA
     * ==========================================
     *
     * Pipeline :
     *  - chargement et prétraitement des images
     *  - construction de la matrice X
     *  - réduction de dimension par ACP
     *  - amélioration de la séparation par LDA
     *  - classification par distance aux centres de classes
     */

    vector<string> nomsClasses;

    string cheminImages = "..∕Image";
    string dossierTests = "../ImagesTest";

    Preprocesseur prep(128);

    // Chargement de la base d'apprentissage.
    // Les images sont converties, recadrées, redimensionnées puis vectorisées.
    prep.chargerDossier(cheminImages);

    // Récupération automatique des noms de classes depuis les dossiers.
    nomsClasses = prep.getNomsClasses();

    vector<string> nomsImages = prep.getLabels();

    // X est la matrice des données :
    // chaque colonne correspond à une image vectorisée.
    MatrixXd X = prep.getXEigen();

    // labels contient la classe numérique associée à chaque image.
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

    /*
     * =========================
     *          ACP
     * =========================
     */

    PCA pca(X);

    // Calcul du visage moyen.
    pca.VisageMoyen();

    // Centrage des données autour du visage moyen.
    pca.Centrage();

    // Construction de la matrice duale G = Xc^T Xc.
    pca.MatriceDuale();

    // Calcul des valeurs propres et vecteurs propres.
    pca.ValeursPropres();
    
    // Contrainte utilisée avant la LDA :
    // k <= N - C
    // avec N le nombre d'images et C le nombre de classes.
    int kMax = X.cols() - nbClasses;

    // Choix automatique du nombre de composantes principales.
    pca.choixK(0.95, kMax);

    // Reconstruction des eigenfaces.
    pca.EigenFaces();

    // Projection des images dans l'espace ACP.
    pca.projectionACP();

    MatrixXd Z = pca.getProjectionACP();

    cout << "Projection ACP Z : "
         << Z.rows()
         << " x "
         << Z.cols()
         << endl;

    /*
     * =========================
     *          LDA
     * =========================
     */

    LDA lda(Z, labels);

    // Moyenne de chaque classe dans l'espace ACP.
    lda.ClasseMoyenne();

    // Moyenne globale des données projetées.
    lda.MoyenneGlobale();

    // Dispersion intra-classe : mesure la dispersion au sein d'une même personne.
    lda.MatriceDispersionIntra();

    // Dispersion inter-classe : mesure l'écart entre les différentes personnes.
    lda.MatriceDispersionInter();

    // Résolution du problème discriminant de Fisher.
    lda.solutionLDA();

    // Construction des fisherfaces.
    lda.Fisherfaces();

    // Projection finale dans l'espace LDA.
    lda.projectionLDA();

    MatrixXd Y = lda.getProjectionLDA();

    /*
     * =========================
     *   Centres des classes
     * =========================
     *
     * Chaque personne est représentée par le centre de ses images
     * dans l'espace final LDA.
     */

    vector<VectorXd> centresClasses(
        nbClasses,
        VectorXd::Zero(Y.rows())
    );

    vector<int> compteurs(nbClasses, 0);

    for(int i = 0; i < Y.cols(); i++){

        int c = labels(i);

        centresClasses[c] += Y.col(i);

        compteurs[c]++;
    }

    for(int c = 0; c < nbClasses; c++){

        centresClasses[c] /= compteurs[c];
    }
    
    /*
     * =========================
     *      Image à tester
     * =========================
     */

    
    if(argc < 2){

        cerr << "Erreur : aucun chemin d'image test fourni."
            << endl;

        cerr << "Utilisation : ./programme chemin_image_test"
            << endl;

        return 1;
    }

    string cheminImageTest = argv[1];

    cv::Mat imageTest =
        cv::imread(cheminImageTest);

    
    if(imageTest.empty()){

        cerr << "Erreur : impossible de charger l'image test."
             << endl;

        return 1;
    }

    // L'image test subit le même prétraitement que les images d'apprentissage.
    cv::Mat vecteurTestCV = prep.pretraiterImage(imageTest);

    if(vecteurTestCV.empty()){

        cerr << "Erreur : aucun visage detecte dans l'image test."
             << endl;

        return 1;
    }

    VectorXd xTest(vecteurTestCV.cols);

    for(int j = 0; j < vecteurTestCV.cols; j++){
        xTest(j) = vecteurTestCV.at<double>(0,j);
    }

    // Projection de l'image test dans l'espace ACP.
    VectorXd zTest = pca.projeterImageACP(xTest);

    // Projection de l'image test dans l'espace LDA.
    VectorXd yTest = lda.projeterImageLDA(zTest);

    /*
     * =========================
     *      Classification
     * =========================
     *
     * On compare l'image test aux centres de classes.
     * La classe retenue est celle dont la distance est minimale.
     */

    double distanceMin = numeric_limits<double>::max();
    double deuxiemeDistance = numeric_limits<double>::max();

    int indiceMin = -1;

    for(int c = 0; c < nbClasses; c++){

        double distance = (yTest - centresClasses[c]).norm();

        cout << "Distance vers classe "
             << c
             << " : "
             << distance
             << endl;

        if(distance < distanceMin){

            deuxiemeDistance = distanceMin;

            distanceMin = distance;

            indiceMin = c;
        }
        else if(distance < deuxiemeDistance){

            deuxiemeDistance = distance;
        }
    }

    /*
     * =========================
     *   Décision et confiance
     * =========================
     *
     * Le seuil permet d'éviter de forcer une reconnaissance
     * lorsqu'une personne n'est pas présente dans la base.
     *
     * La marge donne une indication de confiance :
     * plus l'écart entre les deux meilleures classes est grand,
     * plus la décision est stable.
     */

    double seuilReconnaissance = 1968.83;
    double marge = deuxiemeDistance - distanceMin;

    string confiance;

    if(marge > 1500){
        confiance = "forte";
    }
    else if(marge > 700){
        confiance = "moyenne";
    }
    else{
        confiance = "faible";
    }

    
    if(distanceMin > seuilReconnaissance){

        cout << "Personne non reconnue avec certitude." << endl;

        cout << "Classe candidate la plus proche : "
            << nomsClasses[indiceMin]
            << endl;
    }
    else{

        cout << "Personne reconnue : "
            << nomsClasses[indiceMin]
            << endl;
    }
    
    
    cout << "Distance minimale : "
         << distanceMin
         << endl;

    cout << "Deuxieme meilleure distance : "
         << deuxiemeDistance
         << endl;

    cout << "Marge : "
         << marge
         << endl;

    cout << "Confiance : "
         << confiance
         << endl;
    
    cout << "Projection finale LDA Y : "
         << Y.rows()
         << " x "
         << Y.cols()
         << endl;

    cout << "Pipeline PCA + LDA termine avec succes."
         << endl;

    return 0;
}