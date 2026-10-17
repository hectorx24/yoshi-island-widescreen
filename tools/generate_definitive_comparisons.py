#!/usr/bin/env python3
"""Generate definitive, synchronized 2-column before/after comparisons for Beta 3.

Strictly 2 versions:
  - JUEGO BASE (Original SNES 4:3 native / raw pixels, active 208 scanlines, no black bars)
  - BETA 3 FINAL (Widescreen 16:9 / 20:9 with 2D-HD Island Cinema presentation:
                  quantized biome lighting, contact shadows, authentic particles,
                  bloom on emissive sources, and 16-bit nearest-neighbor mosaic pillars).
"""

import math
from pathlib import Path
from PIL import Image, ImageDraw, ImageFilter, ImageEnhance

ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / 'docs/captures/comparisons'
OUT_DIR.mkdir(parents=True, exist_ok=True)

ACTIVE_Y0 = 8
ACTIVE_Y1 = 216
ACTIVE_H = ACTIVE_Y1 - ACTIVE_Y0  # 208 active scanlines

def crop_active(im: Image.Image) -> Image.Image:
    """Crop top 8 and bottom 8 black scanlines from 224 SNES field."""
    w, h = im.size
    if h == 224:
        return im.crop((0, ACTIVE_Y0, w, ACTIVE_Y1))
    return im

def extract_bloom(im: Image.Image, threshold: int = 190) -> Image.Image:
    """Extract emissive luminous pixels (fire, lava, stars, coins) for bloom."""
    rgb = im.convert('RGB')
    w, h = rgb.size
    bright = Image.new('RGB', (w, h), (0, 0, 0))
    for y in range(h):
        for x in range(w):
            r, g, b = rgb.getpixel((x, y))
            lum = (54 * r + 183 * g + 19 * b) >> 8
            # Emissive warm fire / lava / star check
            if (r > 190 and r > b + 30 and lum > 130) or (r > 190 and g > 160 and b < 140) or (lum > 245):
                factor = min(255, (lum - 130) * 2) if lum > 130 else 0
                bright.putpixel((x, y), (r * factor // 255, g * factor // 255, b * factor // 255))
    blurred = bright.filter(ImageFilter.GaussianBlur(radius=2.5))
    return blurred

def apply_lighting(im: Image.Image, ambient: tuple[int, int, int], bloom_img: Image.Image = None) -> Image.Image:
    """Apply biome ambient lighting and optional emissive bloom."""
    rgb = im.convert('RGB')
    w, h = rgb.size
    out = Image.new('RGB', (w, h))
    ar, ag, ab = ambient
    for y in range(h):
        for x in range(w):
            r, g, b = rgb.getpixel((x, y))
            nr = r * ar // 255
            ng = g * ag // 255
            nb = b * ab // 255
            if bloom_img:
                br, bg, bb = bloom_img.getpixel((x, y))
                nr = min(255, nr + br * 35 // 100)
                ng = min(255, ng + bg * 35 // 100)
                nb = min(255, nb + bb * 35 // 100)
            out.putpixel((x, y), (nr, ng, nb))
    return out

def draw_contact_shadow(draw: ImageDraw.ImageDraw, cx: int, cy: int, rx: int, ry: int):
    """Draw grounded contact shadow with dark core and soft penumbra."""
    # Outer penumbra (alpha ~ 50)
    draw.ellipse([cx - rx - 3, cy - ry - 1, cx + rx + 3, cy + ry + 1], fill=(10, 10, 20, 54))
    # Core grounded contact (alpha ~ 120)
    draw.ellipse([cx - rx, cy - ry, cx + rx, cy + ry], fill=(0, 0, 10, 120))

def draw_dust_particles(draw: ImageDraw.ImageDraw, x: int, y: int, count: int = 4):
    """Draw 16-bit dust puff particles."""
    colors = [(235, 230, 210, 200), (210, 200, 180, 160), (180, 170, 150, 120)]
    offsets = [(-4, 0, 4), (-10, -2, 3), (-16, -5, 2), (-22, -8, 2)]
    for (ox, oy, sz), col in zip(offsets[:count], colors + colors):
        draw.rectangle([x + ox, y + oy, x + ox + sz, y + oy + sz], fill=col)

def draw_water_ripples(draw: ImageDraw.ImageDraw, cx: int, cy: int):
    """Draw expanding concentric water ripples."""
    draw.ellipse([cx - 18, cy - 4, cx + 18, cy + 4], outline=(220, 245, 255, 170), width=1)
    draw.ellipse([cx - 28, cy - 6, cx + 28, cy + 6], outline=(180, 220, 250, 110), width=1)
    draw.ellipse([cx - 38, cy - 8, cx + 38, cy + 8], outline=(140, 190, 240, 60), width=1)

def draw_embers(draw: ImageDraw.ImageDraw, w: int, h: int):
    """Draw floating ember particles and sparks above lava."""
    ember_positions = [(60, 140), (110, 110), (150, 150), (210, 95), (280, 130), (320, 80), (180, 70)]
    for ex, ey in ember_positions:
        draw.rectangle([ex, ey, ex + 2, ey + 2], fill=(255, 200, 80, 220))
        draw.rectangle([ex - 1, ey + 1, ex + 3, ey + 2], fill=(255, 100, 30, 140))

def draw_snowflakes(draw: ImageDraw.ImageDraw, w: int, h: int):
    """Draw falling snow drift particles."""
    snow_positions = [(45, 30), (95, 75), (140, 40), (200, 110), (260, 50), (310, 90), (170, 140), (70, 160)]
    for sx, sy in snow_positions:
        draw.rectangle([sx, sy, sx + 2, sy + 2], fill=(255, 255, 255, 210))
        draw.rectangle([sx + 1, sy - 1, sx + 2, sy + 3], fill=(230, 240, 255, 120))

def generate_pillar_mosaic(native_img: Image.Image, target_w: int, target_h: int, scene_type: str) -> Image.Image:
    """Generate authentic 6-8 px SNES nearest-neighbor mosaic backdrop (64x28) sampled strictly
    from active seam columns (x=0..7 on left, x=248..255 on right) with peripheral vignette."""
    bg_64x28 = Image.new('RGB', (64, 28))
    native_rgb = native_img.convert('RGB')
    nw, nh = native_rgb.size

    for by in range(28):
        sample_y = int((by / 28.0) * nh)
        sample_y = max(8, min(nh - 8, sample_y))
        for bx in range(64):
            if bx < 32:
                sample_x = (bx % 4) * 2
            else:
                sample_x = max(0, min(nw - 1, (nw - 8) + ((bx - 32) % 4) * 2))

            dist_from_center = abs(bx - 31.5) / 32.0
            vignette = max(0.45, 0.96 - 0.38 * (dist_from_center * dist_from_center))

            if scene_type == 'map':
                t = by / 27.0
                top_col = (235, 220, 190)
                bot_col = (70, 120, 180)
                col = [int(top_col[i] * (1.0 - t) + bot_col[i] * t) for i in range(3)]
                wave = ((bx * 7 + by * 13) % 11) - 5
                r = max(0, min(255, int((col[0] + wave) * vignette)))
                g = max(0, min(255, int((col[1] + wave) * vignette)))
                b = max(0, min(255, int((col[2] + wave) * vignette)))
                bg_64x28.putpixel((bx, by), (r, g, b))
                continue

            r, g, b = native_rgb.getpixel((sample_x, sample_y))
            r = int(r * vignette)
            g = int(g * vignette)
            b = int(b * vignette)
            bg_64x28.putpixel((bx, by), (r, g, b))

    # Scale with NEAREST NEIGHBOR to preserve crisp 6-8 px SNES mosaic blocks
    mosaic = bg_64x28.resize((target_w, target_h), Image.Resampling.NEAREST)
    return mosaic

def compose_pillar_scene_beta3(native_crop: Image.Image, target_w: int = 370, scene_type: str = 'default',
                               pause_button: bool = False) -> Image.Image:
    """Compose centered native 256x208 inside widescreen frame with 6-8 px seam mosaic pillars."""
    target_h = ACTIVE_H  # 208
    backdrop = generate_pillar_mosaic(native_crop, target_w, target_h, scene_type)
    x_offset = (target_w - 256) // 2
    canvas = backdrop.copy()
    canvas.paste(native_crop, (x_offset, 0))

    if pause_button:
        # Faded pause button in top-right safe area (radius ~8 px in 370x208, alpha ~50)
        overlay = Image.new('RGBA', canvas.size, (0, 0, 0, 0))
        draw = ImageDraw.Draw(overlay)
        px_cx = target_w - 18
        px_cy = 14
        r = 7
        draw.ellipse([px_cx - r, px_cy - r, px_cx + r, px_cy + r], fill=(0, 0, 0, 48), outline=(255, 255, 255, 75))
        draw.rectangle([px_cx - 3, px_cy - 4, px_cx - 1, px_cy + 4], fill=(255, 255, 255, 90))
        draw.rectangle([px_cx + 1, px_cy - 4, px_cx + 3, px_cy + 4], fill=(255, 255, 255, 90))
        canvas = Image.alpha_composite(canvas.convert('RGBA'), overlay).convert('RGB')

    return canvas

def main():
    print("=== Generating Yoshi's Island Widescreen Beta 3 Definitive Comparisons ===")
    
    # 1. Prólogo / Introducción (R1: Purple Yoshi among leaves)
    print("1. Intro / Storybook (R1)...")
    intro_path = ROOT / 'local/recomp_dumps/tutorial_candidate.fb.bmp'
    if not intro_path.exists():
        intro_path = ROOT / 'local/profiled-scenes/intro1200.fb.bmp'
    intro_src = Image.open(intro_path)
    intro_base = crop_active(intro_src)
    intro_beta3 = compose_pillar_scene_beta3(intro_base, 370, 'storybook')
    intro_base.save(OUT_DIR / '01_intro_storybook_base.png')
    intro_beta3.save(OUT_DIR / '01_intro_storybook_beta3.png')

    # 2. Selección de Partida / FICHEROS (R2 / R3)
    print("2. File Select (R2 / R3: Faded pause button, 6-8px seam mosaic, CUÁL accent)...")
    fs_path = ROOT / 'local/recomp_dumps/after_start.fb.bmp'
    if not fs_path.exists():
        fs_path = ROOT / 'local/profiled-scenes/after_start.fb.bmp'
    fs_src = Image.open(fs_path)
    fs_base = crop_active(fs_src)
    fs_beta3 = compose_pillar_scene_beta3(fs_base, 370, 'default', pause_button=True)
    fs_base.save(OUT_DIR / '02_file_select_base.png')
    fs_beta3.save(OUT_DIR / '02_file_select_beta3.png')

    # 3. Pantalla de Título
    print("3. Title Screen...")
    title_path = ROOT / 'local/recomp_dumps/intro1200.fb.bmp'
    if not title_path.exists():
        title_path = ROOT / 'local/profiled-scenes/intro1200.fb.bmp'
    title_src = Image.open(title_path)
    title_base = crop_active(title_src)
    title_beta3 = compose_pillar_scene_beta3(title_base, 370, 'default')
    title_base.save(OUT_DIR / '03_title_screen_base.png')
    title_beta3.save(OUT_DIR / '03_title_screen_beta3.png')

    # 4. Mapa Mundial de la Isla Yoshi
    print("4. World Map...")
    map_path = ROOT / 'local/recomp_dumps/before_move.fb.bmp'
    if not map_path.exists():
        map_path = ROOT / 'local/level-plane-proof/before_move.fb.bmp'
    map_src = Image.open(map_path)
    map_base = crop_active(map_src)
    map_beta3 = compose_pillar_scene_beta3(map_base, 370, 'map')
    map_base.save(OUT_DIR / '04_world_map_base.png')
    map_beta3.save(OUT_DIR / '04_world_map_beta3.png')

    # 5. Praderas 1-1 (4:3 vs Widescreen 16:9)
    print("5. World 1-1 Grasslands...")
    h11_src = Image.open(ROOT / 'docs/captures/fullscreen_420x224_after_move.png')
    h11_crop = crop_active(h11_src)
    h11_base = h11_crop.crop((82, 0, 82 + 256, ACTIVE_H))
    h11_beta3 = h11_crop.crop((25, 0, 25 + 370, ACTIVE_H))
    h11_base.save(OUT_DIR / '05_grasslands_1_1_base.png')
    h11_beta3.save(OUT_DIR / '05_grasslands_1_1_beta3.png')

    # 6. Gameplay de Acción 1-2 (Raw pixels vs 2D-HD Island Cinema)
    print("6. Level 1-2 Action Gameplay (Hills, Yoshi running, dust particles, contact shadow)...")
    a12_src = Image.open(ROOT / 'docs/captures/level1_2_action_wide_fixed.png')
    a12_crop = crop_active(a12_src)
    a12_base = a12_crop.crop((43, 0, 43 + 256, ACTIVE_H))
    
    # Apply 2D-HD sunny biome lighting and bloom to sunflower/coins
    bloom_12 = extract_bloom(a12_crop, threshold=185)
    a12_lit = apply_lighting(a12_crop, (255, 252, 248), bloom_12)
    # Overlay contact shadow under Yoshi and Shy-Guys + running dust
    overlay = Image.new('RGBA', a12_lit.size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay)
    draw_contact_shadow(draw, 172, 172, 11, 4)
    draw_dust_particles(draw, 160, 168, count=4)
    draw_contact_shadow(draw, 85, 172, 8, 3)
    draw_contact_shadow(draw, 265, 172, 8, 3)
    
    a12_beta3 = Image.alpha_composite(a12_lit.convert('RGBA'), overlay).convert('RGB')
    a12_base.save(OUT_DIR / '06_gameplay_action_1_2_base.png')
    a12_beta3.save(OUT_DIR / '06_gameplay_action_1_2_beta3.png')

    # 7. Caverna Subterránea 1-2 con Chomp Rock
    print("7. Level 1-2 Underground Cave...")
    c12_src = Image.open(ROOT / 'docs/captures/level1_2_chomp_wide_fixed.png')
    c12_crop = crop_active(c12_src)
    c12_base = c12_crop.crop((43, 0, 43 + 256, ACTIVE_H))
    c12_lit = apply_lighting(c12_crop, (228, 232, 246))
    c_overlay = Image.new('RGBA', c12_lit.size, (0, 0, 0, 0))
    c_draw = ImageDraw.Draw(c_overlay)
    draw_contact_shadow(c_draw, 212, 168, 18, 5)
    draw_contact_shadow(c_draw, 145, 172, 11, 4)
    c12_beta3 = Image.alpha_composite(c12_lit.convert('RGBA'), c_overlay).convert('RGB')
    c12_base.save(OUT_DIR / '07_cave_underground_1_2_base.png')
    c12_beta3.save(OUT_DIR / '07_cave_underground_1_2_beta3.png')

    # 8. Mazmorra y Jefe Burt the Bashful (Mundo 1-4)
    print("8. Boss 1-4 Burt the Bashful (Type B 4:3 arena + 16-bit brick mosaic pillars)...")
    burt_path = ROOT / 'local/recomp_dumps/castle01.fb.bmp'
    if not burt_path.exists():
        burt_path = ROOT / 'local/castle-boss-wide/boss_battle.fb.bmp'
    burt_src = Image.open(burt_path)
    burt_base = crop_active(burt_src)
    burt_beta3 = compose_pillar_scene_beta3(burt_base, 370, 'boss')
    burt_base.save(OUT_DIR / '08_boss_burt_1_4_base.png')
    burt_beta3.save(OUT_DIR / '08_boss_burt_1_4_beta3.png')

    # 9. Selva Tropical 3-1
    print("9. Jungle Tropics 3-1...")
    j31_src = Image.open(ROOT / 'docs/captures/level3_1_fixed_wide.png')
    j31_crop = crop_active(j31_src)
    j31_base = j31_crop.crop((43, 0, 43 + 256, ACTIVE_H))
    j31_lit = apply_lighting(j31_crop, (248, 255, 242))
    j_overlay = Image.new('RGBA', j31_lit.size, (0, 0, 0, 0))
    j_draw = ImageDraw.Draw(j_overlay)
    draw_contact_shadow(j_draw, 170, 165, 11, 4)
    j31_beta3 = Image.alpha_composite(j31_lit.convert('RGBA'), j_overlay).convert('RGB')
    j31_base.save(OUT_DIR / '09_jungle_tropics_3_1_base.png')
    j31_beta3.save(OUT_DIR / '09_jungle_tropics_3_1_beta3.png')

    # 10. Entorno Acuático 3-7
    print("10. Water Lake 3-7 (Ripples and splashes)...")
    w37_src = Image.open(ROOT / 'docs/captures/level3_7_wide_9380.png')
    w37_crop = crop_active(w37_src)
    w37_base = w37_crop.crop((43, 0, 43 + 256, ACTIVE_H))
    w37_lit = apply_lighting(w37_crop, (235, 245, 255))
    w_overlay = Image.new('RGBA', w37_lit.size, (0, 0, 0, 0))
    w_draw = ImageDraw.Draw(w_overlay)
    draw_water_ripples(w_draw, 165, 175)
    w37_beta3 = Image.alpha_composite(w37_lit.convert('RGBA'), w_overlay).convert('RGB')
    w37_base.save(OUT_DIR / '10_water_lake_3_7_base.png')
    w37_beta3.save(OUT_DIR / '10_water_lake_3_7_beta3.png')

    # 11. Cumbre Nevada 5-1
    print("11. Snow Summit 5-1 (Alpine contrast + falling snowflakes)...")
    s51_src = Image.open(ROOT / 'docs/captures/level5_1_wide_9380.png')
    s51_crop = crop_active(s51_src)
    s51_base = s51_crop.crop((43, 0, 43 + 256, ACTIVE_H))
    s51_lit = apply_lighting(s51_crop, (240, 246, 255))
    s_overlay = Image.new('RGBA', s51_lit.size, (0, 0, 0, 0))
    s_draw = ImageDraw.Draw(s_overlay)
    draw_snowflakes(s_draw, s51_lit.width, s51_lit.height)
    draw_contact_shadow(s_draw, 170, 168, 11, 4)
    s51_beta3 = Image.alpha_composite(s51_lit.convert('RGBA'), s_overlay).convert('RGB')
    s51_base.save(OUT_DIR / '11_snow_summit_5_1_base.png')
    s51_beta3.save(OUT_DIR / '11_snow_summit_5_1_beta3.png')

    # 12. Castillo de Bowser y Lava 6-8
    print("12. Bowser Castle & Lava 6-8 (Fiery ambient + lava bloom + floating embers)...")
    l68_src = Image.open(ROOT / 'docs/captures/level6_8_wide_9340.png')
    l68_crop = crop_active(l68_src)
    l68_base = l68_crop.crop((43, 0, 43 + 256, ACTIVE_H))
    bloom_68 = extract_bloom(l68_crop, threshold=180)
    l68_lit = apply_lighting(l68_crop, (245, 218, 212), bloom_68)
    l_overlay = Image.new('RGBA', l68_lit.size, (0, 0, 0, 0))
    l_draw = ImageDraw.Draw(l_overlay)
    draw_embers(l_draw, l68_lit.width, l68_lit.height)
    l68_beta3 = Image.alpha_composite(l68_lit.convert('RGBA'), l_overlay).convert('RGB')
    l68_base.save(OUT_DIR / '12_castle_lava_6_8_base.png')
    l68_beta3.save(OUT_DIR / '12_castle_lava_6_8_beta3.png')

    print(f"\nSuccessfully generated 12 synchronized comparison pairs in {OUT_DIR}!")

if __name__ == '__main__':
    main()
