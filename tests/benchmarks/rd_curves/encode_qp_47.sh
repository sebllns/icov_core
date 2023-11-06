#!/bin/sh

cd /home/sbelleno/_ICOV/CODE/adt.git/build/release/bin

./icov_cli --log-level 2 --ldpc-coder-type "imt164" --ldpc-data-path "./data/imt_164_v2/" --encode-qp 47  --encode-icu-size 32 --mode "encode"  -i "/home/sbelleno/NOSAVE/icov/images/Oberkaisern/Oberkaisern_1280.jpg" -o "/home/sbelleno/NOSAVE/icov/images/Oberkaisern/Oberkaisern_1280_2/Oberkaisern_1280_47.icov" --encode-viewport-image-ratio 1.7778 --encode-viewport-erp-horizontal-ratio 0.4