#ifndef FILE_TABLE
#define FILE_TABLE
#include <string>
#include "fs/utils/MyBitMap.h"
#include <map>
#include <vector>
#include <set>
#include <fstream>
class FileTable {
private:
    std::multiset<std::string> isExist;
    std::multiset<std::string> isOpen;
    std::vector<std::string> fname;
    std::vector<std::string> format;
    std::map<std::string, int> nameToID;
    std::string* idToName;
    MyBitMap* ft, *ff;
    int n;
    void load() {
        std::ifstream fin("filenames");
        fin >> n;
        for (int i = 0; i < n; ++ i) {
            std::string s, a;
            fin >> s;
            isExist.insert(s);
            fname.push_back(s);
            fin >> a;
            format.push_back(a);
        }
        fin.close();
    }
    void save() {
        std::ofstream fout("filenames");
        fout << fname.size() << std::endl;
        for (uint i = 0; i < fname.size(); ++ i) {
            fout << fname[i] << std::endl;
            fout << format[i] << std::endl;
        }
        fout.close();
    }
public:
    int newTypeID() {
        int k = ft->findLeftOne();
        ft->setBit(k, 0);
        return k;
    }
    int newFileID(const std::string& name) {
        int k = ff->findLeftOne();
        ff->setBit(k, 0);
        nameToID[name] = k;
        isOpen.insert(name);
        idToName[k] = name;
        return k;
    }
    bool ifexist(const std::string& name) {
        return (isExist.find(name) != isExist.end());
    }
    void addFile(const std::string& name, const std::string& fm) {
        isExist.insert(name);
        fname.push_back(name);
        format.push_back(fm);
    }
    int getFileID(const std::string& name) {
        if (isOpen.find(name) == isOpen.end()) {
            return -1;
        }
        return nameToID[name];
    }
    void freeTypeID(int typeID) {
        ft->setBit(typeID, 1);
    }
    void freeFileID(int fileID) {
        ff->setBit(fileID, 1);
        std::string name = idToName[fileID];
        isOpen.erase(name);
    }
    std::string getFormat(std::string name) {
        for (uint i = 0; i < fname.size(); ++ i) {
            if (name == fname[i]) {
                return format[i];
            }
        }
        return "-";
    }
    FileTable(int fn, int tn) {
        load();
        ft = new MyBitMap(tn, 1);
        ff = new MyBitMap(fn, 1);
        idToName = new std::string[fn];
        isOpen.clear();
    }
    ~FileTable() {
        save();
    }
};
#endif
