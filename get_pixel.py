#!/usr/bin/env python3
import sys
import struct

def read_ppm(filename):
    with open(filename, 'rb') as f:
        magic = f.readline().strip()
        line = f.readline()
        while line.startswith(b'#'):
            line = f.readline()
        width, height = map(int, line.split())
        maxval = int(f.readline().strip())
        data = f.read()
    return width, height, data

def get_pixel(data, width, x, y):
    idx = (y * width + x) * 3
    return data[idx], data[idx+1], data[idx+2]

if len(sys.argv) < 5:
    print(f"Usage: {sys.argv[0]} file1.ppm file2.ppm x y")
    sys.exit(1)

file1 = sys.argv[1]
file2 = sys.argv[2]
x = int(sys.argv[3])
y = int(sys.argv[4])

w1, h1, d1 = read_ppm(file1)
w2, h2, d2 = read_ppm(file2)

print(f"Comparing pixel ({x}, {y}):")
r1, g1, b1 = get_pixel(d1, w1, x, y)
r2, g2, b2 = get_pixel(d2, w2, x, y)

print(f"  {file1}: RGB({r1}, {g1}, {b1})")
print(f"  {file2}: RGB({r2}, {g2}, {b2})")

if (r1, g1, b1) != (r2, g2, b2):
    print(f"  DIFFERENT!")
else:
    print(f"  IDENTICAL")
