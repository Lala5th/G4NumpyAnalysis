#include "NumpyAnalysisManager.hh"

int main(){
    NumpyAnalysisManager* man = NumpyAnalysisManager::GetInstance();
    int id = man->CreateDataset<int,char>("arr1");
    int id2 = man->CreateDataset<int,float>("arr2");
    man->AddData<int,char>(id,5,'a');
    for(size_t i = 0; i < 1000; i++)
        man->AddData<int,char>(id,i,'b');
    man->AddData<int,float>(id2,4,3.5);
    for(size_t i = 0; i < 1000; i++)
    man->AddData<int,float>(id2,i,4.7);
    man->WriteData();
}
