#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H
 
#include <mysql_driver.h>			// bibliothèques permettant de parler à une base SQL depuis C++
#include <mysql_connection.h>		// idem
#include <cppconn/exception.h>		//idem
#include <memory>
#include <stdexcept>
#include <iostream>
 
// DatabaseManager sert de gestionnaire de connexion à la base de données MySQL

#ifndef DB_HOST
#define DB_HOST "tcp://127.0.0.1:3306"
#endif
#ifndef DB_USER
#define DB_USER "Projet"
#endif
#ifndef DB_PASS
#define DB_PASS "MFgroupe15@"
#endif
#ifndef DB_NAME
#define DB_NAME "Image"
#endif
 
class DatabaseManager {
public:
    DatabaseManager() {			//constructeur
        try {
            sql::mysql::MySQL_Driver* driver =
                sql::mysql::get_mysql_driver_instance();
 
            conn_.reset(driver->connect(DB_HOST, DB_USER, DB_PASS));
            conn_->setSchema(DB_NAME);
            std::cout << "Connexion à la BDD réussie." << std::endl;
        }
        catch (const sql::SQLException& e) {
            throw std::runtime_error(
                std::string("Échec de connexion MySQL : ") + e.what()
            );
        }
    }
 
    sql::Connection* getConnection() const {		//getteur pour pouvoir se connecter et exécuter des requêtes SQL
        if (!conn_) {
            throw std::runtime_error("Connexion BDD non initialisée.");
        }
        return conn_.get();
    }
 
    bool isConnected() const {			//vérifier que l'on est pas déjà connecté à la BDD
        return conn_ && conn_->isValid();
    }
 
    void reconnect() {			//essaye de reconnecter si la connexion est perdue
        try {
            if (conn_) conn_->reconnect();
            std::cout << "Reconnexion réussie." << std::endl;
        }
        catch (const sql::SQLException& e) {
            throw std::runtime_error(
                std::string("Échec de reconnexion : ") + e.what()
            );
        }
    }
 
private:
    std::unique_ptr<sql::Connection> conn_;  // la connexion est automatiquement libérée quand l'objet est détruit
};
 
#endif
 
