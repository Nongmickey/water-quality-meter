#!/usr/bin/env python3
"""วาดจุดวัดด้วยมัลติมิเตอร์บนบอร์ด TDS Meter V1.0 -> docs/tds-board-test-points.svg (แปลงเป็น .jpg แล้วเก็บไว้)

วิธีใช้:  python3 tools/make_tds_board_points.py <รูปบอร์ดที่ครอปแล้ว.jpg>
(รูปครอป 920x1200 จากรูปถ่ายด้านหน้าบอร์ด หัวต่อ - + A อยู่ด้านบน)
"""
import base64
import sys
from pathlib import Path

OUT = Path(__file__).resolve().parent.parent / "docs" / "tds-board-test-points.svg"
IMG_W, IMG_H = 920, 1200
PANEL = 600
W, H = IMG_W + PANEL, IMG_H
INK, MUTED = "#102a32", "#587480"
RED, BLUE, ORANGE, GRAY = "#e53935", "#1e63d6", "#f08c00", "#6b7b80"

img64 = base64.b64encode(Path(sys.argv[1]).read_bytes()).decode()
out = []
A = out.append


def text(x, y, t, size=18, fill=INK, anchor="start", bold=False, halo=False):
    b = ' font-weight="bold"' if bold else ""
    if halo:
        b += ' stroke="#ffffff" stroke-width="6" paint-order="stroke"'
    A(f'<text x="{x}" y="{y}" font-size="{size}" fill="{fill}" text-anchor="{anchor}"{b}>{t}</text>')


def box(x1, y1, x2, y2, color, label, lx, ly, anchor="start"):
    A(f'<rect x="{x1}" y="{y1}" width="{x2-x1}" height="{y2-y1}" rx="8" fill="none" '
      f'stroke="{color}" stroke-width="4" stroke-dasharray="10 6"/>')
    text(lx, ly, label, 20, color, anchor, True, True)


def pin(x, y, num, color, dy):
    A(f'<circle cx="{x}" cy="{y}" r="11" fill="none" stroke="{color}" stroke-width="4"/>')
    text(x, y + dy, num, 20, color, "middle", True, True)


A(f'<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" '
  f'viewBox="0 0 {W} {H}" width="{W}" height="{H}" font-family="Tahoma, \'Noto Sans Thai\', sans-serif">')
A(f'<rect width="{W}" height="{H}" fill="#ffffff"/>')
A(f'<image x="0" y="0" width="{IMG_W}" height="{IMG_H}" xlink:href="data:image/jpeg;base64,{img64}"/>')

# ---------- ชิปแต่ละตัว ----------
box(318, 392, 612, 580, ORANGE, "CD4060 สร้างจังหวะไฟสลับ", 465, 372, "middle")
box(352, 628, 610, 818, BLUE, "LMV324 ขยายสัญญาณ", 481, 862, "middle")
box(175, 662, 282, 752, BLUE, "TPS60400", 228, 790, "middle")
text(228, 814, "สร้างไฟลบ −3V", 17, BLUE, "middle", True, True)
box(700, 270, 805, 360, RED, "ตัวคุมไฟ +3V", 752, 392, "middle")

# ---------- ขาที่ต้องวัด ----------
# CD4060 (ขา 1 = ล่างซ้าย ฝั่งที่มีรอยบุ๋ม)
pin(342, 415, "16", RED, -22)
pin(475, 415, "12", GRAY, -22)
pin(509, 415, "11", ORANGE, -22)
pin(542, 415, "10", ORANGE, -22)
pin(576, 415, "9", ORANGE, -22)
for x, n in [(450, "4"), (483, "5"), (516, "6"), (550, "7")]:
    pin(x, 560, n, ORANGE, 38)
pin(585, 560, "8", GRAY, 38)
# LMV324 (ขา 1 = ล่างซ้าย)
pin(484, 792, "4", RED, 38)
pin(476, 655, "11", BLUE, -22)

# ---------- คำอธิบาย ----------
X = IMG_W + 30
y = 50
text(X, y, "จุดวัดบนบอร์ด TDS ด้วยมัลติมิเตอร์", 26, INK, bold=True)
y += 34
for line in ["เสียบไฟให้บอร์ด (ไฟ LED ติด) · ตั้งมิเตอร์ DC 20V",
             "สายดำแตะ G · สายแดงแตะปลายขาชิปทีละขา",
             "ระวังอย่าให้ปลายสายแตะ 2 ขาพร้อมกัน",
             "นับขา: ขา 1 อยู่ล่างซ้าย (ฝั่งที่มีรอยบุ๋มวงกลม)"]:
    text(X, y, line, 17, MUTED)
    y += 26

steps = [
    (BLUE, "① ไฟลบ (สำคัญที่สุด)", [
        "LMV324 ขา 11 ต้องได้ประมาณ −3 V (มีเครื่องหมายลบ)",
        "ได้ 0 V = ไฟลบไม่มา → TPS60400 หรือ",
        "ตัวเก็บประจุ 2 ตัวข้างมันเสีย"]),
    (RED, "② ไฟเลี้ยง +3 V", [
        "LMV324 ขา 4 และ CD4060 ขา 16 ≈ +3 V",
        "ได้ 0 V = ไฟไม่เข้าชิป (ตัวคุมไฟ / ลายวงจร)"]),
    (GRAY, "③ ขารีเซ็ต CD4060 (ชิปนี้ใช้ไฟ ±3 V)", [
        "ขา 8 ≈ −3 V · ขา 12 ต้องเท่ากับขา 8 (≈ −3 V)",
        "ขา 12 ได้ ≈ +3 V = ชิปถูกสั่งหยุด ไม่สร้างไฟสลับ"]),
    (ORANGE, "④ ตัวสร้างจังหวะ CD4060", [
        "ขา 9, 10, 11 ปกติได้ประมาณ 0 V (กึ่งกลาง ±3 V)",
        "ค้างที่ −3 V หรือ +3 V = ไม่สั่น → CD4060 เสีย",
        "ขา 4, 5, 6, 7 ปกติได้ ≈ 0 V นิ่งๆ",
        "ค่ากระโดด +2 ↔ −2 V ช้าๆ = สั่นช้าเกินไป",
        "(ตรงกับอาการที่เห็น ±30 mV แกว่งช้าๆ)"]),
]
y += 18
for color, title, lines in steps:
    A(f'<rect x="{X-12}" y="{y-26}" width="{PANEL-36}" height="{36 + 27*len(lines)}" rx="10" '
      f'fill="#f7f9fa" stroke="{color}" stroke-width="2.5"/>')
    text(X, y, title, 20, color, bold=True)
    y += 30
    for line in lines:
        text(X + 6, y, line, 17, INK)
        y += 27
    y += 26

text(X, y + 4, "ถ้ามิเตอร์มีโหมด Hz: วัดขา 4–7 ของ CD4060", 17, MUTED, bold=True)
text(X, y + 30, "ปกติควรได้หลักร้อยถึงหลักพัน Hz", 17, MUTED)
text(X, y + 56, "อ้างอิงวงจร DFRobot SEN0244 ที่บอร์ดนี้ลอกแบบมา", 15, MUTED)
A("</svg>")

OUT.write_text("\n".join(out), encoding="utf-8")
print("สร้าง", OUT)
