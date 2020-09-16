#include "NumpyAnalysisManager.hh"

#include "g4cnpy.h"

#include <iostream>

NumpyAnalysisManager* NumpyAnalysisManager::instance = 0;
std::mutex NumpyAnalysisManager::instanceMutex;

NumpyAnalysisManager::NumpyAnalysisManager(bool contWrite){
    fname = "out.npz";
    continousWrite = contWrite;
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



void NumpyAnalysisManager::WriteData(){
    instanceMutex.lock();
    std::string stat = "w";
    for(size_t i = 0;i < data.size();i++){
        if(((std::vector<std::tuple<>>*)data[i])->empty()) continue;
        writeFuncs[i](fname,dataTitles[i],data.at(i),stat);
        stat = "a";
    }
    if(stat == "w")
        std::cout << "No output produced for " << fname << "! This might be an error!\n";
    instanceMutex.unlock();
}
