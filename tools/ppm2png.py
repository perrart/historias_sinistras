#!/usr/bin/env python3
"""Converte as capturas .ppm do simulador em .png (tamanho real e ampliado 3x)."""
import glob
import os
import sys
from PIL import Image

d = sys.argv[1] if len(sys.argv) > 1 else 'sim/out'
for p in sorted(glob.glob(os.path.join(d, '*.ppm'))):
    im = Image.open(p)
    base = p[:-4]
    im.save(base + '.png')
    im.resize((im.width * 3, im.height * 3), Image.NEAREST).save(base + '@3x.png')
    os.remove(p)
print('ok')
