# SLabSimu
Geant4 based simulation for MuonSLab.

# Environment 
If you are on inpac cluster:
```bash
source ~mocen/hailing.env
```

# Quick Start
After setting the environment
```bash
git clone git@github.com:MuonSLab/SLabSimu.git
mkdir build && cd build
cmake ../SLabSimu
make -j9
./SLabSimu
```