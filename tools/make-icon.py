#!/usr/bin/env python3
"""Generate resources/icon.hvif: an original HVIF (Haiku Vector Icon Format)
icon for Tansu, a small tansu chest / shelf-ledge shape on a 64x64 canvas.

The byte layout mirrors Haiku's src/libs/icon/flat_icon/FlatIconImporter.cpp:
  uint32 magic FLAT_ICON_MAGIC = 'ficn' (0x6669636e), little-endian, so the
    file starts with the bytes "ncif"
  styles:  u8 count; per style u8 type + payload
  paths:   u8 count; per path u8 flags, u8 pointCount, coords
  shapes:  u8 count; per shape u8 type, u8 styleIndex, u8 pathCount,
           path indices, u8 shapeFlags

MIT License, Copyright (c) 2026 Westenra Softworks.
"""

import argparse
import struct
import sys

FLAT_ICON_MAGIC = 0x6669636E  # 'ficn' in FlatIconFormat.cpp
MAGIC = struct.pack("<I", FLAT_ICON_MAGIC)  # b"ncif"

STYLE_SOLID_COLOR = 1
PATH_FLAG_CLOSED = 1 << 1
PATH_FLAG_NO_CURVES = 1 << 3
SHAPE_TYPE_PATH_SOURCE = 10


def write_coord(value):
    """Mirror write_coord() in FlatIconFormat.cpp."""
    value = max(-128.0, min(192.0, value))
    if int(value * 100.0) == int(value) * 100 and -32.0 <= value <= 95.0:
        return struct.pack("B", int(value + 32.0) & 0xFF)
    raw = int((value + 128.0) * 102.0) | 0x8000
    return struct.pack("BB", (raw >> 8) & 0xFF, raw & 0xFF)


def solid_color_style(rgb):
    """STYLE_SOLID_COLOR: a raw rgb_color, i.e. red, green, blue, alpha."""
    r, g, b = rgb
    return bytes([STYLE_SOLID_COLOR, r, g, b, 255])


def rect_path(x0, y0, x1, y1):
    """Closed straight-line rectangle path."""
    points = [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]
    data = bytes([PATH_FLAG_CLOSED | PATH_FLAG_NO_CURVES, len(points)])
    for x, y in points:
        data += write_coord(x) + write_coord(y)
    return data


def build_icon():
    # styles: light wood (lid), dark wood (body), dark line, brass handle
    styles = [
        solid_color_style((0xB0, 0x6A, 0x2E)),
        solid_color_style((0x78, 0x48, 0x1E)),
        solid_color_style((0x3C, 0x24, 0x10)),
        solid_color_style((0xE6, 0xBE, 0x50)),
    ]

    # paths: lid, body, two drawer separators, three handles
    paths = [
        rect_path(8, 10, 56, 20),
        rect_path(10, 18, 54, 56),
        rect_path(12, 31, 52, 33),
        rect_path(12, 43, 52, 45),
        rect_path(29, 23, 35, 26),
        rect_path(29, 37, 35, 40),
        rect_path(29, 49, 35, 52),
    ]

    shape_styles = [0, 1, 2, 2, 3, 3, 3]

    data = bytearray(MAGIC)
    data += bytes([len(styles)])
    for style in styles:
        data += style
    data += bytes([len(paths)])
    for path in paths:
        data += path
    data += bytes([len(shape_styles)])
    for i, style_index in enumerate(shape_styles):
        data += bytes([SHAPE_TYPE_PATH_SOURCE, style_index, 1, i, 0])
    return bytes(data)


def verify(data):
    """Re-parse per FlatIconImporter::_ParseSections() to prove validity."""
    pos = [0]

    def read(n):
        chunk = data[pos[0]:pos[0] + n]
        if len(chunk) != n:
            raise ValueError("unexpected end of data")
        pos[0] += n
        return chunk

    def read_u8():
        return read(1)[0]

    if struct.unpack("<I", read(4))[0] != FLAT_ICON_MAGIC:
        raise ValueError("bad magic")

    style_count = read_u8()
    for _ in range(style_count):
        style_type = read_u8()
        if style_type != STYLE_SOLID_COLOR:
            raise ValueError("unsupported style type %d" % style_type)
        read(4)

    path_count = read_u8()
    for _ in range(path_count):
        flags = read_u8()
        point_count = read_u8()
        for _ in range(point_count):
            for _ in range(2):
                first = read_u8()
                if first & 128:
                    read_u8()
        if not flags & (PATH_FLAG_CLOSED | PATH_FLAG_NO_CURVES):
            raise ValueError("unexpected path flags %d" % flags)

    shape_count = read_u8()
    for _ in range(shape_count):
        shape_type = read_u8()
        if shape_type != SHAPE_TYPE_PATH_SOURCE:
            raise ValueError("unsupported shape type %d" % shape_type)
        style_index = read_u8()
        shape_path_count = read_u8()
        indices = read(shape_path_count)
        flags = read_u8()
        if (style_index >= style_count or max(indices) >= path_count
                or flags):
            raise ValueError("bad shape reference")

    if pos[0] != len(data):
        raise ValueError("trailing bytes")
    return True


def rdef_hex_lines(data, chunk=32):
    return ['\t$"%s"' % data[i:i + chunk].hex() for i in range(0, len(data), chunk)]


def main():
    parser = argparse.ArgumentParser(description="Generate the Tansu HVIF icon.")
    parser.add_argument("output", nargs="?", default="resources/icon.hvif")
    parser.add_argument("--print-rdef", action="store_true",
        help="print the vector_icon hex lines for app.rdef")
    args = parser.parse_args()

    data = build_icon()
    verify(data)

    with open(args.output, "wb") as f:
        f.write(data)

    if args.print_rdef:
        for line in rdef_hex_lines(data):
            print(line)
    print("wrote %s (%d bytes, verified)" % (args.output, len(data)),
        file=sys.stderr)


if __name__ == "__main__":
    main()
