from PIL import Image

im = Image.open(r'e:/automaton/doc/fig2.png').convert('RGB')
big = im.resize((im.width * 4, im.height * 4), Image.LANCZOS)
big.save(r'e:/automaton/_fig2_big.png')
print('saved', big.size)
