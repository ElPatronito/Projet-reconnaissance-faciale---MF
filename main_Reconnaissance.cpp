#include <iostream>
#include "RecognitionPipeline.h"
using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage : " << argv[0] << " <chemin_image_test> [seuil]" << endl;
        return 1;
    }

    string cheminImageTest = argv[1];
    double seuil = (argc >= 3) ? stod(argv[2]) : 1500.0;
    string cheminImages = "/home/cytech/ING1/Projet_Reconnaissance_Faciale/Image";

    RecognitionPipeline pipeline(cheminImages, 128, seuil);

    try {
        pipeline.initialiser();

        ResultatReconnaissance res = pipeline.reconnaitre(cheminImageTest);

        cout << "\n--- Resultat ---" << endl;
        if (res.reconnu)
            cout << "Personne reconnue : " << res.nom << endl;
        else
            cout << "Personne non reconnue (Inconnu)" << endl;
        cout << "Distance minimale : " << res.distance << endl;
        cout << "Seuil utilise     : " << pipeline.getSeuil() << endl;

    } catch (const exception& e) {
        cerr << "Erreur : " << e.what() << endl;
        return 1;
    }

    return 0;
}
