set -e

# clear
rm -Rf MMVII-PhgrProj

# or auto RTL from average + create SysCo/RTL.xml
MMVII ImportGCP coord.cor ANXYZSxSySz InitRTL ChSys=[L93,RTL] AddInfoFree=0 Comment=*

# import obs
MMVII ImportOBS obs.obs Obs1

# adjust
MMVII TopoAdj Obs1 Obs1_out [[InitRTL,1.,FinalRTL]] UC_UK="[.*,.*,.*,1]"

# export to L93
MMVII GCPChSysCo L93 FinalRTL FinalL93

# Compare with Comp3d output
MMVII ImportGCP coord.new NXYZSxSySz OutCompL93 ChSys=[L93] Comment=*

meld MMVII-PhgrProj/ObjCoordWorld/FinalL93/MesGCP-NewGCP.xml MMVII-PhgrProj/ObjCoordWorld/OutCompL93/MesGCP-coord.xml

