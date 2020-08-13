#include "NumpyAnalysisManager.hh"

int main(){
    NumpyAnalysisManager* man = NumpyAnalysisManager::GetInstance();
    int id = man->CreateDataset<int,double,float>("arr1");
    int id2 = man->CreateDataset<int,float>("arr2");
    man->AddData<int,double,float>(id,5,35.34,1.3);
    for(size_t i = 0; i < 1000; i++)
        man->AddData<int,double,float>(id,i,15.31,1.6);
    man->AddData<int,float>(id2,4,3.5);
    for(size_t i = 0; i < 1000; i++)
    man->AddData<int,float>(id2,i,4.7);
    man->WriteData();
}
