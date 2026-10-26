#!/usr/bin/env python3
"""Generate a synthetic equirectangular (ERP) test image as binary PPM.

Red follows longitude, green follows latitude, blue is constant. With
--stripes, blue becomes a diagonal stripe pattern with sharp edges, a much
harder content for the encoder. Only numpy is required.
"""

import argparse

import numpy as np


def make_erp(width, height, stripes=False):
    y, x = np.mgrid[0:height, 0:width]
    r = x * 255 // width
    g = y * 255 // height
    b = ((x + y) % 64) * 4 if stripes else np.full_like(x, 128)
    return np.stack([r, g, b], axis=-1).astype(np.uint8)


def write_ppm(path, img):
    h, w, _ = img.shape
    with open(path, "wb") as f:
        f.write(b"P6 %d %d 255\n" % (w, h))
        f.write(img.tobytes())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", help="output .ppm file")
    parser.add_argument("--width", type=int, default=1024)
    parser.add_argument(
        "--height", type=int, default=None, help="default: width / 2 (2:1 ERP ratio)"
    )
    parser.add_argument("--stripes", action="store_true",
                        help="add diagonal stripes (sharp edges) on blue")
    args = parser.parse_args()

    height = args.height or args.width // 2
    write_ppm(args.output, make_erp(args.width, height, args.stripes))


if __name__ == "__main__":
    main()
