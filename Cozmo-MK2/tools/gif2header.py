#!/usr/bin/env python3
"""
gif2header.py — converte animações para o formato do Cozmo-MK2 (OLED 128x64).

Entradas aceitas:
  - GIF animado ...................... python gif2header.py piscada.gif --name wink
  - Pasta com PNG/JPG numerados ...... python gif2header.py frames/ --name love --fps 20
  - Header do image2cpp (Emotions.h) . python gif2header.py Emotions.h --name shush --fps 30

Saída: anim_<name>.h, pronto para colocar na pasta do sketch.
Frames repetidos são guardados uma vez só (cada frame único ocupa 1 KB de flash).

Requer: pip install pillow
"""
import argparse, re, sys
from pathlib import Path
from PIL import Image, ImageOps, ImageSequence

W, H = 128, 64
FRAME_BYTES = W * H // 8


def to_1bit(img, threshold, dither, invert, stretch):
    img = img.convert("L")
    if invert:
        img = ImageOps.invert(img)
    if stretch:
        img = img.resize((W, H), Image.LANCZOS)
    else:  # mantém proporção, centraliza em fundo preto
        img = ImageOps.contain(img, (W, H), Image.LANCZOS)
        canvas = Image.new("L", (W, H), 0)
        canvas.paste(img, ((W - img.width) // 2, (H - img.height) // 2))
        img = canvas
    if dither:
        return img.convert("1")  # Floyd–Steinberg
    return img.point(lambda p: 255 if p >= threshold else 0).convert("1")


def pack(img):
    """Formato do Adafruit drawBitmap: linha a linha, MSB = pixel da esquerda."""
    px = img.load()
    out = bytearray()
    for y in range(H):
        for xb in range(0, W, 8):
            b = 0
            for bit in range(8):
                if px[xb + bit, y]:
                    b |= 0x80 >> bit
            out.append(b)
    return bytes(out)


def load_gif(path, a):
    frames, delays = [], []
    im = Image.open(path)
    for fr in ImageSequence.Iterator(im):
        frames.append(pack(to_1bit(fr.convert("RGBA").convert("RGB"), a.threshold, a.dither, a.invert, a.stretch)))
        d = fr.info.get("duration", 0) or round(1000 / a.fps)
        delays.append(max(10, int(d)))
    return frames, delays


def load_folder(path, a):
    files = sorted(p for p in Path(path).iterdir() if p.suffix.lower() in (".png", ".jpg", ".jpeg", ".bmp"))
    if not files:
        sys.exit(f"Nenhuma imagem em {path}")
    frames = [pack(to_1bit(Image.open(f).convert("RGB"), a.threshold, a.dither, a.invert, a.stretch)) for f in files]
    return frames, [round(1000 / a.fps)] * len(frames)


def load_image2cpp_header(path, a):
    """Lê arrays 'const unsigned char nome [] PROGMEM = {...}' na ordem do *_allArray."""
    src = Path(path).read_text(errors="ignore")
    arrays = {n: bytes(int(h, 16) for h in re.findall(r"0x[0-9a-fA-F]{2}", body))
              for n, body in re.findall(r"unsigned char\s+(\w+)\s*\[\]\s*PROGMEM\s*=\s*\{(.*?)\};", src, re.S)}
    m = re.search(r"\w+allArray\s*\[\d*\]\s*=\s*\{(.*?)\};", src, re.S)
    order = re.findall(r"\w+", m.group(1)) if m else list(arrays)
    frames = [arrays[n] for n in order if n in arrays]
    bad = [len(f) for f in frames if len(f) != FRAME_BYTES]
    if bad:
        sys.exit(f"Frames com tamanho inesperado (esperado {FRAME_BYTES} bytes, ou seja 128x64)")
    return frames, [round(1000 / a.fps)] * len(frames)


def write_header(name, frames, delays, src, out_path):
    uniq, seq = [], []
    index = {}
    for f in frames:
        if f not in index:
            index[f] = len(uniq)
            uniq.append(f)
        seq.append(index[f])

    L = [f"// Gerado por tools/gif2header.py a partir de '{Path(src).name}'",
         f"// {len(frames)} passos, {len(uniq)} frames únicos ({len(uniq)} KB de flash)",
         "#pragma once", '#include "BitmapAnim.h"', "",
         f"const uint8_t {name}_frames[{len(uniq)}][{FRAME_BYTES}] PROGMEM = {{"]
    for i, f in enumerate(uniq):
        L.append(f"  {{ // frame {i}")
        for j in range(0, FRAME_BYTES, 16):
            L.append("    " + ", ".join(f"0x{b:02x}" for b in f[j:j + 16]) + ",")
        L.append("  },")
    L.append("};")
    L.append(f"const uint16_t {name}_seq[{len(seq)}] PROGMEM = {{ {', '.join(map(str, seq))} }};")
    L.append(f"const uint16_t {name}_ms[{len(delays)}] PROGMEM = {{ {', '.join(map(str, delays))} }};")
    L.append(f'const BitmapAnim {name.upper()}_ANIM = {{ "{name}", {name}_frames, {name}_seq, {name}_ms, {len(seq)} }};')
    Path(out_path).write_text("\n".join(L) + "\n")
    return len(frames), len(uniq)


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("input", help="GIF, pasta de imagens ou header .h do image2cpp")
    p.add_argument("--name", required=True, help="identificador C (ex.: love, wink)")
    p.add_argument("--fps", type=float, default=30, help="velocidade quando a entrada não tem tempos (padrão 30)")
    p.add_argument("--threshold", type=int, default=128, help="limiar preto/branco 0-255 (padrão 128)")
    p.add_argument("--dither", action="store_true", help="usar pontilhado em vez de limiar (bom para fotos)")
    p.add_argument("--invert", action="store_true", help="inverter cores (desenho preto em fundo branco)")
    p.add_argument("--stretch", action="store_true", help="esticar para 128x64 em vez de manter a proporção")
    p.add_argument("-o", "--out", help="arquivo de saída (padrão anim_<name>.h)")
    a = p.parse_args()

    if not re.fullmatch(r"[A-Za-z_]\w*", a.name):
        sys.exit("--name precisa ser um identificador C válido (letras, números e _)")

    src = Path(a.input)
    if src.is_dir():
        frames, delays = load_folder(src, a)
    elif src.suffix.lower() == ".h":
        frames, delays = load_image2cpp_header(src, a)
    else:
        frames, delays = load_gif(src, a)

    out = a.out or f"anim_{a.name}.h"
    n, u = write_header(a.name, frames, delays, src, out)
    print(f"{out}: {n} passos, {u} frames únicos (economia de {n - u} KB)")


if __name__ == "__main__":
    main()
