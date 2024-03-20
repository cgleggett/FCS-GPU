# FastCaloSim GPU Project

## Table of contents
* [Introduction](#Introduction)
* [Build Instructions for Different Backends](#Build-Instructions-for-Different-Backends)
* [Build Instructions for Cori](#Build-Instructions-for-Cori)
* [Validation](#Validation)
* [Formatting](#Formatting)

## Introduction

This repository contains the source code for the standalone version of
the ATLAS experiment's Liquid Argon Calorimeter parametrized
simulation. The original code was written in C++ for CPUs as part of
the ATLAS simulation software. A minimal form was extracted from the
ATLAS repository to enable rapid development in a standalone
environment without the multitude of ATLAS dependencies. This code was
then rewritten to run on GPUs using CUDA as reported
[here](https://arxiv.org/abs/2103.14737)

In order to study various portability layers to enable execution on
different GPU architectures, the code has also been ported to Kokkos,
HIP, SYCL, alpaka and std::par. Build instructions for these various
technologies are listed below.

FastCaloSim has the following dependencies:
* cmake (3.18 or higher)
* C++ compiler that is compatible with C++17. Recent versions of gcc, icpx and hipcc are recommended
* [ROOT](https://root.cern.ch). A recent version is recommended, with newer versions of the C++ compiler necessitating newer versions of ROOT

Build instructions for `cori.nersc.gov` are shown below as an
example. These should be easily replicable on any modern system with
the appropriate backends and hardware installed.

The CUDA, Kokkos, alpaka and std::par versions can all be built from
the same branch of the repository (`main`). The SYCL version is in the
`sycl` branch, and the HIP version is in `hip`. The following build
instructions assume that the repository has been cloned into the directory
named `src`.

Two different versions of the code have been developed. One simulates particle
interactions one at a time, the other groups a number of particles together
before offloading the work to the GPU to increase the GPU's workload. The latter
is referred to as the "group simulation".

## Build Instructions for Different Backends

First setup `cmake` and `ROOT`. If `ROOT` is installed in `$ROOT_PATH`, ensure
that your `$LD_LIBRARY_PATH` includes
`$ROOT_PATH/lib` and `$CMAKE_PREFIX_PATH` includes `$ROOT_PATH`.

### Input data

The simulation requires input data and parametrization files. The base directory of
these files can be specified either via the runtime parameter `--dataDir=DIR` or by
setting the environment variable `FCS_DATAPATH`.

### Original CPU code

```
cmake ../src/FastCaloSimAnalyzer \
-DENABLE_XROOTD=Off -DCMAKE_CXX_STANDARD=17 -DCMAKE_CXX_EXTENSIONS=Off \
-DENABLE_GPU=off
```

### CUDA

Checkout from branch `main`. For group simulation use branch `group_sim_combined`.
Build the project for an NVIDIA A100:

```
cmake ../src/FastCaloSimAnalyzer \
-DENABLE_XROOTD=Off -DCMAKE_CXX_STANDARD=17 -DCMAKE_CXX_EXTENSIONS=Off \
-DENABLE_GPU=on -DCMAKE_CUDA_ARCHITECTURES=80
```

### Kokkos

Kokkos must be built with `-DBUILD_SHARED_LIBS=On`.

For the CUDA backend, also use
`-DKokkos_ENABLE_CUDA_RELOCATABLE_DEVICE_CODE=Off -DKokkos_ENABLE_CUDA_LAMBDA=On`.

Checkout from branch `main`. For group simulation use branch `group_sim_combined`.
Build the project with
```
cmake ../src/FastCaloSimAnalyzer \
-DENABLE_XROOTD=Off -DCMAKE_CXX_STANDARD=17 -DCMAKE_CXX_EXTENSIONS=Off \
-DCMAKE_CXX_COMPILER=nvcc_wrapper \
-DENABLE_GPU=on -DUSE_KOKKOS=ON
```

#### Other Kokkos backends

Other hardware backend architectures are also supported. FastCaloSim has been
tested with the following Kokkos architectures:
* CUDA
* HIP
* SYCL
* pThreads
* OpenMP
* Serial

you will need to adjust the value of the cmake option `-DCMAKE_CXX_COMPILER=`
or the environment variable `$CXX` accordingly.

### SYCL

In order to run with SYCL, the `sycl` branch of repository should be
used. If the GPU does not support double precision types, such as the
Intel A770 GPU, use the `sycl_A770` branch. It is recommended that
ROOT be built with the same compiler that is used to build
FastCaloSim, be it icpx or clang.

Checkout from branch `sycl`. Build the project with
```
cmake ../src/FastCaloSimAnalyzer \
-DENABLE_XROOTD=Off -DCMAKE_CXX_STANDARD=17 -DCMAKE_CXX_EXTENSIONS=Off \
-DENABLE_SYCL=ON -DSYCL_TARGET_GPU=ON
```

SYCL has been tested using the icpx (oneAPI) compiler on Intel GPUs, and
llvm on NVIDIA and AMD GPUs.

### std::par

Checkout from branch `main`.  For group simulation use branch `group_sim_combined`.
Build the project with
```
cmake ../src/FastCaloSimAnalyzer \
-DENABLE_XROOTD=Off -DCMAKE_CXX_STANDARD=17 -DCMAKE_CXX_EXTENSIONS=Off \
-DCMAKE_CXX_COMPILER=$PWD/../src/scripts/nvc++_p \
-DENABLE_GPU=on -DUSE_STDPAR=ON -DSTDPAR_TARGET=gpu
```

Use cmake flag `-DUSE_STDPAR=On`.

In order to compile for std::par, `nvc++` from the nvidia nvhpc package must be
chosen for the CXX compiler. However ROOT still cannot build with nvc++, so part
of FastCaloSim must be built with g++. Also, nvc++ is not well supported in cmake,
and a number of compiler flags must be removed from the command line for it to work.
A wrapper script is provided in [scripts/nvc++p](scripts/nvc++_p) which chooses the correct compiler
for the various parts of FastCaloSim, and filters out the problematic compiler flags
for nvc++. Either set the `CXX` environment variable to point to this, or explicitly
set it during cmake configuration with `-DCMAKE_CXX_COMPILER=$PWD/../src/scripts/nvc++_p`.
You may need to edit the script to pickup the correct localrc configuration file for
nvc++. These can be generated with `makelocalrc` from the nvhpc package.

To see exactly what the wrapper script is doing, set the env var `NVCPP_VERBOSE=1`.

There are 3 backends for std::par: gpu, multicore, and serial cpu. These are normally
triggered by the nvc++ flags `-stdpar=gpu`, `-stdpar=multicore` and `-nostdpar`. Select
the desired backend with the cmake flags `-DSTDPAR_TARGET=XXX` where `XXX` is one of
`gpu`, `mutlicore` or `cpu`. If `cpu` is selected, the random numbers must be generated
on the cpu with `-DRNDGEN_CPU=On`


When profiling using `nsys`, make sure to pick it up from the nvhpc package, and not
directly from cuda.

When using the multicore backend, it is currently recommended to set the environment
variables `NV_NOSWITCHERROR=1`.


### HIP

Checkout from branch `hip`. For group simulation use branch `group_sim_hip`.
Build the project with

```
export CXX=`hipcc`
cmake ../src/FastCaloSimAnalyzer \
-DENABLE_XROOTD=Off -DCMAKE_CXX_STANDARD=17 -DCMAKE_CXX_EXTENSIONS=Off \
-DENABLE_GPU=on
```

### alpaka

### OpenMP

## Build Instructions for alpha/lambda @ CSI, BNL
```
module use /work/software/modulefiles
module load llvm-openmp-dev
source /work/atif/packages/root-6.24-gcc-9.3.0/bin/thisroot.sh
export FCS_DATAPATH=/work/atif/FastCaloSimInputs/
export OMP_TARGET_OFFLOAD=mandatory
cmake ../FastCaloSimAnalyzer -DENABLE_XROOTD=off -DENABLE_GPU=on -DENABLE_OMPGPU=on \ 
 -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_CXX_STANDARD=14 \
 -DCUDA_CUDART_LIBRARY=/usr/local/cuda/lib64/libcudart.so \
 -DCUDA_TOOLKIT_ROOT_DIR=/usr/local/cuda/ -DCMAKE_CUDA_COMPILER=/usr/local/cuda/bin/nvcc
```

## Build Instructions for Cori

### Building ROOT
ROOT v 6.14.08 (from tag) has been built with gcc8.3 and c++14 and installed in
`/global/cfs/cdirs/atlas/leggett/root/v6-14-08_gcc83_c14`

While FastCaloSim can be built with c++17 support, nvcc (for CUDA) can
only handle c++14


```
ROOT was built with
module load gcc/8.3.0
module load cmake/3.14.4

export CC=`which gcc`
export CXX=`which g++`

cmake -Dcxx=14 -Dcxx14=ON ../src
make -j30 VERBOSE=1 >& make.log
```

an example script is [here](scripts/cori/install_root_cxx14.sh)

### Building FastCaloSim

(code checked out in directory "src")

```
module load cuda
module load gcc/8.3.0
module load cmake/3.14.4

export ROOTDIR=/global/cfs/cdirs/atlas/leggett/root/v6-14-08_gcc83
export CMAKE_PREFIX_PATH=$ROOTDIR
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$ROOTDIR/lib
export PATH=$PATH:$ROOTDIR/bin

export CC=`which gcc`
export CXX=`which g++`

mkdir build
cd build
cmake ../src/FastCaloSimAnalyzer -DENABLE_XROOTD=off -DENABLE_GPU=on -DCMAKE_CXX_STANDARD=14
make -j 30 VERBOSE=1 >& make.log
```

to disable GPU, set option `-DENABLE_GPU=off`

### To run on GPU nodes

```
source $BUILD_DIR/x86_64-linux15-gcc8-opt/setup.sh

module load esslurm
salloc -N 1 -t 30 -c 80 --gres=gpu:8 --exclusive -C gpu -A m1759
srun -N1 -n1 runTFCSSimulation --dataDir=/global/cfs/cdirs/atlas/leggett/data/FastCaloSimInputs
```

## Kokkos

If Kokkos with a CUDA backend is already installed, ensure that the
environment variables `KOKKOS_ROOT` points to the install area, and that
`nvcc_wrapper` is in your `PATH`. Otherwise:

### Install Kokkos With CUDA backend
Checkout Kokkos from `git@github.com:kokkos/kokkos.git` into `$KOKKOS_SRC`

Set the env var `KOKKOS_INSTALL_DIR` to an appropriate value. CPU and
GPU architectures must be chosen. See [here](https://github.com/kokkos/kokkos/wiki/Compiling). For example, on a Haswell CPU and V100 GPU:

Build with
```
cmake ../src \
-DCMAKE_INSTALL_PREFIX=${KOKKOS_INSTALL_DIR} \
-DKokkos_ARCH_HSW=${CPU_ARCH} \
-DBUILD_SHARED_LIBS=On -DKokkos_ENABLE_CUDA_RELOCATABLE_DEVICE_CODE=Off \
-DKokkos_ENABLE_CUDA=On -DKokkos_ARCH_VOLTA72=On -DKokkos_ENABLE_CUDA_LAMBDA=On \
-DKokkos_ENABLE_SERIAL=On \
-DKokkos_CXX_STANDARD=14 \
-DCMAKE_CXX_COMPILER=${KOKKOS_SRC}/bin/nvcc_wrapper

make
make install
```

An example script to build and install Kokkos is [here](scripts/cori/install_kokkos.sh)

### Build FastCaloSim with CUDA enabled Kokkos

To build with Kokkos instead of plain nvcc, make sure you have the Kokkos
environment loaded, and that `$CXX` points to `nvcc_wrapper` from Kokkos.

Then add `-DUSE_KOKKOS=on` to the FastCaloSim cmake configuration


## Validation
### Random Numbers

Random numbers are by default generated on the GPU if `-DENABLE_GPU=On` is set
during configuration. Alternatively, to help comparison between CPU and GPU codes,
the random numbers can be generated on the CPU and transferred to the GPU. This is
enabled by setting the cmake configuration parameter `-DRNDGEN_CPU=On`.


### Checking Output

The number of hit cells and counts can be displayed by setting the environment
variable `FCS_DUMP_HITCOUNT=1` at run time. This will result in an output like:
```
 HitCellCount:  12 /  12   nhit:   55   
 HitCellCount:  48 /  60   nhit: 1220  *
 HitCellCount:  76 / 136   nhit: 4944  *
 HitCellCount:   6 / 142   nhit:   10   
 HitCellCount:   1 / 143   nhit:    1
```
Lines marked with an asterisk were executed on the GPU.


## Formatting
The FastCaloSimAnalyzer code has been formatted with:

```
find FastCaloSimAnalyzer -type f -iname \*.h -o -iname \*.cpp -o -iname \*.cxx -o -iname \*.cu | xargs clang-format-9 -i -style=file
```
