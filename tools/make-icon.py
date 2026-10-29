#!/usr/bin/env python3
"""Convert resources/icon.svg into resources/icon.hvif (Haiku Vector Icon Format)
and refresh the vector_icon resource in resources/app.rdef.

Only the SVG subset the Tansu icon uses is supported: <polygon>, <polyline>,
<line>, <path> (M, L, Q, Z), <circle>, <ellipse>, <g> (fill/stroke inheritance,
clip-path with a convex <polygon>), solid colours and userSpaceOnUse
<linearGradient>s. Curves, circles and strokes are flattened into polygons, so
every HVIF path uses PATH_FLAG_NO_CURVES.

The byte layout follows Haiku's src/libs/icon/flat_icon/FlatIconImporter.cpp:
  uint32 magic FLAT_ICON_MAGIC = 'ficn' (0x6669636e), little-endian, so the
    file starts with the bytes "ncif"
  styles:  u8 count; per style u8 type + payload
           solid colour: raw rgb_color (R, G, B, A)
           gradient: u8 type, u8 flags, u8 stop count, [6 x float24 matrix],
                     per stop u8 offset + R, G, B, A
  paths:   u8 count; per path u8 flags, u8 point count, coords
  shapes:  u8 count; per shape u8 type, u8 style index, u8 path count,
           path indices, u8 shape flags
Shapes are filled with the non-zero rule (AGG's default in IconRenderer.cpp).

MIT License, Copyright (c) 2026 Westenra Softworks.
"""

import argparse
import math
import re
import struct
import sys
import xml.etree.ElementTree as ET

FLAT_ICON_MAGIC = 0x6669636E  # 'ficn' in FlatIconFormat.cpp
MAGIC = struct.pack("<I", FLAT_ICON_MAGIC)  # b"ncif"

STYLE_SOLID_COLOR = 1
STYLE_GRADIENT = 2
GRADIENT_LINEAR = 0
GRADIENT_FLAG_TRANSFORM = 1 << 1
PATH_FLAG_CLOSED = 1 << 1
PATH_FLAG_NO_CURVES = 1 << 3
SHAPE_TYPE_PATH_SOURCE = 10

SVG_NS = "{http://www.w3.org/2000/svg}"
CURVE_STEPS = 6
CIRCLE_POINTS = 14
MAX_COUNT = 255


# ---------------------------------------------------------------- encoding

def write_coord(value):
    """Mirror write_coord() in FlatIconFormat.cpp."""
    value = max(-128.0, min(192.0, value))
    if int(value * 100.0) == int(value) * 100 and -32.0 <= value <= 95.0:
        return struct.pack("B", int(value + 32.0) & 0xFF)
    raw = int((value + 128.0) * 102.0) | 0x8000
    return struct.pack("BB", (raw >> 8) & 0xFF, raw & 0xFF)


def write_float_24(value):
    """Mirror write_float_24() in FlatIconFormat.cpp."""
    bits = struct.unpack("<I", struct.pack("<f", value))[0]
    sign = bits >> 31
    exponent = ((bits >> 23) & 0xFF) - 127
    mantissa = bits & 0x7FFFFF
    if exponent >= 32 or exponent < -32:
        return b"\0\0\0"
    short = (sign << 23) | ((exponent + 32) << 17) | (mantissa >> 6)
    return bytes([(short >> 16) & 0xFF, (short >> 8) & 0xFF, short & 0xFF])


# ---------------------------------------------------------------- SVG parsing

def parse_color(text, opacity=1.0):
    text = text.strip()
    if not text.startswith("#"):
        raise ValueError("unsupported colour %r" % text)
    h = text[1:]
    if len(h) == 3:
        h = "".join(c * 2 for c in h)
    r, g, b = int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16)
    return (r, g, b, int(round(255 * opacity)))


def parse_points(text):
    nums = [float(n) for n in re.findall(r"-?\d*\.?\d+(?:e-?\d+)?", text)]
    return list(zip(nums[0::2], nums[1::2]))


def parse_path(d):
    """Return (list of subpaths as point lists, closed flags) for M/L/Q/Z."""
    tokens = re.findall(r"[MLQZ]|-?\d*\.?\d+(?:e-?\d+)?", d)
    subpaths, closed, current, i, cmd = [], [], None, 0, None
    while i < len(tokens):
        if tokens[i] in "MLQZ":
            cmd = tokens[i]
            i += 1
            if cmd == "Z":
                closed[-1] = True
                continue
        nums = lambda n: [float(t) for t in tokens[i:i + n]]
        if cmd == "M":
            current = [tuple(nums(2))]
            subpaths.append(current)
            closed.append(False)
            i += 2
            cmd = "L"
        elif cmd == "L":
            current.append(tuple(nums(2)))
            i += 2
        elif cmd == "Q":
            cx, cy, x, y = nums(4)
            x0, y0 = current[-1]
            for k in range(1, CURVE_STEPS + 1):
                t = k / CURVE_STEPS
                current.append(((1 - t) ** 2 * x0 + 2 * (1 - t) * t * cx + t * t * x,
                                (1 - t) ** 2 * y0 + 2 * (1 - t) * t * cy + t * t * y))
            i += 4
        else:
            raise ValueError("unsupported path command %r" % cmd)
    return subpaths, closed


def ellipse_points(cx, cy, rx, ry, n=CIRCLE_POINTS):
    return [(cx + rx * math.cos(2 * math.pi * k / n), cy + ry * math.sin(2 * math.pi * k / n))
            for k in range(n)]


# ---------------------------------------------------------------- geometry

def area(poly):
    return 0.5 * sum(x0 * y1 - x1 * y0
                     for (x0, y0), (x1, y1) in zip(poly, poly[1:] + poly[:1]))


def oriented(poly, ccw=True):
    return poly if (area(poly) > 0) == ccw else poly[::-1]


def convex_hull(points):
    pts = sorted(set(points))
    if len(pts) < 3:
        return pts

    def half(seq):
        out = []
        for p in seq:
            while len(out) >= 2 and ((out[-1][0] - out[-2][0]) * (p[1] - out[-2][1])
                                     - (out[-1][1] - out[-2][1]) * (p[0] - out[-2][0])) <= 0:
                out.pop()
            out.append(p)
        return out

    lower, upper = half(pts), half(pts[::-1])
    return lower[:-1] + upper[:-1]


def is_convex(poly):
    sign = 0
    n = len(poly)
    for i in range(n):
        (x0, y0), (x1, y1), (x2, y2) = poly[i], poly[(i + 1) % n], poly[(i + 2) % n]
        cross = (x1 - x0) * (y2 - y1) - (y1 - y0) * (x2 - x1)
        if abs(cross) > 1e-9:
            if sign == 0:
                sign = 1 if cross > 0 else -1
            elif (cross > 0) != (sign > 0):
                return False
    return True


def grow_convex(poly, r):
    """Outward offset of a convex polygon with round joins (Minkowski sum with a disc)."""
    pts = []
    for x, y in poly:
        pts += ellipse_points(x, y, r, r, 8)
    return convex_hull(pts)


def shrink_convex(poly, r):
    """Inward offset of a convex polygon (mitred); returns None if it collapses."""
    poly = oriented(poly, True)
    n = len(poly)
    lines = []
    for i in range(n):
        (x0, y0), (x1, y1) = poly[i], poly[(i + 1) % n]
        dx, dy = x1 - x0, y1 - y0
        length = math.hypot(dx, dy)
        if length < 1e-9:
            continue
        nx, ny = -dy / length, dx / length  # inward normal for a CCW polygon
        lines.append(((x0 + nx * r, y0 + ny * r), (dx, dy)))
    out = []
    for i in range(len(lines)):
        (p, d), (q, e) = lines[i - 1], lines[i]
        den = d[0] * e[1] - d[1] * e[0]
        if abs(den) < 1e-9:
            out.append(q)
            continue
        t = ((q[0] - p[0]) * e[1] - (q[1] - p[1]) * e[0]) / den
        out.append((p[0] + d[0] * t, p[1] + d[1] * t))
    if len(out) < 3 or area(out) <= 0:
        return None
    return out


def stroke_polyline(pts, width, round_caps):
    """One polygon outlining an open polyline (good for gentle curves)."""
    r = width / 2
    clean = [pts[0]]
    for p in pts[1:]:
        if math.hypot(p[0] - clean[-1][0], p[1] - clean[-1][1]) > 1e-6:
            clean.append(p)
    pts = clean
    if len(pts) < 2:
        return ellipse_points(pts[0][0], pts[0][1], r, r, 12)
    normals = []
    for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
        length = math.hypot(x1 - x0, y1 - y0)
        normals.append((-(y1 - y0) / length, (x1 - x0) / length))
    left, right = [], []
    for i, (x, y) in enumerate(pts):
        if i == 0:
            nx, ny = normals[0]
        elif i == len(pts) - 1:
            nx, ny = normals[-1]
        else:
            ax, ay = normals[i - 1]
            bx, by = normals[i]
            nx, ny = ax + bx, ay + by
            length = math.hypot(nx, ny)
            nx, ny = nx / length, ny / length
            # mitre length, limited so sharp bends do not spike
            scale = min(1 / max(nx * bx + ny * by, 0.5), 2)
            nx, ny = nx * scale, ny * scale
        left.append((x + nx * r, y + ny * r))
        right.append((x - nx * r, y - ny * r))

    def cap(center, normal, steps=6):
        cx, cy = center
        a0 = math.atan2(normal[1], normal[0])
        return [(cx + r * math.cos(a0 - math.pi * k / steps), cy + r * math.sin(a0 - math.pi * k / steps))
                for k in range(1, steps)]

    poly = left[:]
    if round_caps:
        poly += cap(pts[-1], normals[-1])
    poly += right[::-1]
    if round_caps:
        n = normals[0]
        poly += cap(pts[0], (-n[0], -n[1]))
    return poly


def clip_convex(subject, clip):
    """Sutherland-Hodgman: clip any polygon against a convex polygon."""
    clip = oriented(clip, True)
    out = subject
    for (ax, ay), (bx, by) in zip(clip, clip[1:] + clip[:1]):
        inside = lambda p: (bx - ax) * (p[1] - ay) - (by - ay) * (p[0] - ax) >= 0
        src, out = out, []
        for i, p in enumerate(src):
            q = src[i - 1]
            if inside(p):
                if not inside(q):
                    out.append(intersect(q, p, (ax, ay), (bx, by)))
                out.append(p)
            elif inside(q):
                out.append(intersect(q, p, (ax, ay), (bx, by)))
        if not out:
            break
    return out


def intersect(p, q, a, b):
    x1, y1 = p; x2, y2 = q; x3, y3 = a; x4, y4 = b
    den = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4)
    t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / den
    return (x1 + t * (x2 - x1), y1 + t * (y2 - y1))


# ---------------------------------------------------------------- SVG -> shapes

class Converter:
    def __init__(self, root):
        self.gradients, self.clips = {}, {}
        self.shapes = []  # (paint, [polygons])
        for el in root.iter():
            tag = el.tag.replace(SVG_NS, "")
            if tag == "linearGradient":
                stops = []
                for st in el:
                    stops.append((float(st.get("offset")),
                                  parse_color(st.get("stop-color"), float(st.get("stop-opacity", 1)))))
                if el.get("gradientUnits") != "userSpaceOnUse":
                    raise ValueError("only userSpaceOnUse gradients are supported")
                self.gradients[el.get("id")] = (
                    tuple(float(el.get(k)) for k in ("x1", "y1", "x2", "y2")), stops)
            elif tag == "clipPath":
                poly = parse_points(el.find(SVG_NS + "polygon").get("points"))
                if not is_convex(poly):
                    raise ValueError("clip paths must be convex polygons")
                self.clips[el.get("id")] = poly
        for child in root:
            self.walk(child, {}, None)

    def paint(self, value):
        if value is None or value == "none":
            return None
        m = re.match(r"url\(#(.+)\)", value)
        if m:
            return ("gradient",) + self.gradients[m.group(1)]
        return ("solid", parse_color(value))

    def add(self, paint, polys, clip):
        if paint is None:
            return
        if clip is not None:
            polys = [clip_convex(p, clip) for p in polys]
        polys = [p for p in polys if len(p) >= 3 and abs(area(p)) > 1e-4]
        if polys:
            self.shapes.append((paint, polys))

    def walk(self, el, inherited, clip):
        tag = el.tag.replace(SVG_NS, "")
        if tag in ("defs", "linearGradient", "clipPath", "stop"):
            return
        attrs = dict(inherited)
        for key in ("fill", "stroke", "stroke-width", "stroke-linecap"):
            if el.get(key) is not None:
                attrs[key] = el.get(key)
        if el.get("transform"):
            raise ValueError("transform attributes are not supported")
        m = re.match(r"url\(#(.+)\)", el.get("clip-path", ""))
        if m:
            clip = self.clips[m.group(1)]
        if tag == "g":
            for child in el:
                self.walk(child, attrs, clip)
            return

        fill = self.paint(attrs.get("fill", "#000"))
        stroke = self.paint(attrs.get("stroke"))
        width = float(attrs.get("stroke-width", 1))
        round_caps = attrs.get("stroke-linecap") == "round"

        if tag in ("polygon", "polyline"):
            subpaths, closed = [parse_points(el.get("points"))], [tag == "polygon"]
        elif tag == "path":
            subpaths, closed = parse_path(el.get("d"))
        elif tag == "line":
            subpaths = [[(float(el.get("x1")), float(el.get("y1"))),
                         (float(el.get("x2")), float(el.get("y2")))]]
            closed, fill = [False], None
        elif tag in ("circle", "ellipse"):
            cx, cy = float(el.get("cx")), float(el.get("cy"))
            rx = float(el.get("r") or el.get("rx"))
            ry = float(el.get("r") or el.get("ry"))
            subpaths, closed = [ellipse_points(cx, cy, rx, ry)], [True]
        else:
            raise ValueError("unsupported element <%s>" % tag)

        filled = [p for p in subpaths if len(p) >= 3]
        if stroke is not None and stroke == fill and all(closed) and all(is_convex(p) for p in filled):
            # "silhouette outline" idiom: fill + same-colour stroke = grown shape
            self.add(fill, [grow_convex(p, width / 2) for p in filled], clip)
            return
        if tag != "line" and tag != "polyline":
            self.add(fill, filled, clip)
        if stroke is None:
            return
        polys = []
        for pts, is_closed in zip(subpaths, closed):
            if is_closed and is_convex(pts):
                outer = oriented(grow_convex(pts, width / 2), True)
                inner = shrink_convex(pts, width / 2)
                polys.append(outer)
                if inner:
                    polys.append(oriented(inner, False))  # hole under the non-zero rule
            elif is_closed:
                polys.append(stroke_polyline(pts + pts[:2], width, False))
            else:
                polys.append(stroke_polyline(pts, width, round_caps))
        self.add(stroke, polys, clip)


# ---------------------------------------------------------------- HVIF writing

def gradient_matrix(x1, y1, x2, y2):
    """Affine (sx, shy, shx, sy, tx, ty) mapping gradient space -64..64 on x
    (IconRenderer.cpp, GRADIENT_LINEAR) onto the segment (x1,y1)-(x2,y2)."""
    dx, dy = x2 - x1, y2 - y1
    s = math.hypot(dx, dy) / 128.0
    angle = math.atan2(dy, dx)
    c, n = math.cos(angle) * s, math.sin(angle) * s
    return (c, n, -n, c, (x1 + x2) / 2, (y1 + y2) / 2)


def encode_style(paint):
    if paint[0] == "solid":
        return bytes([STYLE_SOLID_COLOR]) + bytes(paint[1])
    _, (x1, y1, x2, y2), stops = paint
    data = bytes([STYLE_GRADIENT, GRADIENT_LINEAR, GRADIENT_FLAG_TRANSFORM, len(stops)])
    for value in gradient_matrix(x1, y1, x2, y2):
        data += write_float_24(value)
    for offset, color in stops:
        data += bytes([int(round(offset * 255))]) + bytes(color)
    return data


def encode_path(poly):
    if len(poly) > MAX_COUNT:
        raise ValueError("path with %d points" % len(poly))
    data = bytes([PATH_FLAG_CLOSED | PATH_FLAG_NO_CURVES, len(poly)])
    for x, y in poly:
        data += write_coord(x) + write_coord(y)
    return data


def build_icon(svg_path):
    conv = Converter(ET.parse(svg_path).getroot())
    styles, style_index = [], {}
    paths, path_index = [], {}
    shapes = []
    for paint, polys in conv.shapes:
        style = encode_style(paint)
        if style not in style_index:
            style_index[style] = len(styles)
            styles.append(style)
        indices = []
        for poly in polys:
            path = encode_path(poly)
            if path not in path_index:
                path_index[path] = len(paths)
                paths.append(path)
            indices.append(path_index[path])
        shapes.append((style_index[style], indices))

    for name, count in (("styles", len(styles)), ("paths", len(paths)), ("shapes", len(shapes))):
        if count > MAX_COUNT:
            raise ValueError("too many %s: %d (HVIF allows %d)" % (name, count, MAX_COUNT))

    data = bytearray(MAGIC)
    data += bytes([len(styles)]) + b"".join(styles)
    data += bytes([len(paths)]) + b"".join(paths)
    data += bytes([len(shapes)])
    for style, indices in shapes:
        data += bytes([SHAPE_TYPE_PATH_SOURCE, style, len(indices)] + indices + [0])
    return bytes(data), (len(styles), len(paths), len(shapes))


# ---------------------------------------------------------------- verification

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
        if style_type == STYLE_SOLID_COLOR:
            read(4)
        elif style_type == STYLE_GRADIENT:
            read_u8()
            flags = read_u8()
            stops = read_u8()
            if flags & GRADIENT_FLAG_TRANSFORM:
                read(18)
            read(5 * stops)
        else:
            raise ValueError("unsupported style type %d" % style_type)

    path_count = read_u8()
    for _ in range(path_count):
        flags = read_u8()
        point_count = read_u8()
        if not flags & PATH_FLAG_NO_CURVES:
            raise ValueError("unexpected path flags %d" % flags)
        for _ in range(point_count * 2):
            if read_u8() & 128:
                read_u8()

    shape_count = read_u8()
    for _ in range(shape_count):
        if read_u8() != SHAPE_TYPE_PATH_SOURCE:
            raise ValueError("unsupported shape type")
        style_index = read_u8()
        indices = read(read_u8())
        if read_u8() or style_index >= style_count or max(indices) >= path_count:
            raise ValueError("bad shape reference")

    if pos[0] != len(data):
        raise ValueError("trailing bytes")
    return True


def rdef_hex_lines(data, chunk=32):
    return ['\t$"%s"' % data[i:i + chunk].hex() for i in range(0, len(data), chunk)]


def update_rdef(rdef_path, data):
    with open(rdef_path) as f:
        text = f.read()
    block = "resource vector_icon {\n" + "\n".join(rdef_hex_lines(data)) + "\n};"
    new, count = re.subn(r"resource vector_icon \{.*?\n\};", lambda m: block, text, flags=re.S)
    if count != 1:
        raise ValueError("expected exactly one vector_icon resource in %s" % rdef_path)
    with open(rdef_path, "w") as f:
        f.write(new)


def main():
    parser = argparse.ArgumentParser(description="Convert the Tansu SVG icon to HVIF.")
    parser.add_argument("svg", nargs="?", default="resources/icon.svg")
    parser.add_argument("output", nargs="?", default="resources/icon.hvif")
    parser.add_argument("--rdef", default="resources/app.rdef",
        help="rdef file whose vector_icon resource is replaced ('' to skip)")
    args = parser.parse_args()

    data, (styles, paths, shapes) = build_icon(args.svg)
    verify(data)

    with open(args.output, "wb") as f:
        f.write(data)
    if args.rdef:
        update_rdef(args.rdef, data)

    print("wrote %s (%d bytes: %d styles, %d paths, %d shapes, verified)"
        % (args.output, len(data), styles, paths, shapes), file=sys.stderr)


if __name__ == "__main__":
    main()
