#include "NumpyAnalysisManager.hh"

int main(){
    NumpyAnalysisManager* man = NumpyAnalysisManager::GetInstance();
    int id = man->CreateDataset<int,char>("name");
    man->AddData<int,char>(id,5,'a');
    man->AddData(id,7,'b');
    man->WriteData();
}
