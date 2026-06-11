#include "LDA.h"
#include <algorithm>

LDA::LDA(const MatrixXd& data,
         const VectorXi& Ylabels){

    Z = data;

    labels = Ylabels;

    nbClasses =
        labels.maxCoeff() + 1;
}

void LDA::ClasseMoyenne(){

    classMeans.resize(nbClasses);

    for(int c = 0; c < nbClasses; c++){

        VectorXd mean =
            VectorXd::Zero(Z.rows());

        int compteur = 0;

        for(int i = 0; i < Z.cols(); i++){

            if(labels(i) == c){

                mean += Z.col(i);

                compteur++;
            }
        }

        if(compteur > 0){

            mean /= compteur;
        }

        classMeans[c] = mean;
    }
}

void LDA::MoyenneGlobale(){

    globalMean = Z.rowwise().mean();
}

void LDA::MatriceDispersionIntra(){

    Sw = MatrixXd::Zero(
        Z.rows(),
        Z.rows()
    );

    for(int c = 0; c < nbClasses; c++){

        VectorXd mu_c =
            classMeans[c];

        for(int i = 0; i < Z.cols(); i++){

            if(labels(i) == c){

                VectorXd diff =
                    Z.col(i) - mu_c;

                Sw +=
                    diff * diff.transpose();
            }
        }
    }
}

void LDA::MatriceDispersionInter(){

    Sb = MatrixXd::Zero(
        Z.rows(),
        Z.rows()
    );

    for(int c = 0; c < nbClasses; c++){

        VectorXd mu_c =
            classMeans[c];

        VectorXd diff =
            mu_c - globalMean;

        int Nc = 0;

        for(int i = 0; i < labels.size(); i++){

            if(labels(i) == c){

                Nc++;
            }
        }

        Sb +=
            Nc * diff * diff.transpose();
    }
}

void LDA::solutionLDA(){

    MatrixXd M =
        Sw.inverse() * Sb;

    EigenSolver<MatrixXd> solver(M);

    if(solver.info() != Eigen::Success){

        cerr << "Erreur decomposition LDA."
             << endl;

        return;
    }

    ldaEigenvalues =
        solver.eigenvalues().real();

    ldaEigenvectors =
        solver.eigenvectors().real();
}

void LDA::Fisherfaces(){

    int r = nbClasses - 1;

    fisherfaces.resize(ldaEigenvectors.rows(), r);

    vector<int> indices(ldaEigenvalues.size());

    for(int i = 0; i < ldaEigenvalues.size(); i++){
        indices[i] = i;
    }

    sort(indices.begin(), indices.end(),
        [this](int a, int b){
            return ldaEigenvalues(a) > ldaEigenvalues(b);
        }
    );

    for(int i = 0; i < r; i++){
        fisherfaces.col(i) = ldaEigenvectors.col(indices[i]);
    }
}

void LDA::projectionLDA(){

    Y =
        fisherfaces.transpose() * Z;
}

MatrixXd LDA::getProjectionLDA() const{

    return Y;
}

MatrixXd LDA::getFisherfaces() const{

    return fisherfaces;
}

VectorXd LDA::projeterImageLDA(const VectorXd& zTest) const{

    VectorXd yTest = fisherfaces.transpose() * zTest;

    return yTest;
}