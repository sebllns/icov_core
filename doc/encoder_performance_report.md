# Encoder performance and "No convergence" warnings

Date: 2026-10-26
Build: `build/release` (CMake `Release`, GCC 12.4, conda env `icov-core`, `ICOV_USE_OMP=OFF`, `ICOV_ASSERTION_ENABLED=ON`)
Machine: Intel Core Ultra 7 165U, 14 logical CPUs, Linux 6.14
Input: synthetic ERP images from `scripts/gen_synthetic_erp.py` (at the time of the measurements the stripes were the default; they now need `--stripes`)

## 1. Summary

- **Why it is slow.** About 95% of the encoder's CPU time is spent in the LDPC belief-propagation (BP) decoder that the encoder runs to pick a rate. Every decode attempt always runs the full number of BP iterations (default 100), because early stopping is disabled. For each bitplane, the encoder also tries the rates one by one from the lowest (up to 34 attempts), for every prediction (up to 9) and every color component (3) of every block.
- **Why there are so many warnings.** "No convergence for bitplane N" means that BP failed even at the highest rate (m = n, rate 1). The encoder then stores the bitplane raw. This is not data loss: the stored data is lossless and the cost (n bits) equals the highest LDPC rate anyway. The failures occur only on the low-order bitplanes (6 to 9) of blocks with sharp edges. The synthetic image's diagonal stripes create such edges, while a smooth gradient gives zero failures. The warnings are not caused by too few BP iterations: the failure count is the same with 20 and 100 iterations.
- **Quick win (measured).** `--ldpc-bp-ite 20` makes encoding 3.4x to 3.9x faster, with output size within 0.1% and the same failures.

## 2. Setup

```bash
E=$HOME/miniforge3/envs/icov-core
export LD_LIBRARY_PATH=$E/lib
cd core/build/release/bin
S=/path/to/workdir

# Test images
python scripts/gen_synthetic_erp.py $S/s256.ppm --width 256 --stripes  # striped, 256x128
python scripts/gen_synthetic_erp.py $S/t64.ppm  --width 64 --stripes   # striped, 64x32
python scripts/gen_synthetic_erp.py $S/g256.ppm --width 256            # smooth gradient, 256x128
```

The ICU size is 32, so a 256x128 image has 32 blocks.

## 3. Measures

### 3.1 First attempts (default options)

```bash
./icov_cli -i $S/erp.ppm -o $S/encoded.icov/ -d data/imt_164_v2/
```

| Image | Result |
|---|---|
| 1024x512 striped (512 blocks) | killed by a 600 s `timeout`, not finished |
| 512x256 striped (128 blocks) | stopped by hand after about 7 min, 568 warnings at that point (bitplane 6: 28, 7: 246, 8: 275, 9: 19) |

### 3.2 BP iteration count vs content

The four runs ran in parallel. Each process is single-threaded (OpenMP is off), and the machine has 14 CPUs.

```bash
for img in s256 g256; do for it in 100 20; do
  d=$S/x_${img}_$it; mkdir -p $d/out
  /usr/bin/time -f "%e s %M KB" ./icov_cli -i $S/$img.ppm -o $d/out/ \
      -d data/imt_164_v2/ --ldpc-bp-ite $it > $d/log 2>&1 &
done; done; wait
grep -c "No convergence" $S/x_*/log
```

| Image | BP iterations | Wall time | Warnings | Output size | Peak RSS |
|---|---|---|---|---|---|
| g256 (smooth) | 100 | 99.1 s | 0 | 59926 B | 200 MB |
| g256 (smooth) | 20 | 29.0 s | 0 | 59957 B | 200 MB |
| s256 (striped) | 100 | 169.1 s | 137 | 83650 B | 201 MB |
| s256 (striped) | 20 | 43.0 s | 137 | 83734 B | 201 MB |
| t64 (striped, 2 blocks) | 100 | 5.0 s | 3 | 9427 B | - |

Warnings per bitplane for s256 are identical at 100 and 20 iterations: bitplane 6: 10, 7: 55, 8: 67, 9: 5.

A linear fit `T = a + b * iterations` gives:

| Image | a (fixed) | b (per iteration) | Share of BP at 100 iterations |
|---|---|---|---|
| g256 | 11.5 s | 0.88 s | 88% |
| s256 | 11.4 s | 1.58 s | 93% |

Extrapolating s256 (about 5.2 s per block at 100 iterations): 512x256 takes about 11 min and 1024x512 about 45 min. This matches the first attempts.

### 3.3 Profiling

Unprivileged perf is blocked by default on Ubuntu (`kernel.perf_event_paranoid=4`), and gdb cannot attach to a running process (`kernel.yama.ptrace_scope=1`). The first setting was lowered for the session:

```bash
sudo sysctl kernel.perf_event_paranoid=1
```

60 s of the s256 encode at 100 iterations were recorded, then the main thread's self time was computed by sample count:

```bash
timeout -s INT 60 perf record -F 499 -g -o $S/perf.data \
    ./icov_cli -i $S/s256.ppm -o $S/x_perf/out/ -d data/imt_164_v2/
perf script -i $S/perf.data -F tid | sort | uniq -c | sort -rn | head
perf script -i $S/perf.data -F tid,ip,sym | awk '/^ *<main_tid> /{getline l; sub(/^[ \t]*[0-9a-f]+ /,"",l); c[l]++; n++}
    END{for(k in c) printf "%6.2f%% %s\n", 100*c[k]/n, k}' | sort -rn | head
```

Main thread (29964 samples, on CPU for the whole 60 s):

| Self time | Function |
|---|---|
| 80.7% | `Decoder_LDPCA_BP_flooding_SPA::_decode_single_ite` |
| 14.7% | `Decoder_LDPCA_BP_flooding_SPA::_decode` (inlined variable-node update and posterior) |
| 1.3% | `Decoder_LDPCA_BP_flooding_SPA` constructor (coder init) |
| < 0.3% each | transform, quantization, entropy estimate, LLR init |

The default `perf report` showed 35% in `libopenblasp` (`blas_thread_server`). That figure is a cycle-weighting artifact of the hybrid P/E cores: the 10 OpenBLAS threads have only about 58 samples each (about 0.1 s of CPU in total), so they cost nothing measurable. A Release build has no `-g`: function names are available but not line numbers, and inlined functions are merged into their callers. `RelWithDebInfo` would give line-level detail.

## 4. Analysis

### 4.1 Encoding loop

`FrameEncoder::encodeBlock` (`lib/encoder/FrameEncoder.cpp:313`) encodes each block for each color component (3) and each available prediction (up to 8 intra side informations plus INTER). For each of these, `InteractiveCoder::encode_bitplanes` codes each bitplane of the quantized coefficients. For one bitplane, `InteractiveCoder::encode` (`lib/icov/core/ldpc_codec_layer/InteractiveCoder.hpp:71`) runs:

```
for i = 0 .. 33:              # 34 rates, m = 4, 8, 16, ..., 256 (numcodes.txt)
    compute syndrome at rate i
    decode with BP            # full n_ite iterations
    if decoded == source: stop
```

The cost of one bitplane is therefore (number of rates tried) x `n_ite` BP iterations. A bitplane that never converges costs 34 x 100 iterations.

### 4.2 Early stopping is disabled

`Decoder_LDPCA_BP_flooding_SPA` builds its aff3ct base class with `enable_syndrome = false` (`lib/icov/ldpc_codec_layer/aff3ct/Decoder_LDPCA_BP_flooding_SPA.hxx:19`, constructor `Decoder_LDPC_BP(N, N, n_ite, _H, false, 1)`). The convergence test inside `_decode` is therefore never run, and every decode does all `n_ite` iterations, even when the bitplane is decoded after a few. The aff3ct `check_syndrome_soft` checks `H x = 0`, whereas LDPCA needs `H x = s`. This is probably why it was turned off, but nothing replaced it.

Measured effect: going from 100 to 20 iterations gives the same failures and the same output size within 0.1%. Most decodes that converge do so well before 20 iterations, so about 80% of the BP work is wasted.

### 4.3 Origin of the "No convergence" warnings

- The warning is thrown in `encode_bitplane` (`InteractiveCoder.hpp:403`) when BP still fails at the last rate. The bitplane is then stored raw (`SyndromeType_SOURCE`), and the final assertion `predec == bsp.src_symboles()` still passes, so coding stays lossless.
- The last rate has m = 256 = n, so it is rate 1. Above m = n/2 = 128, the extra syndrome bits are "doping", i.e. source bits sent directly, with their LLR set to the maximum. Raw storage costs the same n bits, so a fallback costs no rate, only time.
- Failures only affect bitplanes 6 to 9, the low-order magnitude bits of the quantized coefficients. For these bits the side information is nearly uninformative (LLR close to 0), so they need a rate close to 1, where BP is weakest.
- **Content dependence (measured):** the smooth gradient gives 0 failures, while the same image with diagonal stripes (`b = ((x + y) % 64) * 4`) gives 137. The stripes produce a 252-to-0 jump every 64 pixels, i.e. sharp diagonal edges in most 32x32 blocks, hence many large coefficients with uncertain low bits. The test image is close to a worst case.
- **Iteration budget is not the cause (measured):** 20 and 100 iterations give exactly the same failures.
- **Aggravating factor (confirmed in 6.3, but it cannot simply be changed, see 6.4):** the doping order is the identity. The sort by LLR reliability is commented out (`lib/icov/core/ldpc_codec_layer/LLRProvider.cpp:132`), so the doped bits are the first positions of the block rather than the least reliable ones. At full rate, BP must then solve the 128 remaining unreliable bits with 128 checks of the rate-1/2 matrix, which commonly fails on stopping sets.

### 4.4 Other factors

- OpenMP is off (`ICOV_USE_OMP=OFF`). The prediction loop in `FrameEncoder::encodeBlock` has a `#pragma omp parallel for` over up to 9 predictions, which is unused in this build. Not measured.
- The fixed cost (about 11 s at 256x128, 5 s at 64x32) covers coder initialization: 9 prediction coders x 2 code lengths, and the matrix construction in `IMT164MatrixProvider`.

## 5. Conclusions and recommendations

| # | Action | Expected gain | Status |
|---|---|---|---|
| 1 | Use `--ldpc-bp-ite 20` for tests and tutorials | 3.4x to 3.9x | measured |
| 2 | Add an LDPCA syndrome check (`H x == s`) in `_decode` and enable early stopping | most of the remaining BP time for converging decodes | implemented (section 6) |
| 3 | Start the rate search near the expected rate (e.g. from the conditional entropy estimate), or use a bisection, instead of a linear scan from the lowest rate | fewer decodes per bitplane | implemented (section 6) |
| 4 | Skip LDPC and store raw when the estimated rate is close to 1 | removes the 34-attempt cost of the failing bitplanes | replaced by the rate search and the rate-1 cap (section 6) |
| 5 | Build with `-DICOV_USE_OMP=ON` | up to 9x on the prediction loop | in README, measured about 3x (section 6) |
| 6 | Restore the doping order by LLR reliability and check whether full-rate failures disappear | fewer raw bitplanes | tested and reverted: incompatible with the shared syndrome (section 6.4) |
| 7 | Use a smooth gradient (no stripes) as the tutorial test image | 0 warnings, 1.7x faster | implemented: generator default, `--stripes` for the hard case |

Items 2 and 3 change encoder behavior. They should be validated against the golden outputs planned in the maintenance plan (bit-exact output, or an equal or lower rate). Section 6 reports what was implemented.

## 6. Implemented changes and results

### 6.1 Changes

| # | Change | Location |
|---|---|---|
| 2 | Early stopping: after each BP iteration, stop as soon as the hard decision satisfies the LDPCA syndrome (`H x = s`) | `Decoder_LDPCA_BP_flooding_SPA::_check_syndrome`, `_decode` |
| 3 | Rate search: start from the conditional entropy estimate minus 0.05, then use exponential steps up to the first converging rate and bisect back down. Each attempt starts from a fresh doping state, as in the decoder | `InteractiveCoder::encode`, `InteractiveCoder::conditional_entropy`, `LLRProvider::reset_doping` |
| 4 | Rates above 1 are no longer tried (luma codes go up to m = 1520 for n = 1024), because raw storage is cheaper | `InteractiveCoder::encode` |
| 5 | `-DICOV_USE_OMP=ON` added to the README build command | `README.md` |

A separate skip-to-raw threshold (original item 4) was not needed: with the new search, a failing bitplane costs about log2(rates) decodes instead of all of them.

**Compatibility:** items 2 to 4 change the rates the encoder chooses but not the bitstream format, so files encoded before these changes still decode. One theoretical exception: with early stopping, the decoder could stop on another word with the same syndrome, which requires the difference to be a codeword of H. This was not observed.

### 6.2 Method

To find the starting rate, the encoder was temporarily instrumented to log, for each bitplane, the conditional entropy estimate `H = mean(-log2 P(x_i | llr_i))` and the rate the linear search selected (8579 bitplanes, g256 and s256):

- n = 256: 5369 bitplanes, 89 failures. n = 1024: 3210 bitplanes, 10 failures.
- n = 1024 uses 87% of the failed-decode work (158 rates in steps of 8/1024).
- Chosen rate minus H: median +0.021, 5th percentile -0.027, 1st percentile -0.041. 28% of the bitplanes converge slightly below H (finite-length effects and LLR model mismatch).
- A start at H - 0.05 alone would remove only 19% of the decodes. This led to the exponential search plus bisection.

Each variant was measured with the script below. It encodes, decodes 5 viewpoints headless against the ground truth, and compares per-block bitrate and PSNR from the decoder CSV:

```bash
icov_cli -i $S/$img.ppm -o $d/out/ -d data/imt_164_v2/ [options]
icov_cli -m decode -i $d/out -d data/imt_164_v2/ \
    --decode-positions-input $S/positions.csv --decode-ground-truth $S/$img.ppm \
    --decode-csv-output $d/csv/res --decode-headless 1
```

`positions.csv` holds 5 viewpoints (`time,latitude,longitude`): `0,0,0`, `0,10,45`, `0,-20,-90`, `0,30,170`, `0,-60,120`. They cover 50 block decodes. "Stored" is the size of `syndrome` + `prediction`, and "Viewed bits" is the sum of the encoded bitrate of the decoded blocks.

### 6.3 Results (256x128, 100 BP iterations unless noted)

| Variant | Image | Encode time | Warnings | Stored | Viewed bits | Mean PSNR |
|---|---|---|---|---|---|---|
| baseline | g256 | 110.0 s | 0 | 31387 B | 108608 | 51.363 dB |
| + early stop (2) | g256 | 119.0 s | 0 | 31372 B | 108576 | 51.363 dB |
| + rate search (3, 4) | g256 | 24.4 s | 0 | 31376 B | 108704 | 51.363 dB |
| + OpenMP (5) | g256 | 8.3 s | 0 | 31376 B | 108704 | 51.363 dB |
| (doping order, reverted) | g256 | 8.6 s | 0 | 30858 B | 107680 | 51.363 dB |
| baseline | s256 | 207.1 s | 137 | 34623 B | 189208 | 45.475 dB |
| + early stop (2) | s256 | 198.5 s | 99 | 34615 B | 188848 | 45.475 dB |
| + rate search (3, 4) | s256 | 32.9 s | 100 | 34576 B | 189352 | 45.475 dB |
| + OpenMP (5) | s256 | 11.1 s | 100 | 34576 B | 189352 | 45.475 dB |
| (doping order, reverted) | s256 | 11.2 s | 0 | 33168 B | 172104 | 45.475 dB |

The baseline, early stop and rate search rows ran two or four encodes in parallel. OpenMP and later rows ran one at a time. The 512x256 result is in 6.5.

Observations:

- Early stopping alone does not reduce time: within a linear search, all attempts but the last fail and still run every iteration. It does reduce warnings (137 to 99), because full-length runs sometimes moved away from a decode that had already converged.
- The rate search gives 4.5x to 6.3x, with viewed bits within +0.1% of the baseline.
- The OpenMP build produces byte-identical output to the serial build (`syndrome`, `prediction`, `header` compared with `cmp`) and runs about 3x faster on this machine.
- Doping by reliability removed the full-rate failures (s256: 100 to 0, viewed bits -9.1%), but it is not valid, see 6.4.
- Mean PSNR is identical in every variant: all decoded blocks are exact, since the coding is lossless after quantization.
- Overall, with the kept changes and 100 iterations, encoding is 13x (g256) to 19x (s256) faster than the baseline, with stored size within 0.2% and identical PSNR. The OpenMP output is deterministic: three runs gave byte-identical files.

### 6.4 Doping order by LLR reliability: tested and reverted

A block stores one syndrome per bitplane, the longest across its predictions, and each prediction reads a prefix of it (`FrameEncoder::saveDataToEncode`, `lib/encoder/FrameEncoder.cpp:680`). The prefix is valid for every prediction only if the syndrome bits do not depend on the side information:

- The parity bits `H x` depend only on the source.
- With the identity doping order, the doped bits are `src[0..k]`, which also depends only on the source.
- With an LLR-sorted order, the doped bits are the source bits at the least reliable positions of the prediction that produced the stored syndrome. Another prediction reading a prefix in the doped range would get bits for the wrong positions and decode incorrectly.

The symptom was nondeterministic OpenMP output: when two predictions reach the same length, the first thread to write wins, and three runs gave three different `syndrome` files. The headless decode test (5 viewpoints, 50 block decodes, each through a single prediction) did not exercise the faulty case, so identical PSNR there did not prove correctness.

The full-rate failures are therefore the cost of a doping order that is identical for all predictions. Removing them needs another order with that property, e.g. one based on the matrix structure (variable nodes that break stopping sets). This is left as a research item. A useful test to add would decode every prediction of every block from the final stored syndrome.

### 6.5 BP iterations with the kept changes

| Variant | Image | Encode time | Warnings | Stored |
|---|---|---|---|---|
| all kept, 100 iterations | g256 | 8.3 s | 0 | 31376 B |
| all kept, 100 iterations | s256 | 10.3 s to 11.1 s | 100 | 34576 B |
| all kept, 20 iterations | g256 | 3.1 s | 0 | 31410 B |
| all kept, 20 iterations | s256 | 3.8 s | 100 | 34637 B |
| all kept, 100 iterations | 512x256 striped | 35.6 s | 442 | 118348 B |

20 iterations is about 3x faster for +0.1% to +0.2% size and identical PSNR. It is a good setting for quick tests, while the default stays at 100. The 512x256 striped image, which had not finished after 7 minutes, now encodes in 36 s.
