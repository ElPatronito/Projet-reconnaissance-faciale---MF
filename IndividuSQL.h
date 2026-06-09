#ifndef INDIVIDUSQL_H
#define INDIVIDUSQL_H

// ce fichier est un DAO (Data Access Object) permet de faire le lien entre BDD et code

#include <memory>
#include <stdexcept>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

#include "../models/Individu.h"
#include "../DatabaseManager.h"
 
class IndividuSQL {
public:
    explicit IndividuSQL(DatabaseManager& db) : db_(db) {}		//réutilise la connxion du DatabaseManager

// Recherche d'un individu dans la BDD par son nom et prénom et renvoi son ID, s'il n'y est pas il renvoi -1
    int findIdByNom(const std::string& nom, const std::string& prenom) {
        checkConnection();
        try {
            std::unique_ptr<sql::PreparedStatement> stmt(
                db_.getConnection()->prepareStatement(
                    "SELECT idPersonne FROM Individus "
                    "WHERE nom = ? AND prenom = ? LIMIT 1"
                )
            );
            stmt->setString(1, nom);
            stmt->setString(2, prenom);
 
            std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
            if (res->next())
                return res->getInt("idPersonne");
            return -1;
        }
        catch (const sql::SQLException& e) {
            throw std::runtime_error(
                std::string("IndividuSQL::findIdByNom : ") + e.what()
            );
        }
    }
 
// Ajouter un nouvel individu à la BDD 
    int save(const Individu& individu) {
		checkConnection();
		try {
			std::unique_ptr<sql::PreparedStatement> stmtInsert(
				db_.getConnection()->prepareStatement(
					"INSERT INTO Individus (nom, prenom) VALUES (?, ?)"
				)
			);
			stmtInsert->setString(1, individu.getNom());
			stmtInsert->setString(2, individu.getPrenom());
			stmtInsert->executeUpdate();
		}
		catch (const sql::SQLException& e) {
			throw std::runtime_error(
				std::string("IndividuSQL::save (insert) : ") + e.what()
			);
		}

		try {
			std::unique_ptr<sql::PreparedStatement> stmtSelect(
				db_.getConnection()->prepareStatement(
					"SELECT idPersonne FROM Individus "
					"WHERE nom = ? AND prenom = ? "
					"ORDER BY idPersonne DESC LIMIT 1"
				)
			);
			stmtSelect->setString(1, individu.getNom());
			stmtSelect->setString(2, individu.getPrenom());

			std::unique_ptr<sql::ResultSet> res(stmtSelect->executeQuery());
			if (res->next())
				return res->getInt("idPersonne");

			throw std::runtime_error("IndividuSQL::save : id introuvable après INSERT.");
		}
		catch (const sql::SQLException& e) {
			throw std::runtime_error(
				std::string("IndividuSQL::save (select) : ") + e.what()
			);
		}
	}
 
private:
    DatabaseManager& db_;
 
    void checkConnection() {		//vérifie avant chaque procédure que l'on est connecté
        if (!db_.isConnected()) {
            std::cerr << "Connexion perdue, tentative de reconnexion..." << std::endl;
            db_.reconnect();
        }
    }
};
 
#endif
 
