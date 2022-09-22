source /opt/intel/oneapi/setvars.sh
cmake -G "Ninja" -DCMAKE_BUILD_TYPE="$1" -DCMAKE_CXX_COMPILER="icpc" -DCMAKE_C_COMPILER="icc" -DCMAKE_Fortran_COMPILER="ifort" -S "." -B "./out/build/x64-intel-$1"
cmake --build "./out/build/x64-intel-$1" --target opencaeporo