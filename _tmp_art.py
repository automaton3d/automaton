from PIL import Image
import numpy as np

im = Image.open(r'e:/automaton/doc/fig2.png').convert('L')
a = np.asarray(im)

colx = {0: (64, 128), 1: (128, 192), 2: (192, 256), 3: (256, 320),
        4: (320, 384), 5: (384, 448), 6: (448, 512), 7: (512, 576)}
rows = [('000', 55, 85), ('001', 90, 120), ('010', 127, 157),
        ('011', 164, 194), ('100', 200, 230), ('101', 237, 267),
        ('110', 278, 308), ('111', 317, 347)]
rowcode = {r[0]: (r[1], r[2]) for r in rows}
THR = 170

def art(name, x0, x1, y0, y1):
    cell = a[y0 + 3:y1 - 3, x0 + 3:x1 - 3]
    bin = (cell < THR).astype(int)
    ys, xs = np.where(bin)
    if len(xs) == 0:
        print(name, ': EMPTY'); return
    xa, xb = xs.min(), xs.max()
    ya, yb = ys.min(), ys.max()
    sub = bin[ya:yb + 1, xa:xb + 1]
    print('---', name, '(bbox %dx%d)' % (sub.shape[1], sub.shape[0]))
    for yy in range(sub.shape[0]):
        print(''.join('#' if sub[yy, xx] else '.' for xx in range(sub.shape[1])))

codes = ['001', '010', '011', '100', '101', '110']
for ci in range(1, 7):
    x0, x1 = colx[ci]
    y0, y1 = rowcode['000']
    art('N_row x %s' % codes[ci - 1], x0, x1, y0, y1)
for ri in range(1, 7):
    x0, x1 = colx[0]
    y0, y1 = rowcode[codes[ri - 1]]
    art('%s x N_col' % codes[ri - 1], x0, x1, y0, y1)
