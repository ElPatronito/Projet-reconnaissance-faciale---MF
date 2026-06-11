#ifndef PCA_H
#define PCA_H

#include <iostream>
#include <vector>
#include <cmath>
#include <Eigen/Dense>

using namespace std;

using Eigen::MatrixXd;
using Eigen::VectorXd;
using Eigen::SelfAdjointEigenSolver;

class PCA {

    private :
    
    MatrixXd X;
    MatrixXd Xc;

    VectorXd meanFace;

    MatrixXd G;

    VectorXd eigenvalues;
    MatrixXd eigenvectors;
    
    MatrixXd eigenfaces;

    MatrixXd Z;

    int k;


    public :

    PCA(const MatrixXd& data);

    void VisageMoyen();

    void Centrage();

    void MatriceDuale();

    void ValeursPropres();

    void choixK(double seuil = 0.95, int kMax = -1);

    void EigenFaces();

    void projectionACP();

    MatrixXd getProjectionACP() const;

    MatrixXd getEigenfaces() const;

    VectorXd getMeanFace() const;

    VectorXd projeterImageACP(const VectorXd& xTest) const;

};

#endif