"""Generate the OpenPuck logo SVGs in docs/brand/.

Usage: python tools/gen_logo.py docs/brand fonts/AtkinsonHyperlegibleNext-Bold.ttf
(needs fontTools; PNGs are rendered from the SVGs, e.g. with rsvg-convert)

Mark geometry is defined on the device's own 240x240 grid so the SVG and the
LVGL implementation share the same numbers.
"""
import math, os, sys
from fontTools.ttLib import TTFont
from fontTools.pens.svgPathPen import SVGPathPen
from fontTools.pens.transformPen import TransformPen
from fontTools.pens.boundsPen import BoundsPen

OUT = sys.argv[1]
TTF = sys.argv[2]
os.makedirs(OUT, exist_ok=True)

INK = "#0E1013"
BONE = "#F5F2EA"
EMBER = "#FF5A36"
INK_LIGHT = "#16181C"  # wordmark/ring on light backgrounds

# --- Mark geometry (240 grid) ---
C = 120          # center
R = 80           # ring center-line radius
W = 32           # ring stroke width (round caps => cap radius 16)
GAP_AT = -45     # gap center, degrees, 0 = 3 o'clock, clockwise (screen coords)
HALF_GAP = 32    # degrees from gap center to each cap center
BEAD_R = 16      # bead radius == cap radius


def pt(a, r=R, c=C):
    t = math.radians(a)
    return c + r * math.cos(t), c + r * math.sin(t)


def ring_path(c=C, r=R):
    a0 = GAP_AT + HALF_GAP          # -13
    a1 = GAP_AT - HALF_GAP + 360    # 283
    x0, y0 = pt(a0, r, c)
    x1, y1 = pt(a1, r, c)
    return f"M{x0:.2f} {y0:.2f} A{r} {r} 0 1 1 {x1:.2f} {y1:.2f}"


def mark_group(ring=BONE, bead=EMBER, tx=0, ty=0, s=1.0):
    bx, by = pt(GAP_AT)
    return (f'<g transform="translate({tx:.2f} {ty:.2f}) scale({s:.5f})">'
            f'<path d="{ring_path()}" fill="none" stroke="{ring}" stroke-width="{W}" stroke-linecap="round"/>'
            f'<circle cx="{bx:.2f}" cy="{by:.2f}" r="{BEAD_R}" fill="{bead}"/></g>')


def svg(w, h, body, bg=None, title="OpenPuck"):
    bgr = f'<rect width="{w}" height="{h}" fill="{bg}"/>' if bg else ""
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {w} {h}" width="{w}" height="{h}" '
            f'role="img" aria-label="{title}"><title>{title}</title>{bgr}{body}</svg>\n')


def write(name, s):
    with open(os.path.join(OUT, name), "w") as f:
        f.write(s)


# --- Wordmark from Atkinson Hyperlegible Next Bold ---
font = TTFont(TTF)
gs = font.getGlyphSet()
cmap = font.getBestCmap()
upm = font["head"].unitsPerEm
cap = font["OS/2"].sCapHeight
hmtx = font["hmtx"]

# simple pair kerning from GPOS is skipped; tracking applied instead
def text_paths(text, size, x, baseline, fill, tracking=0.0, skip_first=False):
    s = size / upm
    out, cx = [], x
    for i, ch in enumerate(text):
        g = cmap[ord(ch)]
        adv = hmtx[g][0]
        if not (skip_first and i == 0):
            pen = SVGPathPen(gs)
            gs[g].draw(TransformPen(pen, (s, 0, 0, -s, cx, baseline)))
            out.append(f'<path d="{pen.getCommands()}" fill="{fill}"/>')
        cx += adv * s + tracking * size
    return "".join(out), cx - x


def glyph_bounds(ch):
    bp = BoundsPen(gs)
    gs[cmap[ord(ch)]].draw(bp)
    return bp.bounds, hmtx[cmap[ord(ch)]][0]


# ---------- Exports ----------
# 1. Mark, transparent (works on dark); mark on light uses ink ring
write("openpuck-mark.svg", svg(240, 240, mark_group()))
write("openpuck-mark-light.svg", svg(240, 240, mark_group(ring=INK_LIGHT)))
write("openpuck-mark-mono.svg", svg(240, 240, mark_group(ring="currentColor", bead="currentColor")))
# App/avatar tile: mark on ink square (GitHub avatar, favicon)
write("openpuck-avatar.svg", svg(240, 240, mark_group(), bg=INK))
# Round device splash reference (240 round screen)
write("openpuck-device.svg", svg(240, 240, f'<circle cx="120" cy="120" r="120" fill="#000"/>' + mark_group()))

# 2. Lockup: mark replaces the "O" of OpenPuck
def lockup(fg, ring, bead, bg=None, pad=None):
    size = 200.0                     # font size in px
    s = size / upm
    (ox0, oy0, ox1, oy1), oadv = glyph_bounds("O")
    o_h = (oy1 - oy0) * s            # O height incl overshoot
    # ring outer diameter in mark units = 2*(R+W/2) = 192; scale so it matches O height * 1.04
    ms = (o_h * 1.04) / (2 * (R + W / 2))
    mark_d = 2 * (R + W / 2) * ms
    baseline = 200.0
    o_left = ox0 * s
    # place mark so its outer circle sits where the O's bounds are
    o_cx = o_left + (ox1 - ox0) * s / 2
    o_cy = baseline - (oy0 + oy1) / 2 * s
    word, adv = text_paths("OpenPuck", size, 0, baseline, fg, tracking=-0.01, skip_first=True)
    mark = mark_group(ring=ring, bead=bead, tx=o_cx - C * ms, ty=o_cy - C * ms, s=ms)
    # bounds
    top = o_cy - mark_d / 2
    bottom = baseline + 4
    p = pad if pad is not None else 0
    w = adv + 2 * p
    h = (bottom - top) + 2 * p
    body = f'<g transform="translate({p:.2f} {p - top:.2f})">{mark}{word}</g>'
    return w, h, body

for name, fg, ring, bg in [("openpuck-lockup", BONE, BONE, None),
                           ("openpuck-lockup-light", INK_LIGHT, INK_LIGHT, None)]:
    w, h, body = lockup(fg, ring, EMBER)
    write(f"{name}.svg", svg(round(w, 1), round(h, 1), body))

# 3. Social preview 1280x640
w, h, body = lockup(BONE, BONE, EMBER)
sc = 760 / w
tx = (1280 - w * sc) / 2
ty = 270 - h * sc / 2
tag, tw = text_paths("Open-source firmware for round-screen life counters", 34, 0, 0, "#9AA0A8")
social = (f'<g transform="translate({tx:.1f} {ty:.1f}) scale({sc:.4f})">{body}</g>'
          f'<g transform="translate({(1280 - tw) / 2:.1f} {ty + h * sc + 90:.1f})">{tag}</g>')
write("openpuck-social.svg", svg(1280, 640, social, bg=INK))
print("ok", OUT)
