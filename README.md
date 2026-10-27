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
- Git, to fetch the aff3ct submodule

---

### :arrow_right_hook: Third-party libraries

All external dependencies come from conda-forge (`environment.txt`, see the conda build below). aff3ct (git submodule) and the VTM subset are built from this repository.

- For the core part:
    - Custom VTM (included)
    - Aff3ct (included)
    - [OpenCV 4.10](https://opencv.org/releases/)
    - [Boost 1.84](https://www.boost.org/users/history/version_1_84_0.html)
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

Tests: configure with `-DICOV_BUILD_TESTS=ON` (default in Debug), build, then run:

```bash
ctest --test-dir build/release --output-on-failure      # all tests
ctest --test-dir build/release -L unit                  # LDPCA coder unit tests, about 1 s
ctest --test-dir build/release -L integration           # encode/decode round trip, about 20 s
```

The integration test encodes two small synthetic images twice (the outputs must be identical), then decodes a few viewpoints and checks the PSNR of every block. GitHub CI (`.github/workflows/ci.yml`) runs both on every push and pull request.

Documentation: doxygen and graphviz live in a separate environment, because graphviz conflicts with the OpenCV dependencies of `icov-core`:

```bash
conda create -n icov-doc -c conda-forge --override-channels --file environment-doc.txt
conda run -n icov-doc doxygen doc/Doxyfile    # output in doc/html/
```

The Docs workflow (`.github/workflows/docs.yml`) builds it the same way and publishes it to GitHub Pages on every push to `main`.

## Run

ICOV encodes an equirectangular (ERP) image or video once. A client then decodes only the blocks of its viewport, each one predicted from whichever neighboring block it has already decoded.

The commands below run from the build output directory:

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
./icov_cli -i erp.ppm -o encoded.icov
```

`-o` names the output file (default: the input name with `.icov`). On 14 threads, encoding the 1024x512 gradient takes about 80 s. For quick tests, `--ldpc-bp-ite 20` is about 3x faster for less than 0.5% more data.

Main encoder options:

| Option | Default | Meaning |
|---|---|---|
| `--encode-qp` | 27 | Quantization parameter (higher: lower quality, fewer bits) |
| `--encode-icu-size` | 32 | Block size in pixels |
| `-f`, `--encode-fov` | 80 | Horizontal field of view of the viewport, in degrees |
| `--ldpc-bp-ite` | 100 | Maximum belief-propagation iterations |
| `--encode-frame-count`, `--encode-start-frame` | 0, 0 | Frame range for videos (0: all frames) |
| `--export-reference` | off | Also write the original and reconstructed frames (see below) |
| `-d` | built-in | LDPC matrix set directory, see "LDPC matrices" below |
| `--external-matrices` | off | Do not store the matrices of a `-d` set in the file |

The encoder writes:

| File | Content |
|---|---|
| `encoded.icov` | The stream, a single ISOBMFF file: parameter set, block index, and one storage unit per block. Everything a server needs, nothing else |
| `encoded.stats.json` | Encoder information and statistics: version, input name, rate histograms, MSE per block. Optional for the decoder (PSNR columns of the CSV export) |
| `encoded.ref/` | Only with `--export-reference`: `original_<f>.tif` and `reconstructed_<f>.tif`, used by the viewer difference view (F6) and the source view |

The bitstream syntax is described in `doc/bitstream_report.md`.

Warnings like `No convergence for bitplane N` mean that a bitplane could not be LDPC-coded and was stored raw. Coding stays lossless, so they are expected on content with sharp edges (e.g. `--stripes`). See `doc/encoder_performance_report.md`.

### 3. Navigate in the viewer

```bash
./icov_viewer -i encoded.icov
```

Add `--decode-ground-truth erp.ppm` to display the viewport PSNR against the original. The source view (`X`) and the difference view (`F6`) need the reference frames: encode with `--export-reference`. Keys (AZERTY layout):

| Key | Action |
|---|---|
| `Z` `Q` `S` `D` | Move the viewpoint up, left, down, right |
| `R` | Reset the viewpoint |
| `W` | Switch between the viewport and the full ERP view |
| `F1` | Decoded image (`X` toggles the source, needs `--export-reference`) |
| `F2` / `F3` | Rate per block / rate of the current viewport |
| `F4` / `F5` | Prediction direction / prediction category per block |
| `F6` | Difference with the encoder reconstruction (needs `--export-reference`) |
| `C` | Toggle ICOV decoding |
| `P` / `F` | Play or pause / next frame (videos) |
| `Esc` | Quit |

A small "ICOV Monitor" window shows the frame, the viewpoint, the decoder state, the key bindings (F1) and a color legend for the current view.

### 4. Decode a navigation path without display

The CLI decoder replays a list of viewpoints and exports metrics to CSV, with no graphical output:

```bash
printf '0,0,0\n0,10,45\n0,-20,-90\n' > positions.csv
mkdir -p results
./icov_cli -m decode -i encoded.icov \
  --decode-positions-input positions.csv \
  --decode-csv-output results/run --decode-headless 1 --verify-hash
```

Each line of `positions.csv` is `time_in_seconds,latitude,longitude` in degrees. The time is converted to a frame number with the content frame rate; use 0 for an image. The decoder writes:

| File | Content |
|---|---|
| `results/run_requests.csv` | One line per viewpoint request |
| `results/run_blocks.csv` | One line per decoded block: prediction used, encoded bitrate, all-intra bitrate, MSE, PSNR, transmitted bytes |
| `results/run_viewports.csv` | Per-viewport summary (only the header is written in the current version) |

`--verify-hash` compares every decoded block with the encoder reconstruction (CRC-32 stored in the file) and exits with an error on any difference.

The same `--decode-positions-input` option makes `icov_viewer` replay the path instead of using the keyboard; it exits once the path is replayed.

### LDPC matrices

The LDPC matrix set `imt_164_v2` is compiled into the programs, so no data directory is needed. The file records which set was used (name and CRC-32), and the decoder refuses a set that does not match.

To experiment with another set, pass its directory with `-d` (the sets in `data/` are copied next to the binaries). Unless the set has the content of the built-in one, the encoder stores its matrices in the file (about 18 KB), so the file decodes anywhere without `-d`. With `--external-matrices` they are not stored, and the decoder needs the same `-d`. The decoder looks for the matrices in the file first, then in the built-in set, then in `-d`.

The stored matrices are not part of the block units: a streaming server would send them once per session, only to clients that do not already know the set (not implemented yet).

## Links

Doxygen: https://sebllns.github.io/icov_core/

Demo video: https://vimeo.com/user188622130



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

