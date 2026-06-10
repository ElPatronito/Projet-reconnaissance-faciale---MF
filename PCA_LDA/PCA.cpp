#include "PCA.h"

PCA::PCA(const MatrixXd& data){

    X = data;

    k = 0;
}

void PCA::VisageMoyen(){

    meanFace = X.rowwise().mean();
}

void PCA::Centrage(){

    Xc = X.colwise() - meanFace;
}

void PCA::MatriceDuale(){

    G = Xc.transpose() * Xc;
}

void PCA::ValeursPropres(){

    SelfAdjointEigenSolver<MatrixXd> solver(G);

    if(solver.info() != Eigen::Success){

        cerr << "Erreur decomposition ACP."
             << endl;

        return;
    }

    eigenvalues = solver.eigenvalues();

    eigenvectors = solver.eigenvectors();
}

void PCA::choixK(double seuil, int kMax){

    double sommeTotale = eigenvalues.sum();
    double sommePartielle = 0.0;

    k = 0;

    for(int i = eigenvalues.size() - 1; i >= 0 ; i--){

        if(kMax > 0 && k >= kMax){
            break;
        }

        sommePartielle += eigenvalues(i);
        k++;

        double R = sommePartielle / sommeTotale;

        if(R >= seuil){
            break;
        }
    }

    cout << "Nombre de composantes principales retenues : "
         << k << endl;
}

void PCA::EigenFaces(){

    int compteur = 0;

    eigenfaces.resize(Xc.rows(), k);

    for(int i = eigenvalues.size() - 1;
        i >= eigenvalues.size() - k;
        i--){

        double lambda = eigenvalues(i);

        if(lambda > 1e-10){

            VectorXd v =
                eigenvectors.col(i);

            VectorXd u =
                (1.0 / sqrt(lambda))
                * Xc * v;

            u.normalize();

            eigenfaces.col(compteur) = u;

            compteur++;
        }
    }
}

void PCA::projectionACP(){

    if(eigenfaces.cols() == 0){

        cerr << "Erreur : aucune eigenface disponible."
             << endl;

        return;
    }

    Z = eigenfaces.transpose() * Xc;
}

MatrixXd PCA::getProjectionACP() const{

    return Z;
}

MatrixXd PCA::getEigenfaces() const{

    return eigenfaces;
}

VectorXd PCA::getMeanFace() const{

    return meanFace;
}

VectorXd PCA::projeterImageACP(const VectorXd& xTest) const{

    VectorXd xCentre = xTest - meanFace;

    VectorXd zTest = eigenfaces.transpose() * xCentre;

    return zTest;
}
