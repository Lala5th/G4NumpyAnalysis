# G4NumpyAnalysis

This is a wrapper class for cnpy[https://github.com/rogersce/cnpy] developed for use in Geant4 simulations. While it was developed for the framework of Geant4 it may be useful for other threaded applications, as such there is no dependence on Geant libraries.

# Installation

To build you will need cmake. To install do

```bash
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/path/to/install/dir ..
make
make install
```

# Usage

After install it should work with cmake's find_package() command as well as with using
```
-L/path/to/install/dir -lG4NumpyAnalysis
```
with g++.

To use it include the NumpyAnalysisManager.hh header file.

# Description

It implements a proprietary analysis manager, which is quite different from the analysis managers provided by Geant4. It is accessed similarly via invoking `NumpyAnalysisManager::GetInstance()`, which returns the active instance or creates an instance if it's not yet initialised. The other methods are `SetFilename(std::string)`, which changes the name of the output (default is out.npz). `CreateDataset(std::string title,int dim)`, which creates a new datatable with dim columns, while returning the id of the new datatable. `AddData(int id, double* row)` adds a row to the datatable with the matching id. Finalyl `WriteData()` writes the npz object to disk.

While it is technically thread-safe, it is advisable that worker, threads do not access it, otherwise (if badly handled), the datatables might store the same data multiple times. In Geant4 the IsMaster() function can be quite helpful so worker threads do not accidentally smear the data.
