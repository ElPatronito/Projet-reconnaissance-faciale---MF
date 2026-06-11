#ifndef LDA_H
#define LDA_H

#include <iostream>
#include <vector>
#include <Eigen/Dense>
#include <algorithm>

using namespace std;

using Eigen::MatrixXd;
using Eigen::VectorXd;
using Eigen::VectorXi;
using Eigen::EigenSolver;

class LDA {

    private :

    MatrixXd Z;

    vector<VectorXd> classMeans;

    VectorXd globalMean;

    MatrixXd Sw;
    MatrixXd Sb;

    VectorXd ldaEigenvalues;
    MatrixXd ldaEigenvectors;

    MatrixXd fisherfaces;

    MatrixXd Y;

    VectorXi labels;

    int nbClasses;

    public :

    LDA(const MatrixXd& data,
        const VectorXi& Ylabels);

    void ClasseMoyenne();

    void MoyenneGlobale();

    void MatriceDispersionIntra();

    void MatriceDispersionInter();

    void solutionLDA();

    void Fisherfaces();

    void projectionLDA();

    MatrixXd getProjectionLDA() const;

    MatrixXd getFisherfaces() const;

    VectorXd projeterImageLDA(const VectorXd& zTest) const;
};

#endif