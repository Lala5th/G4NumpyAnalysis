#ifndef NumpyAnalysisManager_hh
#define NumpyAnalysisManager_hh

#include <string>
#include <mutex>
#include <vector>

class NumpyAnalysisManager{
    public:
        NumpyAnalysisManager();
        ~NumpyAnalysisManager();
        void SetFilename(std::string);
        int CreateDataset(std::string,int);
        void AddData(int,double,double*);
        void WriteData();
        static NumpyAnalysisManager* GetInstance();
    private:
        static NumpyAnalysisManager* instance;
        static std::mutex instanceMutex;
        std::string fname;
        std::vector<std::string> dataTitles;
        std::vector<std::vector<double>*> data;
        std::vector<int> dataDim;
};

#endif
