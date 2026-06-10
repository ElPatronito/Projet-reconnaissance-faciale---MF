#include "Preprocesseur.h"
#include <iostream>

//constructeur
Preprocesseur::Preprocesseur(int taille) : taille(taille) {}



cv::Mat Preprocesseur::pretraiterImage(const cv::Mat& img) {

    cv::Mat gris, egalise, redim, flottant, vecteur;

    if (img.channels() == 3) { //si l'image est en BGR
        cv::cvtColor(img, gris, cv::COLOR_BGR2GRAY); //convertit en gris
    } else {
        gris = img.clone();// sinon copier l'image
    }

    cv::equalizeHist(gris, egalise); // améliore le contraste 

    cv::resize(egalise, redim, cv::Size(taille, taille)); // redimensionne l'image

    redim.convertTo(flottant, CV_64F); // convertit les pixels en doubles 64 bits

    vecteur = flottant.reshape(1, 1); // transforme la matrice en vecteur de 128*128 lignes

    return vecteur; //retourne l'image transformée en vecteur 
}




void Preprocesseur::chargerDossier(const std::string& cheminDossier) {

    X.release(); // libère la mémoire de la matrice et la remet à 0
    labels.clear(); // vide les listes avec les noms 
    labelsNumeriques.clear(); // vide les numéros
    correspondanceLabels.clear(); //vide la liste des correspondance

    std::vector<std::string> extensionsValides = {".jpg", ".jpeg", ".png", ".pgm"};
    //liste les extensions valides

    std::cout << "Chargement des images depuis : " << cheminDossier << std::endl;
    //affiche où en est le code pour vérifier si c'est le bon chemin 
    
    
    for (const auto& entree : fs::recursive_directory_iterator(cheminDossier)) {
    //parcourt tous les fichiers et sous-dossiers du chemin donné
        
        std::string ext = entree.path().extension().string();
        //convertit le nom du fichier en string manipulable
        //extrait l'extension

        bool valide = false;
        for (const auto& e : extensionsValides) {
            if (ext == e) { valide = true; break; }
        }
        if (!valide) continue;
        // parcourt les extensions valides
       
       
        cv::Mat img = cv::imread(entree.path().string()); //lit l'image et la met en mémoire
        if (img.empty()) { // si la lecture échoue
            std::cerr << "Impossible de lire : " << entree.path() << std::endl; 
            // img est vide et affiche une erreur sur cerr
            continue;
        }

        cv::Mat vecteur = pretraiterImage(img);
        X.push_back(vecteur);
        // Prétraite l'image et la met dans la matrice X (colonne -> transposée ensuite)


        std::string nomFichier = entree.path().stem().string();
        labels.push_back(nomFichier);
        // récupère le nom du chemin parent et créé une classe à ce nom

        std::string nomClasse = entree.path().parent_path().filename().string();
        // récupère le nom du dossier parent comme nom de classe 

        if(correspondanceLabels.find(nomClasse) == correspondanceLabels.end()){//cherche la clé 
            int nouvelIndice = correspondanceLabels.size();
            correspondanceLabels[nomClasse] = nouvelIndice;
        }
        //Si la classe n'a pas été attribuée : attribution d'un numéro dans l'ordre croissnat

        int labelNumerique = correspondanceLabels[nomClasse];
        labelsNumeriques.push_back(labelNumerique);
        //récupère l'indice numérique de la classe et l'ajoute à la listecorrespondanceLabels

        std::cout << entree.path().filename().string()
                  << " -> classe " << labelNumerique
                  << std::endl;
    }

    std::cout << X.rows << " images chargées." << std::endl; 
    //affiche le nombre d'images chargés = le nombre de lignes de la matrice
}


Eigen::MatrixXd Preprocesseur::getXEigen() const{ 

    Eigen::MatrixXd Xeigen(X.cols, X.rows); //matrice eigen 

    for(int i = 0; i < X.rows; i++){
        for(int j = 0; j < X.cols; j++){
            Xeigen(j,i) = X.at<double>(i,j);
        }
    }

    return Xeigen; // retourne la transposé de X 
}

Eigen::VectorXi Preprocesseur::getLabelsNumeriquesEigen() const{

    Eigen::VectorXi y(labelsNumeriques.size()); 
    //crée un vecteur y de la même taille que labelsNumeriques

    for(int i = 0; i < labelsNumeriques.size(); i++){
        y(i) = labelsNumeriques[i];
    }

    return y;
}
