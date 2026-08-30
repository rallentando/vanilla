# Regenerates the line-art icons under resources/ (D-131).
#
# Run from anywhere:  python scripts/draw_icons.py [--out DIR]
# Needs Pillow.  --out renders somewhere else for a look before overwriting.
#
# What it draws and what it leaves alone:
#   - drawn: the chevrons (menu back/forward/rewind/fastforward, tree bar
#     up/down/left/right), the crosses, the speakers, clone, trash, plus,
#     reload, and the tool bar's command and search. Thin strokes (1px at
#     base size), corners square and circles round, near-opaque black ink
#     (alpha 230 -- Theme::Ink recolours through the alpha, so the ink is
#     the shape and the colour arrives at paint time).
#   - left alone: the tree glyphs (blank/folder/folded/unfolded and their
#     white variants -- kept at their current weight on purpose), table.png
#     (the vanilla logo itself, the one icon that keeps its fill), and
#     empty.png.
#
# Geometry is written in base-size pixel units. Everything is rendered at
# 8x supersampling and box-filtered down to the base and @2x sizes, so an
# edge that sits on an integer coordinate comes out crisp and a diagonal
# gets its antialiasing from the filter rather than from hand-placed grey.

import argparse
import os

from PIL import Image, ImageDraw

S = 8            # supersample factor
INK_ALPHA = 230  # near-opaque: thin lines carry less weight, so the ink is dark

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "resources")


def canvas(size):
    return Image.new("RGBA", (size * S, size * S), (0, 0, 0, 0))


def stroke(draw, points, width=1.0):
    # a polyline with round joints, in base-size units.
    pts = [(x * S, y * S) for x, y in points]
    draw.line(pts, fill=(0, 0, 0, 255), width=round(width * S), joint="curve")
    # PIL leaves the ends of a wide line square and un-rounded; cap them.
    r = width * S / 2.0
    for x, y in (pts[0], pts[-1]):
        draw.ellipse([x - r, y - r, x + r, y + r], fill=(0, 0, 0, 255))


def rect_outline(draw, x0, y0, x1, y1, width=1.0):
    # a crisp rectangle whose 1px edges sit on the pixel grid: the given
    # coordinates are the OUTER edge in base-size units.
    w = round(width * S)
    draw.rectangle([x0 * S, y0 * S, x1 * S - 1, y1 * S - 1],
                   outline=(0, 0, 0, 255), width=w)


def bar(draw, x0, y0, x1, y1):
    # a filled axis-aligned bar (used for crisp 1px strokes on the grid).
    draw.rectangle([x0 * S, y0 * S, x1 * S - 1, y1 * S - 1], fill=(0, 0, 0, 255))


def poly_outline(draw, points, width=1.0):
    pts = [(x * S, y * S) for x, y in points]
    draw.line(pts + [pts[0]], fill=(0, 0, 0, 255), width=round(width * S),
              joint="curve")


def chevron(draw, apex, arm_x, arm_dy, width=1.0):
    # a "<" (or rotated) drawn as two strokes meeting at the apex.
    ax, ay = apex
    stroke(draw, [(arm_x, ay - arm_dy), (ax, ay), (arm_x, ay + arm_dy)], width)


def chevron_v(draw, apex, arm_y, arm_dx, width=1.0):
    ax, ay = apex
    stroke(draw, [(ax - arm_dx, arm_y), (ax, ay), (ax + arm_dx, arm_y)], width)


# ---- the icons, one function each, drawing in base-size units ----------------

def icon_cross(size, margin):
    im = canvas(size)
    d = ImageDraw.Draw(im)
    m = margin
    stroke(d, [(m, m), (size - m, size - m)])
    stroke(d, [(size - m, m), (m, size - m)])
    return im


def icon_back(size=16):
    im = canvas(size)
    chevron(ImageDraw.Draw(im), apex=(5, 8), arm_x=11, arm_dy=5.5)
    return im


def icon_forward(size=16):
    return icon_back(size).transpose(Image.FLIP_LEFT_RIGHT)


def icon_rewind(size=16):
    im = canvas(size)
    d = ImageDraw.Draw(im)
    chevron(d, apex=(2.5, 8), arm_x=7.5, arm_dy=5.5)
    chevron(d, apex=(8.5, 8), arm_x=13.5, arm_dy=5.5)
    return im


def icon_fastforward(size=16):
    return icon_rewind(size).transpose(Image.FLIP_LEFT_RIGHT)


def icon_reload(size=16):
    import math
    im = canvas(size)
    d = ImageDraw.Draw(im)
    r = 5.0
    cx = cy = 8.0
    box = [(cx - r) * S, (cy - r) * S, (cx + r) * S, (cy + r) * S]
    # PIL angles run clockwise from 3 o'clock, and y is down, so 270 is the
    # top. The stroke runs clockwise from upper-right round to upper-left,
    # and the head sits on that terminal end, continuing the direction of
    # travel -- a head anchored on the line reads as an arrow, a head
    # floating in the gap reads as a smudge.
    d.arc(box, start=300, end=225, fill=(0, 0, 0, 255), width=round(1.0 * S))
    a = math.radians(225)
    bx, by = cx + r * math.cos(a), cy + r * math.sin(a)
    tx, ty = -math.sin(a), math.cos(a)   # tangent, clockwise
    nx, ny = math.cos(a), math.sin(a)    # normal, outward
    pts = [(bx + tx * 2.9, by + ty * 2.9),
           (bx + nx * 1.7, by + ny * 1.7),
           (bx - nx * 1.7, by - ny * 1.7)]
    d.polygon([(x * S, y * S) for x, y in pts], fill=(0, 0, 0, 255))
    return im


def icon_chevron_up(size=11):
    im = canvas(size)
    chevron_v(ImageDraw.Draw(im), apex=(5.5, 3.5), arm_y=7.5, arm_dx=3.5)
    return im


def icon_chevron_down(size=11):
    return icon_chevron_up(size).transpose(Image.FLIP_TOP_BOTTOM)


def icon_chevron_left(size=11):
    im = canvas(size)
    chevron(ImageDraw.Draw(im), apex=(3.5, 5.5), arm_x=7.5, arm_dy=3.5)
    return im


def icon_chevron_right(size=11):
    return icon_chevron_left(size).transpose(Image.FLIP_LEFT_RIGHT)


def icon_plus(size=11):
    im = canvas(size)
    d = ImageDraw.Draw(im)
    bar(d, 1, 5, 10, 6)
    bar(d, 5, 1, 6, 10)
    return im


def icon_clone(size=10):
    # two square outlines, the front one lower-left, corners square.
    im = canvas(size)
    d = ImageDraw.Draw(im)
    rect_outline(d, 3, 1, 9, 7)
    # knock the back square out where the front one will sit, so the
    # front reads as being in front rather than as a lattice.
    d.rectangle([1 * S, 3 * S, 7 * S - 1, 9 * S - 1], fill=(0, 0, 0, 0))
    rect_outline(d, 1, 3, 7, 9)
    return im


def icon_speaker(size=10, muted=False):
    # the usual orientation: driver box on the left, cone opening right.
    # audible gets one sound arc -- at ten pixels that arc is what makes
    # the outline read as a speaker rather than as a flag. The glyph sits
    # a pixel right of where it would centre; the box-heavy left side made
    # it hang left in its rectangle (D-131, user feedback).
    import math
    im = canvas(size)
    d = ImageDraw.Draw(im)
    poly_outline(d, [(2.8, 3.4), (4.8, 3.4), (7.6, 0.9), (7.6, 9.1),
                     (4.8, 6.6), (2.8, 6.6)])
    if muted:
        stroke(d, [(3.0, 9.0), (9.5, 2.5)])
    else:
        ar = 1.4
        box = [(8.0 - ar) * S, (5.0 - ar) * S, (8.0 + ar) * S, (5.0 + ar) * S]
        d.arc(box, start=-52, end=52, fill=(0, 0, 0, 255), width=round(1.0 * S))
    return im


def icon_trash(size=11):
    im = canvas(size)
    d = ImageDraw.Draw(im)
    bar(d, 4, 0, 7, 1)               # handle, a pixel clear of the lid
    bar(d, 1, 2, 10, 3)              # lid
    rect_outline(d, 2, 4, 9, 10)     # body, square corners
    return im


def icon_command(size=16):
    im = canvas(size)
    d = ImageDraw.Draw(im)
    chevron(d, apex=(7.5, 8), arm_x=3.5, arm_dy=3.5)
    bar(d, 9, 12, 14, 13)
    return im


def icon_search(size=16):
    im = canvas(size)
    d = ImageDraw.Draw(im)
    r = 4.0
    cx = cy = 6.0
    d.ellipse([(cx - r) * S, (cy - r) * S, (cx + r) * S, (cy + r) * S],
              outline=(0, 0, 0, 255), width=round(1.0 * S))
    k = 0.7071
    stroke(d, [(cx + r * k, cy + r * k), (13.0, 13.0)], width=1.2)
    return im


ICONS = {
    "menu/back.png":            icon_back,
    "menu/forward.png":         icon_forward,
    "menu/rewind.png":          icon_rewind,
    "menu/fastforward.png":     icon_fastforward,
    "menu/reload.png":          icon_reload,
    "menu/stop.png":            lambda: icon_cross(16, 3.5),
    "notifier/close.png":       lambda: icon_cross(10, 1.5),
    "tableview/close.png":      lambda: icon_cross(10, 1.5),
    "treebar/close.png":        lambda: icon_cross(10, 1.5),
    "tableview/clone.png":      icon_clone,
    "treebar/clone.png":        icon_clone,
    "tableview/audible.png":    lambda: icon_speaker(10, muted=False),
    "treebar/audible.png":      lambda: icon_speaker(10, muted=False),
    "tableview/muted.png":      lambda: icon_speaker(10, muted=True),
    "treebar/muted.png":        lambda: icon_speaker(10, muted=True),
    "tableview/trash.png":      icon_trash,
    "treebar/plus.png":         icon_plus,
    "treebar/up.png":           icon_chevron_up,
    "treebar/down.png":         icon_chevron_down,
    "treebar/left.png":         icon_chevron_left,
    "treebar/right.png":        icon_chevron_right,
    "toolbar/command.png":      icon_command,
    "toolbar/search.png":       icon_search,
}


def render(big, size):
    out = big.resize((size, size), Image.BOX)
    # apply the ink alpha uniformly: the shapes are drawn opaque so the
    # supersampling decides the edges, and the overall darkness is one knob.
    r, g, b, a = out.split()
    a = a.point(lambda v: v * INK_ALPHA // 255)
    return Image.merge("RGBA", (r, g, b, a))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=ROOT)
    args = ap.parse_args()
    for name, fn in sorted(ICONS.items()):
        big = fn()
        base = big.width // S
        path = os.path.join(args.out, name.replace("/", os.sep))
        os.makedirs(os.path.dirname(path), exist_ok=True)
        render(big, base).save(path)
        root, ext = os.path.splitext(path)
        render(big, base * 2).save(root + "@2x" + ext)
        print("%-24s %dpx (+@2x)" % (name, base))


if __name__ == "__main__":
    main()
