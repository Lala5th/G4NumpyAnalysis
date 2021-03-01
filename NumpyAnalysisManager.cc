#include "NumpyAnalysisManager.hh"

#include "g4cnpy.h"

#include "zip.h"

#include <iostream>

NumpyAnalysisManager* NumpyAnalysisManager::instance = 0;
std::mutex NumpyAnalysisManager::instanceMutex = std::mutex();

NumpyAnalysisManager::NumpyAnalysisManager(bool contWrite){
    fname = "out.npz";
    continousWrite = contWrite;
}

NumpyAnalysisManager::~NumpyAnalysisManager(){}

NumpyAnalysisManager* NumpyAnalysisManager::GetInstance(bool contWrite){
    instanceMutex.lock();
    if(!instance){
        instance = new NumpyAnalysisManager(contWrite);
    } // ONLY first run is important for the purposes of this!
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
    if(!continousWrite){
    std::string stat = "w";
    for(size_t i = 0;i < data.size();i++){
        if(((std::vector<std::tuple<>>*)data[i])->empty()) continue;
        writeFuncs[i](fname,dataTitles[i],data.at(i),stat);
        stat = "a";
    }
    if(stat == "w"){
        std::cout << "No output produced for " << fname << "! This might be an error!\n";
    }

    }else{
        zip_t* outfile = zip_open(fname.c_str(),ZIP_TRUNCATE|ZIP_CREATE,NULL);
        if(!outfile) throw std::runtime_error("Could not open zipfile");
        zip_error_t ziperror;
        for(auto name : dataTitles){
            zip_source_t* dataset = zip_source_file_create((name+".npy").c_str(),0,0,&ziperror);
            zip_file_add(outfile, (name+".npy").c_str(),dataset,ZIP_FL_OVERWRITE);
        }
        zip_close(outfile);
    }
    instanceMutex.unlock();
}
