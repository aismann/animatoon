# Converts the .rgba files of test_render into PNGs plus one contact sheet.
import sys, glob, os
from PIL import Image
out = sys.argv[1]
tiles = []
for f in sorted(glob.glob(os.path.join(out, "*.rgba")), key=lambda p: (os.path.basename(p).startswith("example"), int("".join(c for c in os.path.basename(p) if c.isdigit())))):
    data = open(f, "rb").read()
    side = int((len(data) // 4) ** 0.5)
    im = Image.frombytes("RGBA", (side, side), data)
    im.save(f[:-5] + ".png")
    tiles.append(im.resize((290, 290)))
cols = 4
rows = (len(tiles) + cols - 1) // cols
sheet = Image.new("RGBA", (cols * 290, rows * 290), (200, 200, 200, 255))
for i, t in enumerate(tiles):
    sheet.paste(t, ((i % cols) * 290, (i // cols) * 290))
sheet.save(os.path.join(out, "sheet.png"))
