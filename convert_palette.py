#!/usr/bin/env python3

import numpy as np
import sys
import struct

def pixel_to_float(color):
  a, r, g, b = color.to_bytes(4)
  return [r / 255.0, g / 255.0, b / 255.0]

if __name__ == "__main__":
  with open(sys.argv[1], "rb") as file:
    while True:
      pixel = file.read(4)
      if not pixel:
        break
      color = struct.unpack('<I', pixel)[0]
      a, b, g, r = color.to_bytes(4)
      print("{ %f, %f, %f }," % (r/255.0, g/255.0, b/255.0))

