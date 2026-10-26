#!/usr/bin/env bash
# End-to-end test: encode synthetic ERP images twice (output must be
# identical), then decode a few viewpoints headless against the ground truth
# and check the PSNR of every decoded block.
# Usage: roundtrip.sh <bin dir> <python> <scripts dir> <work dir>
set -euo pipefail

bin=$1
python=$2
scripts=$3
work=$4
data=data/imt_164_v2/
min_psnr=35

rm -rf "$work"
mkdir -p "$work"
cd "$bin"
printf '0,0,0\n0,10,90\n0,-30,-120\n0,60,180\n' > "$work/positions.csv"

for pattern in gradient stripes; do
  img=$work/$pattern.ppm
  if [ "$pattern" = stripes ]; then
    "$python" "$scripts/gen_synthetic_erp.py" "$img" --width 128 --stripes
  else
    "$python" "$scripts/gen_synthetic_erp.py" "$img" --width 128
  fi

  for run in 1 2; do
    ./icov_cli -i "$img" -o "$work/${pattern}_$run/" -d "$data" \
      > "$work/${pattern}_$run.log" 2>&1
  done
  for f in header prediction syndrome; do
    cmp "$work/${pattern}_1/$f" "$work/${pattern}_2/$f" ||
      { echo "FAIL $pattern: $f differs between two encodes"; exit 1; }
  done

  ./icov_cli -m decode -i "$work/${pattern}_1" -d "$data" \
    --decode-positions-input "$work/positions.csv" \
    --decode-ground-truth "$img" \
    --decode-csv-output "$work/$pattern" --decode-headless 1 \
    > "$work/${pattern}_decode.log" 2>&1

  # column 14: psnr of the decoded block
  awk -F, -v min="$min_psnr" -v name="$pattern" '
    NR > 1 { n++; if ($14 < min) { bad++; print "FAIL " name ": block " $2 " psnr " $14 } }
    END {
      if (n == 0) { print "FAIL " name ": no decoded block"; exit 1 }
      if (bad > 0) exit 1
      print "OK " name ": " n " blocks decoded, psnr >= " min
    }' "$work/${pattern}_blocks.csv"
done
