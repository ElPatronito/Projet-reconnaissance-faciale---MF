#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFrame>
#include <QThread>
#include <QProgressBar>

#include "RecognitionPipeline.h"
#include "DatabaseManager.h"
#include "repositories/IndividuSQL.h"
#include "repositories/PhotoSQL.h"
#include "models/Individu.h"
#include "models/Photo.h"

// ──────────────────────────────────────────────
//  Worker : reconnaissance dans un thread séparé
// ──────────────────────────────────────────────
class RecognitionWorker : public QThread {
    Q_OBJECT
public:
    explicit RecognitionWorker(RecognitionPipeline* pipeline,
                               const QString& cheminImage,
                               QObject* parent = nullptr)
        : QThread(parent), pipeline_(pipeline), cheminImage_(cheminImage) {}

protected:
    void run() override {
        try {
            ResultatReconnaissance res =
                pipeline_->reconnaitre(cheminImage_.toStdString());
            emit resultReady(res);
        } catch (const std::exception& e) {
            emit errorOccurred(QString::fromStdString(e.what()));
        }
    }

signals:
    void resultReady(ResultatReconnaissance res);
    void errorOccurred(QString msg);

private:
    RecognitionPipeline* pipeline_;
    QString cheminImage_;
};

// ──────────────────────────────────────────────
//  AddImageWindow : ajout d'une image à la base
// ──────────────────────────────────────────────
class AddImageWindow : public QDialog {
    Q_OBJECT
public:
    explicit AddImageWindow(QWidget *parent = nullptr);

private slots:
    void browseImage();
    void validateAdd();

private:
    QLineEdit *inputPrenom;
    QLineEdit *inputNom;
    QLineEdit *inputImagePath;
    QPushButton *btnBrowse;
};

// ──────────────────────────────────────────────
//  CreateAccountWindow
// ──────────────────────────────────────────────
class CreateAccountWindow : public QDialog {
    Q_OBJECT
public:
    explicit CreateAccountWindow(QWidget *parent = nullptr);

private slots:
    void validateAccount();

private:
    QLineEdit *newUser;
    QLineEdit *newPass;
};

// ──────────────────────────────────────────────
//  LoginWindow
// ──────────────────────────────────────────────
class LoginWindow : public QDialog {
    Q_OBJECT
public:
    explicit LoginWindow(QWidget *parent = nullptr);

private slots:
    void tryLogin();
    void createAccount();

private:
    QLineEdit *inputUser;
    QLineEdit *inputPass;
};

// ──────────────────────────────────────────────
//  MainWindow
// ──────────────────────────────────────────────
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void chooseImage();
    void runRecognition();
    void resetRecognition();
    void onResultReady(ResultatReconnaissance res);
    void onRecognitionError(QString msg);
    void openHelp();
    void openLogin();
    void openAddImage();

private:
    // UI
    QFrame        *uploadBox;
    QLabel        *imagePreview;
    QLabel        *resultLabel;
    QLabel        *refImageLabel;   // photo de référence
    QPushButton   *btnResult;
    QPushButton   *btnReset;
    QProgressBar  *progressBar;

    // Pipeline
    RecognitionPipeline *pipeline_;
    QString              selectedImagePath_;
    bool                 pipelineReady_;

    void initPipeline();
};

#endif // MAINWINDOW_H