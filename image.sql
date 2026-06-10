USE Image;
SET FOREIGN_KEY_CHECKS = 0;
DROP TABLE Photos;
DROP TABLE Individus;
SET FOREIGN_KEY_CHECKS = 1;

CREATE TABLE Individus (
    idPersonne INT NOT NULL AUTO_INCREMENT,
    nom VARCHAR(100) NOT NULL,
    prenom VARCHAR(100) NOT NULL,
    PRIMARY KEY (idPersonne)
) ENGINE=InnoDB;

CREATE TABLE Photos (
    idImage INT NOT NULL AUTO_INCREMENT,
    chemin VARCHAR(500) NOT NULL,
    description VARCHAR(255),
    individu_id INT NOT NULL,
    PRIMARY KEY (idImage),
    FOREIGN KEY (individu_id) REFERENCES Individus(idPersonne)
) ENGINE=InnoDB;
