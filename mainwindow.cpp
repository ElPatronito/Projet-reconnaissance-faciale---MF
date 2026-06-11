#include "mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QFrame>
#include <QDesktopServices>
#include <QUrl>
#include <QFile>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProgressBar>
#include <QDir>
#include <QCoreApplication>
#include <QDir>

// Seuil fixe défini dans mainFinal.cpp
static const double  SEUIL         = 1968.83;
static const QString CHEMIN_IMAGES = "/tmp/projet_images/";


AddImageWindow::AddImageWindow(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Ajouter une image à la base");
    setFixedSize(420, 240);

    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *lblPrenom = new QLabel("Prénom :");
    inputPrenom = new QLineEdit();

    QLabel *lblNom = new QLabel("Nom :");
    inputNom = new QLineEdit();

    QLabel *lblImage = new QLabel("Image :");
    QHBoxLayout *rowImage = new QHBoxLayout();
    inputImagePath = new QLineEdit();
    inputImagePath->setReadOnly(true);
    btnBrowse = new QPushButton("Parcourir…");
    rowImage->addWidget(inputImagePath);
    rowImage->addWidget(btnBrowse);

    QPushButton *btnAdd    = new QPushButton("Ajouter");
    QPushButton *btnCancel = new QPushButton("Annuler");
    btnAdd->setStyleSheet(
        "background:#28a745; color:white; padding:6px 18px; border-radius:6px; border:none;");
    btnCancel->setStyleSheet(
        "background:#e0e0e0; color:#111; padding:6px 18px; border-radius:6px;");

    layout->addWidget(lblPrenom);
    layout->addWidget(inputPrenom);
    layout->addWidget(lblNom);
    layout->addWidget(inputNom);
    layout->addWidget(lblImage);
    layout->addLayout(rowImage);
    layout->addSpacing(10);

    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->addWidget(btnAdd);
    btnRow->addWidget(btnCancel);
    layout->addLayout(btnRow);

    connect(btnBrowse, &QPushButton::clicked, this, &AddImageWindow::browseImage);
    connect(btnAdd,    &QPushButton::clicked, this, &AddImageWindow::validateAdd);
    connect(btnCancel, &QPushButton::clicked, this, &AddImageWindow::reject);
}

void AddImageWindow::browseImage() {
    QString file = QFileDialog::getOpenFileName(
        this, "Choisir une image", "",
        "Images (*.png *.jpg *.jpeg)"
    );
    if (!file.isEmpty())
        inputImagePath->setText(file);
}

void AddImageWindow::validateAdd() {
    QString prenom = inputPrenom->text().trimmed();
    QString nom    = inputNom->text().trimmed();
    QString src    = inputImagePath->text().trimmed();

    if (prenom.isEmpty() || nom.isEmpty() || src.isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Veuillez remplir tous les champs.");
        return;
    }

    
    QString dossier = CHEMIN_IMAGES + "/" + prenom;
    QDir().mkpath(dossier);

   
    QDir dir(dossier);
    int numero = dir.entryList({"*.jpg","*.jpeg","*.png","*.pgm"}, QDir::Files).size() + 1;
    QString numero2   = QString("%1").arg(numero, 2, 10, QChar('0'));
    QString nomFichier  = "IMG_" + prenom + "_" + numero2 + ".png";
    QString destination = dossier + "/" + nomFichier;

    
    cv::Mat img = cv::imread(src.toStdString());
    if (img.empty()) {
        QMessageBox::warning(this, "Erreur", "Impossible de lire l'image source.");
        return;
    }
    if (!cv::imwrite(destination.toStdString(), img)) {
        QMessageBox::warning(this, "Erreur", "Impossible de sauvegarder l'image.");
        return;
    }


    try {
        DatabaseManager db;
        IndividuSQL individuRepo(db);
        PhotoSQL    photoRepo(db);

        int individuId = individuRepo.findIdByNom(nom.toStdString(), prenom.toStdString());
        if (individuId == -1) {
            Individu ind(nom.toStdString(), prenom.toStdString());
            individuId = individuRepo.save(ind);
        }
        if (!photoRepo.exists(destination.toStdString())) {
            Photo p(destination.toStdString(), "", individuId);
            photoRepo.save(p);
        }
        QMessageBox::information(this, "Succès",
            "Image ajoutée :\n" + destination + "\net enregistrée en base de données.");
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Attention",
            QString("Image copiée mais erreur BDD : ") + e.what());
    }

    accept();
}


LoginWindow::LoginWindow(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Connexion");
    setFixedSize(350, 250);

    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *labelUser = new QLabel("Identifiant :");
    inputUser = new QLineEdit();

    QLabel *labelPass = new QLabel("Mot de passe :");
    inputPass = new QLineEdit();
    inputPass->setEchoMode(QLineEdit::Password);

    QPushButton *btnLogin  = new QPushButton("Se connecter");
    QPushButton *btnCreate = new QPushButton("Créer un compte");

    layout->addWidget(labelUser);
    layout->addWidget(inputUser);
    layout->addWidget(labelPass);
    layout->addWidget(inputPass);
    layout->addSpacing(10);
    layout->addWidget(btnLogin);
    layout->addWidget(btnCreate);

    connect(btnLogin,  &QPushButton::clicked, this, &LoginWindow::tryLogin);
    connect(btnCreate, &QPushButton::clicked, this, &LoginWindow::createAccount);
}

void LoginWindow::tryLogin() {
    if (inputUser->text().isEmpty() || inputPass->text().isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Veuillez remplir tous les champs.");
        return;
    }
    QMessageBox::information(this, "Connexion", "Connexion réussie !");
    accept();
}

void LoginWindow::createAccount() {
    CreateAccountWindow create(this);
    create.exec();
}



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      pipeline_(nullptr),
      pipelineReady_(false)
{
    setWindowTitle("Reconnaissance Faciale");
    resize(900, 650);

    QWidget *central = new QWidget(this);
    central->setStyleSheet("background-color: white;");
    setCentralWidget(central);

    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(20, 16, 20, 20);
    
    QHBoxLayout *topBar = new QHBoxLayout();

    QLabel *logo = new QLabel;
    logo->setPixmap(QPixmap("/home/cytech/Projet/cytech_logo.png")
                    .scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    QLabel *title = new QLabel("Reconnaissance faciale");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size:28px; font-weight:bold; color:#111111;");

    QPushButton *btnHelp     = new QPushButton("Aide");
    QPushButton *btnLogin    = new QPushButton("Connexion");
    QPushButton *btnAddImage = new QPushButton("Ajouter une image");

    btnHelp->setStyleSheet(
        "background:#e0e0e0; color:#111111; border:1px solid #aaaaaa;"
        "border-radius:20px; padding:6px 14px; font-weight:bold;"
    );
    btnLogin->setStyleSheet(
        "background:#007bff; color:white; border-radius:20px; padding:6px 14px;"
    );
    btnAddImage->setStyleSheet(
        "background:#28a745; color:white; border-radius:20px;"
        "padding:6px 14px; font-weight:bold;"
    );

    QHBoxLayout *topRightButtons = new QHBoxLayout();
    topRightButtons->addWidget(btnHelp);
    topRightButtons->addWidget(btnLogin);

    QVBoxLayout *rightButtons = new QVBoxLayout();
    rightButtons->setSpacing(6);
    rightButtons->addLayout(topRightButtons);
    rightButtons->addWidget(btnAddImage, 0, Qt::AlignRight);

    QWidget *rightWidget = new QWidget();
    rightWidget->setStyleSheet("background:transparent;");
    rightWidget->setLayout(rightButtons);

    topBar->addWidget(logo);
    topBar->addWidget(title, 1);
    topBar->addWidget(rightWidget);
    mainLayout->addLayout(topBar);

   
    mainLayout->addSpacing(10);

    QHBoxLayout *imagesLayout = new QHBoxLayout();
    imagesLayout->setSpacing(20);


    QVBoxLayout *leftCol = new QVBoxLayout();
    QLabel *lblTest = new QLabel("Image à analyser");
    lblTest->setAlignment(Qt::AlignCenter);
    lblTest->setStyleSheet("font-size:13px; color:#555; font-weight:bold;");

    uploadBox = new QFrame();
    uploadBox->setFixedSize(380, 260);
    uploadBox->setStyleSheet(
        "border:2px dashed #cccccc; background:#f5f5f5; border-radius:8px;"
    );
    QVBoxLayout *uploadLayout = new QVBoxLayout(uploadBox);
    QPushButton *btnUpload = new QPushButton("Choisir une image");
    btnUpload->setStyleSheet(
        "background:#007bff; color:white; font-weight:bold;"
        "padding:8px 20px; border-radius:6px; border:none;"
    );
    uploadLayout->addWidget(btnUpload, 0, Qt::AlignCenter);

    imagePreview = new QLabel();
    imagePreview->setAlignment(Qt::AlignCenter);
    imagePreview->setFixedSize(380, 260);
    imagePreview->setVisible(false);

    leftCol->addWidget(lblTest);
    leftCol->addWidget(uploadBox);
    leftCol->addWidget(imagePreview);

    
    QVBoxLayout *rightCol = new QVBoxLayout();
    QLabel *lblRef = new QLabel("Photo de référence");
    lblRef->setAlignment(Qt::AlignCenter);
    lblRef->setStyleSheet("font-size:13px; color:#555; font-weight:bold;");

    refImageLabel = new QLabel("—");
    refImageLabel->setAlignment(Qt::AlignCenter);
    refImageLabel->setFixedSize(380, 260);
    refImageLabel->setStyleSheet(
        "border:2px solid #cccccc; background:#f5f5f5; border-radius:8px;"
    );

    rightCol->addWidget(lblRef);
    rightCol->addWidget(refImageLabel);

    imagesLayout->addLayout(leftCol);
    imagesLayout->addLayout(rightCol);
    mainLayout->addLayout(imagesLayout);

    
    progressBar = new QProgressBar();
    progressBar->setRange(0, 0);
    progressBar->setFixedHeight(6);
    progressBar->setTextVisible(false);
    progressBar->setStyleSheet(
        "QProgressBar { border:none; background:#e0e0e0; border-radius:3px; }"
        "QProgressBar::chunk { background:#007bff; border-radius:3px; }"
    );
    progressBar->setVisible(false);
    mainLayout->addWidget(progressBar);

    // ── Label de résultat ─────────────────────────────────────────
    resultLabel = new QLabel("");
    resultLabel->setAlignment(Qt::AlignCenter);
    resultLabel->setWordWrap(true);
    resultLabel->setStyleSheet(
        "font-size:15px; font-weight:bold; color:#333333; border:none;"
    );
    mainLayout->addWidget(resultLabel, 0, Qt::AlignCenter);

    // ── Boutons d'action ──────────────────────────────────────────
    QHBoxLayout *actionButtons = new QHBoxLayout();
    actionButtons->setAlignment(Qt::AlignCenter);

    btnResult = new QPushButton("Lancer la reconnaissance");
    btnResult->setEnabled(false);
    btnResult->setStyleSheet(
        "background:#6c757d; color:white; font-weight:bold;"
        "padding:10px 36px; border-radius:6px; border:none; font-size:14px;"
    );

    btnReset = new QPushButton("Nouvelle analyse");
    btnReset->setVisible(false);
    btnReset->setStyleSheet(
        "background:#ffc107; color:#111; font-weight:bold;"
        "padding:10px 30px; border-radius:6px; border:none; font-size:14px;"
    );

    actionButtons->addWidget(btnResult);
    actionButtons->addWidget(btnReset);
    mainLayout->addLayout(actionButtons);

    // ── Connexions ────────────────────────────────────────────────
    connect(btnUpload,   &QPushButton::clicked, this, &MainWindow::chooseImage);
    connect(btnResult,   &QPushButton::clicked, this, &MainWindow::runRecognition);
    connect(btnReset,    &QPushButton::clicked, this, &MainWindow::resetRecognition);
    connect(btnHelp,     &QPushButton::clicked, this, &MainWindow::openHelp);
    connect(btnLogin,    &QPushButton::clicked, this, &MainWindow::openLogin);
    connect(btnAddImage, &QPushButton::clicked, this, &MainWindow::openAddImage);

    initPipeline();
}

MainWindow::~MainWindow() {
    delete pipeline_;
}

void MainWindow::initPipeline() {
    progressBar->setVisible(true);

    QThread *initThread = new QThread(this);
    connect(initThread, &QThread::started, this, [this, initThread]() {
        try {
            pipeline_ = new RecognitionPipeline(
                CHEMIN_IMAGES.toStdString(), 128, SEUIL);
            pipeline_->initialiser();
            pipelineReady_ = true;
            QMetaObject::invokeMethod(this, [this]() {
                progressBar->setVisible(false);
                if (!selectedImagePath_.isEmpty())
                    btnResult->setEnabled(true);
            }, Qt::QueuedConnection);
        } catch (const std::exception& e) {
            QString msg = QString::fromStdString(e.what());
            QMetaObject::invokeMethod(this, [this, msg]() {
                progressBar->setVisible(false);
                resultLabel->setText("Erreur chargement modèle : " + msg);
                resultLabel->setStyleSheet(
                    "font-size:13px; color:#dc3545; font-weight:bold; border:none;"
                );
            }, Qt::QueuedConnection);
        }
        initThread->quit();
    });
    initThread->start();
}

void MainWindow::chooseImage() {
    QString file = QFileDialog::getOpenFileName(
        this, "Choisir une image", "",
        "Images (*.png *.jpg *.jpeg)"
    );
    if (file.isEmpty()) return;

    QPixmap pix(file);
    if (pix.isNull()) {
        QMessageBox::warning(this, "Erreur", "Impossible de charger l'image.");
        return;
    }

    selectedImagePath_ = file;
    imagePreview->setPixmap(pix.scaled(380, 260, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    imagePreview->setVisible(true);
    uploadBox->setVisible(false);

    refImageLabel->clear();
    refImageLabel->setText("—");
    resultLabel->setText("");
    btnReset->setVisible(false);

    if (pipelineReady_) {
        btnResult->setEnabled(true);
        btnResult->setStyleSheet(
            "background:#28a745; color:white; font-weight:bold;"
            "padding:10px 36px; border-radius:6px; border:none; font-size:14px;"
        );
    }
}

void MainWindow::runRecognition() {
    if (selectedImagePath_.isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Veuillez d'abord choisir une image.");
        return;
    }
    if (!pipelineReady_) {
        QMessageBox::warning(this, "Patientez", "Le modèle est encore en cours de chargement.");
        return;
    }

    btnResult->setEnabled(false);
    btnResult->setStyleSheet(
        "background:#6c757d; color:white; font-weight:bold;"
        "padding:10px 36px; border-radius:6px; border:none; font-size:14px;"
    );
    resultLabel->setText("Analyse en cours…");
    progressBar->setVisible(true);

    RecognitionWorker *worker =
        new RecognitionWorker(pipeline_, selectedImagePath_, this);

    connect(worker, &RecognitionWorker::resultReady,
            this,   &MainWindow::onResultReady);
    connect(worker, &RecognitionWorker::errorOccurred,
            this,   &MainWindow::onRecognitionError);
    connect(worker, &RecognitionWorker::finished,
            worker, &QObject::deleteLater);

    worker->start();
}

void MainWindow::resetRecognition() {
    selectedImagePath_ = "";
    imagePreview->setVisible(false);
    imagePreview->setPixmap(QPixmap());
    uploadBox->setVisible(true);
    refImageLabel->clear();
    refImageLabel->setText("—");
    resultLabel->setText("");
    btnReset->setVisible(false);
    btnResult->setEnabled(false);
    btnResult->setStyleSheet(
        "background:#6c757d; color:white; font-weight:bold;"
        "padding:10px 36px; border-radius:6px; border:none; font-size:14px;"
    );
}

void MainWindow::onResultReady(ResultatReconnaissance res) {
    progressBar->setVisible(false);

    QString texte;
    if (res.reconnu) {
        texte = QString(
            "✅  Personne reconnue : <b>%1</b><br>"
            "Distance : %2  |  Seuil : %3  |  Marge : %4<br>"
            "Confiance : <b>%5</b>"
        )
        .arg(QString::fromStdString(res.nom))
        .arg(res.distance,         0, 'f', 1)
        .arg(SEUIL,                0, 'f', 1)
        .arg(res.marge,            0, 'f', 1)
        .arg(QString::fromStdString(res.confiance));

        resultLabel->setStyleSheet(
            "font-size:15px; color:#28a745; font-weight:bold; border:none;"
        );

        if (!res.cheminPhotoRef.empty()) {
            QPixmap refPix(QString::fromStdString(res.cheminPhotoRef));
            if (!refPix.isNull())
                refImageLabel->setPixmap(
                    refPix.scaled(380, 260, Qt::KeepAspectRatio, Qt::SmoothTransformation)
                );
        }
    } else {
        texte = QString(
            "❌  Personne non reconnue<br>"
            "Distance : %1  |  Seuil : %2  |  Marge : %3<br>"
            "Confiance : <b>%4</b>"
        )
        .arg(res.distance,         0, 'f', 1)
        .arg(SEUIL,                0, 'f', 1)
        .arg(res.marge,            0, 'f', 1)
        .arg(QString::fromStdString(res.confiance));

        resultLabel->setStyleSheet(
            "font-size:15px; color:#dc3545; font-weight:bold; border:none;"
        );
        refImageLabel->setText("Non reconnu");
    }

    resultLabel->setText(texte);
    btnReset->setVisible(true);
    btnResult->setEnabled(true);
    btnResult->setStyleSheet(
        "background:#28a745; color:white; font-weight:bold;"
        "padding:10px 36px; border-radius:6px; border:none; font-size:14px;"
    );
}

void MainWindow::onRecognitionError(QString msg) {
    progressBar->setVisible(false);
    resultLabel->setText("Erreur : " + msg);
    resultLabel->setStyleSheet(
        "font-size:14px; color:#dc3545; font-weight:bold; border:none;"
    );
    btnResult->setEnabled(true);
    btnReset->setVisible(true);
}

void MainWindow::openHelp() {
    QString path = "/home/cytech/Projet/Livrable.pdf";
    if (!QFile::exists(path)) {
        QMessageBox::warning(this, "Erreur", "Fichier introuvable : " + path);
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void MainWindow::openLogin() {
    LoginWindow login(this);
    login.exec();
}

void MainWindow::openAddImage() {
    AddImageWindow dlg(this);
    dlg.exec();
}



CreateAccountWindow::CreateAccountWindow(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Créer un compte");
    setFixedSize(350, 250);

    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *labelUser = new QLabel("Nouvel identifiant :");
    newUser = new QLineEdit();

    QLabel *labelPass = new QLabel("Nouveau mot de passe :");
    newPass = new QLineEdit();
    newPass->setEchoMode(QLineEdit::Password);

    QPushButton *btnCreate = new QPushButton("Créer le compte");
    QPushButton *btnCancel = new QPushButton("Annuler");

    layout->addWidget(labelUser);
    layout->addWidget(newUser);
    layout->addWidget(labelPass);
    layout->addWidget(newPass);
    layout->addSpacing(10);
    layout->addWidget(btnCreate);
    layout->addWidget(btnCancel);

    connect(btnCreate, &QPushButton::clicked, this, &CreateAccountWindow::validateAccount);
    connect(btnCancel, &QPushButton::clicked, this, &CreateAccountWindow::reject);
}

void CreateAccountWindow::validateAccount() {
    if (newUser->text().isEmpty() || newPass->text().isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Veuillez remplir tous les champs.");
        return;
    }
    QMessageBox::information(this, "Compte créé", "Votre compte a été créé avec succès !");
    accept();
}