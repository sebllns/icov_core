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
  -DBoost_USE_STATIC_LIBS=OFF \
  -DICOV_USE_OMP=ON

# 5. Build
cmake --build build/release -j"$(nproc)"

# 6. Check
./build/release/bin/icov_cli --help
```

Debug build: same as steps 4-5 with `-B build/debug -DCMAKE_BUILD_TYPE=Debug`.

For clang, use `-DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++` in steps 3 and 4.

Notes:
- conda-forge ships only shared Boost libraries, hence `-DBoost_USE_STATIC_LIBS=OFF` (default `ON`).
- `-DICOV_USE_OMP=ON` parallelizes the encoder over block predictions; keep it on whenever the compiler supports OpenMP (default `OFF`). With conda clang, also install `llvm-openmp`.
- Other options: `-DICOV_BUILD_APPS=OFF` (skip the viewer and CLI), `-DICOV_ONLINE_MODE=OFF` (skip the git update of downloaded dependencies such as GLFW and glad, to rebuild without network; the first build still downloads them).

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

ICOV encodes an equirectangular (ERP) image or video once. A client then decodes only the blocks of its viewport, each one predicted from whichever neighboring block it has already decoded.

The commands below run from the build output directory, where `data/` holds the LDPC matrices:

```bash
cd build/release/bin
```

### 1. Create a test image

Any ERP image readable by OpenCV works (2:1 ratio). Its width and height should be multiples of the 32-pixel block size, otherwise the encoder resizes it. To generate a synthetic 1024x512 color gradient:

```bash
python ../../../scripts/gen_synthetic_erp.py erp.ppm
```

Options: `--width` (default 1024), `--height` (default width / 2), and `--stripes`, which adds sharp diagonal edges, a harder content (see step 2).

### 2. Encode

```bash
./icov_cli -i erp.ppm -o encoded.icov/ -d data/imt_164_v2/
```

The output directory is created if needed. On 14 threads, encoding the 1024x512 gradient takes about 80 s. For quick tests, `--ldpc-bp-ite 20` is about 3x faster for less than 0.5% more data.

Main encoder options:

| Option | Default | Meaning |
|---|---|---|
| `--encode-qp` | 27 | Quantization parameter (higher: lower quality, fewer bits) |
| `--encode-icu-size` | 32 | Block size in pixels |
| `-f`, `--encode-fov` | 80 | Horizontal field of view of the viewport, in degrees |
| `--ldpc-bp-ite` | 100 | Maximum belief-propagation iterations |
| `--encode-frame-count`, `--encode-start-frame` | 0, 0 | Frame range for videos (0: all frames) |

`encoded.icov/` contains the stored data (`header`, `prediction`, `syndrome`, `bsize`, `fsize`, `metric`). It also holds `info.txt`, a human-readable summary with the distribution of the encoded rates, which the decoder does not read.

Warnings like `No convergence for bitplane N` mean that a bitplane could not be LDPC-coded and was stored raw. Coding stays lossless, so they are expected on content with sharp edges (e.g. `--stripes`). See `doc/encoder_performance_report.md`.

### 3. Navigate in the viewer

```bash
./icov_viewer -i encoded.icov -d data/imt_164_v2/ --decode-ground-truth erp.ppm
```

`--decode-ground-truth` is optional. It enables the source and difference views. Keys (AZERTY layout):

| Key | Action |
|---|---|
| `Z` `Q` `S` `D` | Move the viewpoint up, left, down, right |
| `R` | Reset the viewpoint |
| `W` | Switch between the viewport and the full ERP view |
| `F1` | Decoded image (`X` toggles the source, needs the ground truth) |
| `F2` / `F3` | Rate per block / rate of the current viewport |
| `F4` / `F5` | Prediction direction / prediction category per block |
| `F6` | Difference with the ground truth |
| `C` | Toggle ICOV decoding |
| `P` / `F` | Play or pause / next frame (videos) |
| `Esc` | Quit |

A small "ICOV Monitor" window shows the frame, the viewpoint and the decoder state.

### 4. Decode a navigation path without display

The CLI decoder replays a list of viewpoints and exports metrics to CSV, with no graphical output:

```bash
printf '0,0,0\n0,10,45\n0,-20,-90\n' > positions.csv
mkdir -p results
./icov_cli -m decode -i encoded.icov -d data/imt_164_v2/ \
  --decode-positions-input positions.csv \
  --decode-ground-truth erp.ppm \
  --decode-csv-output results/run --decode-headless 1
```

Each line of `positions.csv` is `time_in_seconds,latitude,longitude` in degrees. The time is converted to a frame number with the content frame rate; use 0 for an image. The decoder writes:

| File | Content |
|---|---|
| `results/run_requests.csv` | One line per viewpoint request |
| `results/run_blocks.csv` | One line per decoded block: prediction used, encoded bitrate, all-intra bitrate, MSE, PSNR |
| `results/run_viewports.csv` | Per-viewport summary (only the header is written in the current version) |

The same `--decode-positions-input` option makes `icov_viewer` replay the path instead of using the keyboard.

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

