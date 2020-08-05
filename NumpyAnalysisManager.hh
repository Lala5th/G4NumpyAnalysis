#ifndef NumpyAnalysisManager_hh
#define NumpyAnalysisManager_hh

#include <string>
#include <mutex>
#include <vector>
#include <tuple>
#include <functional>

#include "cnpy.h"


class NumpyAnalysisManager{
    public:
        NumpyAnalysisManager();
        ~NumpyAnalysisManager();
        void SetFilename(std::string);
        template<typename... COLS>
        int CreateDataset(std::string name){
            instanceMutex.lock();
            std::vector<std::tuple<COLS...>>* dataset = new std::vector<std::tuple<COLS...>>;
            data.push_back(dataset);
            dataTitles.push_back(name);
            dataDim.push_back(sizeof...(COLS));
            int id = data.end() - data.begin() - 1;
            auto write = [](std::string zipname, std::string fname, const void* data, std::string mode = "w"){
                cnpy::npz_save<COLS...>(zipname,fname,*((std::vector<std::tuple<COLS...>>*)data),mode);
            };
            writeFuncs.push_back(write);
            instanceMutex.unlock();
            return id;
        }
        template<typename... COLS>
        void AddData(int id, COLS... input){
            instanceMutex.lock();
            ((std::vector<std::tuple<COLS...>>*)data.at(id))->push_back(std::make_tuple(input...));
            instanceMutex.unlock();
        }
        void WriteData();
        static NumpyAnalysisManager* GetInstance();
    private:
        static NumpyAnalysisManager* instance;
        static std::mutex instanceMutex;
        std::string fname;
        std::vector<std::string> dataTitles;
        std::vector<void*> data;
        std::vector<size_t> dataDim;
        std::vector<std::function<void(std::string, std::string, const void*, std::string)>> writeFuncs;
};

#endif
