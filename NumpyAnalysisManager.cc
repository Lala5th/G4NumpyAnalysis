#include "NumpyAnalysisManager.hh"

#include "cnpy.h"

#include <iostream>

NumpyAnalysisManager* NumpyAnalysisManager::instance = 0;
std::mutex NumpyAnalysisManager::instanceMutex;

NumpyAnalysisManager::NumpyAnalysisManager(){
    fname = "out.npz";
}

NumpyAnalysisManager::~NumpyAnalysisManager(){}

NumpyAnalysisManager* NumpyAnalysisManager::GetInstance(){
    instanceMutex.lock();
    if(!instance)
        instance = new NumpyAnalysisManager();
    instanceMutex.unlock();
    return instance;
}

void NumpyAnalysisManager::SetFilename(std::string name){
    instanceMutex.lock();
    fname = name;
    instanceMutex.unlock();
}

int NumpyAnalysisManager::CreateDataset(std::string name, int dim){
    instanceMutex.lock();
    std::vector<double>* dataset = new std::vector<double>;
    data.push_back(dataset);
    dataTitles.push_back(name);
    dataDim.push_back(dim);
    int id = data.end() - data.begin() - 1;
    instanceMutex.unlock();
    return id;
}

void NumpyAnalysisManager::AddData(int id, double d, double* coords){
    instanceMutex.lock();
    for(int i = 0; i < dataDim[id]; i++){
        data[id]->push_back(coords[i]);
    }
    data[id]->push_back(d);
    instanceMutex.unlock();
}


void NumpyAnalysisManager::WriteData(){
    instanceMutex.lock();
    std::string stat = "w";
    for(size_t i = 0;i < data.size();i++){
        if(data[i]->empty()) continue;
        cnpy::npz_save(fname,dataTitles[i],data[i]->data(),{data[i]->size()/(dataDim[i]+1),(dataDim[i]+1)},stat);
        stat = "a";
    }
    if(stat == "w")
        std::cout << "No output produced for " << fname << "! This might be an error!\n";
    instanceMutex.unlock();
}
