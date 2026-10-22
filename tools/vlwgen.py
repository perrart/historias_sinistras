#!/usr/bin/env python3
"""Generate Processing/TFT_eSPI-style .vlw smooth fonts with FreeType.

The output is consumed by M5GFX's VLWfont loader (big-endian header,
28-byte glyph records, 8-bit alpha bitmaps).  Also offers a preview
renderer that mimics the on-device result so layouts can be checked.
"""
import struct
import sys
import freetype

# Printable ASCII + Latin-1 supplement + a few typographic extras
DEFAULT_CHARS = (
    [c for c in range(0x20, 0x7F)]
    + [c for c in range(0xA0, 0x100)]
    + [0x2014, 0x2013, 0x2026, 0x201C, 0x201D, 0x2018, 0x2019, 0x2022, 0x25B6, 0x25BA]
)


def load_face(path, px, weight=None, load_flags=None):
    face = freetype.Face(path)
    if weight is not None:
        try:
            face.set_var_named_instance  # noqa
            axes = face.get_variation_info().axes
            coords = []
            for a in axes:
                coords.append(weight if a.tag == 'wght' else a.default)
            face.set_var_design_coords(coords)
        except Exception as exc:  # pragma: no cover
            print('weight not applied:', exc, file=sys.stderr)
    face.set_pixel_sizes(0, px)
    return face


def render_glyph(face, cp, flags):
    idx = face.get_char_index(cp)
    if idx == 0 and cp != 0x20:
        return None
    face.load_char(cp, flags)
    g = face.glyph
    bm = g.bitmap
    w, h = bm.width, bm.rows
    data = bytes(bm.buffer) if w and h else b''
    if w and h and bm.pitch != w:
        rows = []
        for r in range(h):
            rows.append(bytes(bm.buffer[r * bm.pitch:r * bm.pitch + w]))
        data = b''.join(rows)
    adv = (g.advance.x + 32) >> 6
    return dict(cp=cp, w=w, h=h, adv=adv, top=g.bitmap_top, left=g.bitmap_left, data=data)


def make_vlw(path, px, out, chars=DEFAULT_CHARS, weight=None, hint='light', tracking=0, gamma=1.0):
    flags = freetype.FT_LOAD_RENDER
    if hint == 'light':
        flags |= freetype.FT_LOAD_TARGET_LIGHT
    elif hint == 'none':
        flags |= freetype.FT_LOAD_NO_HINTING
    face = load_face(path, px, weight)
    glyphs = []
    for cp in sorted(set(chars)):
        g = render_glyph(face, cp, flags)
        if g is None:
            continue
        g['adv'] += tracking
        if gamma != 1.0 and g['data']:
            g['data'] = bytes(min(255, int(255 * ((b / 255.0) ** gamma) + 0.5)) for b in g['data'])
        glyphs.append(g)
    asc = (face.size.ascender + 32) >> 6
    desc = abs((face.size.descender - 32) >> 6)
    # VLW uses the "d" top and "p" bottom; FreeType metrics are close enough,
    # M5GFX recomputes max ascent/descent from glyphs anyway.
    hdr = struct.pack('>6i', len(glyphs), 11, px, 0, asc, desc)
    recs = b''
    bmps = b''
    for g in glyphs:
        recs += struct.pack('>7i', g['cp'], g['h'], g['w'], g['adv'], g['top'], g['left'], 0)
        bmps += g['data']
    blob = hdr + recs + bmps
    with open(out, 'wb') as f:
        f.write(blob)
    missing = [hex(c) for c in chars if face.get_char_index(c) == 0 and c != 0x20]
    return len(blob), len(glyphs), missing


def parse_vlw(blob):
    n, _, size, _, asc, desc = struct.unpack('>6i', blob[:24])
    recs = []
    off = 24
    for i in range(n):
        recs.append(struct.unpack('>7i', blob[off:off + 28]))
        off += 28
    glyphs = {}
    p = off
    max_asc, max_desc = asc, desc
    for cp, h, w, adv, top, left, _ in recs:
        glyphs[cp] = dict(w=w, h=h, adv=adv, top=top, left=left, data=blob[p:p + w * h])
        p += w * h
        if cp > 0xFF or (0x20 < cp < 0xA0 and cp != 0x7F):
            max_asc = max(max_asc, top)
            max_desc = max(max_desc, h - top)
    return dict(glyphs=glyphs, size=size, ascent=max_asc, descent=max_desc, height=max_asc + max_desc)


if __name__ == '__main__':
    src, px, out = sys.argv[1], int(sys.argv[2]), sys.argv[3]
    weight = float(sys.argv[4]) if len(sys.argv) > 4 else None
    print(make_vlw(src, px, out, weight=weight))
