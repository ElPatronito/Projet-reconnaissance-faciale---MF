#ifndef PHOTOSQL_H
#define PHOTOSQL_H

#include <vector>
#include <memory>
#include <stdexcept>
#include <cppconn/prepared_statement.h>
#include "../models/Photo.h"
#include "../DatabaseManager.h"

class PhotoSQL {
public:
    explicit PhotoSQL(DatabaseManager& db) : db_(db) {}

// Insérer une nouvelle photo dans la BDD
    void save(const Photo& photo) {
        checkConnection();
        try {
            std::unique_ptr<sql::PreparedStatement> stmt(
                db_.getConnection()->prepareStatement(
                    "INSERT INTO Photos (chemin, description, individu_id) "
                    "VALUES (?, ?, ?)"
                )
            );
            stmt->setString(1, photo.getChemin());
            stmt->setString(2, photo.getDescription());
            stmt->setInt   (3, photo.getIndividuId());
            stmt->execute();
        }
        catch (const sql::SQLException& e) {
            throw std::runtime_error(
                std::string("PhotoSQL::save : ") + e.what()
            );
        }
    }

//  Compte le nombre de photo pour un individu afin d'indicer la suivante
    int countByIndividu(int individuId) {
        checkConnection();
        try {
            std::unique_ptr<sql::PreparedStatement> stmt(
                db_.getConnection()->prepareStatement(
                    "SELECT COUNT(*) FROM Photos WHERE individu_id = ?"
                )
            );
            stmt->setInt(1, individuId);

            std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
            if (res->next())
                return res->getInt(1);
            return 0;
        }
        catch (const sql::SQLException& e) {
            throw std::runtime_error(
                std::string("PhotoSQL::countByIndividu : ") + e.what()
            );
        }
    }

// Vérifie si une photo existe déjà par son chemin pour éviter les doublons
    bool exists(const std::string& chemin) {
        checkConnection();
        try {
            std::unique_ptr<sql::PreparedStatement> stmt(
                db_.getConnection()->prepareStatement(
                    "SELECT COUNT(*) FROM Photos WHERE chemin = ?"
                )
            );
            stmt->setString(1, chemin);

            std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
            if (res->next())
                return res->getInt(1) > 0;
            return false;
        }
        catch (const sql::SQLException& e) {
            throw std::runtime_error(
                std::string("PhotoSQL::exists : ") + e.what()
            );
        }
    }

// Récupère toutes les photos d'un individu
    std::vector<Photo> findByIndividu(int individuId) {
        checkConnection();
        try {
            std::unique_ptr<sql::PreparedStatement> stmt(
                db_.getConnection()->prepareStatement(
                    "SELECT idImage, chemin, description, individu_id "
                    "FROM Photos WHERE individu_id = ?"
                )
            );
            stmt->setInt(1, individuId);

            std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
            std::vector<Photo> photos;
            while (res->next()) {
                Photo p;
                p.setId         (res->getInt   ("idImage"));
                p.setChemin     (res->getString("chemin"));
                p.setDescription(res->getString("description"));
                p.setIndividuId (res->getInt   ("individu_id"));
                photos.push_back(p);
            }
            return photos;
        }
        catch (const sql::SQLException& e) {
            throw std::runtime_error(
                std::string("PhotoSQL::findByIndividu : ") + e.what()
            );
        }
    }

private:
    DatabaseManager& db_;  // référence, pas un pointeur nu

    void checkConnection() {
        if (!db_.isConnected()) {
            std::cerr << "Connexion perdue, tentative de reconnexion..." << std::endl;
            db_.reconnect();
        }
    }
};

#endif
