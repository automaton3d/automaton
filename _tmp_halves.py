from PIL import Image
import os

im = Image.open(r'e:/automaton/doc/fig2.png').convert('RGB')
outdir = r'e:/automaton/_ocr_halves'
os.makedirs(outdir, exist_ok=True)

# Grid (original px): col0=labels, col1..8 = 000..111
cols = [(64, 128), (128, 192), (192, 256), (256, 320), (320, 384),
        (384, 448), (448, 512), (512, 576)]
colcode = {0: '000', 1: '001', 2: '010', 3: '011', 4: '100',
           5: '101', 6: '110', 7: '111'}
rows = [
    ('000', 43, 82),
    ('001', 84, 119),
    ('010', 121, 156),   # upper half of band 4
    ('011', 156, 192),   # lower half of band 4
    ('100', 194, 229),
    ('101', 231, 266),
    ('110', 268, 307),
    ('111', 309, 345),
]

def crop_halves(name, x0, x1, y0, y1):
    c = im.crop((x0 + 5, y0 + 2, x1 - 5, y1 - 2))
    w, h = c.size
    top = c.crop((0, 0, w, h // 2)).resize((w * 8, (h // 2) * 8), Image.LANCZOS)
    bot = c.crop((0, h // 2, w, h)).resize((w * 8, (h - h // 2) * 8), Image.LANCZOS)
    top.save(os.path.join(outdir, name + '_t.png'))
    bot.save(os.path.join(outdir, name + '_b.png'))

# N row (000): cols 001..110
for ci in range(6):
    x0, x1 = cols[ci + 1]
    crop_halves('r000_c%s' % colcode[ci + 1], x0, x1, rows[0][1], rows[0][2])
# Nbar row (111): cols 001..110
for ci in range(6):
    x0, x1 = cols[ci + 1]
    crop_halves('r111_c%s' % colcode[ci + 1], x0, x1, rows[7][1], rows[7][2])
# col 000 (ci=0), rows 001..110
for ri in range(1, 7):
    x0, x1 = cols[0]
    crop_halves('r%s_c000' % rows[ri][0], x0, x1, rows[ri][1], rows[ri][2])
# col 111 (ci=7), rows 001..110
for ri in range(1, 7):
    x0, x1 = cols[7]
    crop_halves('r%s_c111' % rows[ri][0], x0, x1, rows[ri][1], rows[ri][2])
print('saved 48 halves (24 cells x top/bottom)')
