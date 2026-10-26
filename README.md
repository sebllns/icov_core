# **_INTERACTIVE CODER FOR OMNIDIRECTIONAL VIDEOS_**

_Core part  of the INRIA ADT project ICOV (Interactive Coder for Omnidirectional Videos)_

# ICOV-core 

A codec framework to encode and decode your equirectangular 360° images/videos in optimal way. All the source code in this project are meant to be shared publicly.


For more details on the principle, please refer to https://ieeexplore.ieee.org/document/9171593
and https://project.inria.fr/intercom/ .


## Compile from source

### :arrow_right_hook: System requirements
- Unix based system with X11 or Wayland if you need to execute the Viewer
(Note: Windows is not fully supported for the current version, but you can try to compile the project without any garantee it will compile or run... good luck :four_leaf_clover: )
- C++ compiler
- CMake > 3.19
- AVX2 support is required
- Threads librairies
- OpenGL if you build the viewer
- OpenMP support will significantly speed up computing
- Git command should be installed - to manage third-party dependencies - if you want to compile the project with the CMake helpers

---

### :arrow_right_hook: Third-party libraries

:wave: **You can manage third-party libraries on your own, but if you follow the steps below to build ICOV with the superbuild tool, it should handle everything for you - depending on your operating system, you may need to still install some system packages, so carefully read the build errors as they will probably  indicate which package is missing.**

- For the core part:
    - Custom VTM (included)
    - Aff3ct (included)
    - [OpenCV 4.6.0](https://opencv.org/releases/)
    - [Boost 1.81.0](https://www.boost.org/users/history/version_1_81_0.html)
- For the Viewer app:
    - freeGLUT
    - glad
    - glm
    - Glfw


---

### :arrow_right_hook: Build with a conda environment (recommended)

The toolchain and dependencies come from conda-forge, listed in `environment.txt`. Only aff3ct is built from the submodule.

```bash
# 1. Sources, including the aff3ct submodule
git clone --recurse-submodules https://github.com/sebllns/icov_core.git
cd icov_core
# or, in an existing clone:
git submodule update --init --recursive

# 2. Conda environment
conda create -n icov-core -c conda-forge --override-channels --file environment.txt
conda activate icov-core

# 3. aff3ct, installed in a local prefix
cmake -S third-party/aff3ct -B build/aff3ct -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$PWD/build/aff3ct-install" \
  -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ \
  -DAFF3CT_COMPILE_EXE=OFF -DAFF3CT_COMPILE_STATIC_LIB=ON \
  -DAFF3CT_POLAR_BIT_PACKING=OFF -DAFF3CT_COLORS=OFF -DAFF3CT_PREC=32
cmake --build build/aff3ct --target install

# 4. Configure ICOV
cmake -S . -B build/release -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$CONDA_PREFIX;$PWD/build/aff3ct-install" \
  -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ \
  -DBoost_USE_STATIC_LIBS=OFF

# 5. Build
cmake --build build/release -j"$(nproc)"

# 6. Check
./build/release/bin/icov_cli --help
```

Debug build: same as steps 4-5 with `-B build/debug -DCMAKE_BUILD_TYPE=Debug`.

For clang, use `-DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++` in steps 3 and 4.

Notes:
- conda-forge ships only shared Boost libraries, hence `-DBoost_USE_STATIC_LIBS=OFF` (default `ON`).
- Main options: `-DICOV_USE_OMP=ON` (OpenMP), `-DICOV_BUILD_APPS=OFF` (skip the viewer and CLI), `-DICOV_ONLINE_MODE=OFF` (offline build).

---

### :arrow_right_hook: Download and build ICOV with the superbuild CMake project

:warning: **Building the project from source and installing all dependencies will require a significant storage ~9GB**

1 - Download source from Gitlab

:rotating_light: **If you have any space in your path, the configuration or build of the project will probably fail**

```bash
git clone --recurse-submodules -j8 https://gitlab.inria.fr/icov/src/core.git
```

2 - Compile
Alternative to the conda build: the superbuild downloads and builds all dependencies

```bash
mkdir build && cd build
cmake ../sp -DCMAKE_BUILD_TYPE=Release && cmake --build . --target all -j8
```

3 - Once ICOV is built successfully, move to the bin folder

```bash
cd icov/bin
```

## Run

Equirectangular videos or images can be encoded/decoded with ICOV software.

### First test: encode/decoded an image

Encoding:
```bash
mkdir encoded.icov
./icov_cli -i /path/to/my/equirect.jpg -o encoded.icov/ -d data/imt_164_v2/
```

Decoding:
```bash
./icov_viewer -i encoded.icov -d data/imt_164_v2/
```

Use Q,S,D,Z to move the viewpoint.

## Links

Doxygen: https://icov.gitlabpages.inria.fr/src/core/index.html

Resources: https://sourceforge.net/projects/icov/files

Demo video: https://vimeo.com/user188622130


<!-- Button to download + download counter -->
<!-- [![Download ICOV](https://img.shields.io/sourceforge/dw/icov.svg)](https://sourceforge.net/projects/icov/files/latest/download)  -->

<!-- Big green button to downoad last version on sourceforge -->
<!-- [![Download ICOV](https://a.fsdn.com/con/app/sf-download-button)](https://sourceforge.net/projects/icov/files/latest/download) -->


## Copyright notice

**Copyright (C) 2021-2026 by Inria. All rights reserved.**

The copyright in this software is being made available under the terms of the GNU Affero General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version. 

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.

**For using ICOV under terms that will not comply with the GNU AGPL, please contact Inria about acquiring an ICOV Professional Edition License.**
 
See contact section below for more information.

This software was developed at:
 > Inria Rennes - Bretagne Atlantique
 > Campus Universitaire de Beaulieu -
 > 35042 Rennes Cedex
 > France

If you have questions regarding the use of this software, please contact Inria using the contact section at the end of the file.

 **This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.**

## [Contact](mailto:ICOV%20team%3C%69%63%6F%76%2D%67%69%74%6C%61%62%40%69%6E%72%69%61%2E%66%72%3E)

- Developer: BELLENOUS, Sébastien
- Project manager: MAUGEY, Thomas
- Team leader: ROUMY, Aline 

