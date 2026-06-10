#ifndef PHOTO_H
#define PHOTO_H

#include <string>

using namespace std;

class Photo {
	private:
	int idImage;
	string chemin;
	string description;
	int individuId;
	
	public:
	Photo() : idImage(0), chemin(""), description(""), individuId(0) {}
	Photo(const string& chemin, const string& description, int individuId) : idImage(0), chemin(chemin), description(description), individuId(individuId) {}
	
	int getId() const { return idImage; }
	string getChemin() const { return chemin; }
	string getDescription() const { return description; }
	int getIndividuId() const { return individuId; }
	
	void setId(int i) {idImage = i; }
	void setChemin     (const string& c) { chemin      = c; }
    void setDescription(const string& d) { description = d; }
    void setIndividuId (int i) { individuId  = i; }
};

#endif
