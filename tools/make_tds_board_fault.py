#!/usr/bin/env python3
"""วาดภาพจุดที่เสียบนบอร์ด TDS Meter V1.0 พร้อมผลวัดมัลติมิเตอร์ -> docs/tds-board-fault.svg

วิธีใช้:  python3 tools/make_tds_board_fault.py <รูปบอร์ดที่ครอปแล้ว.jpg>
(รูปครอป 920x1200 จากรูปถ่ายด้านหน้าบอร์ด หัวต่อ - + A อยู่ด้านบน) แล้วแปลงเป็น .jpg เก็บไว้
"""
import base64
import sys
from pathlib import Path

OUT = Path(__file__).resolve().parent.parent / "docs" / "tds-board-fault.svg"
IMG_W, IMG_H = 920, 1200
PANEL = 640
W, H = IMG_W + PANEL, IMG_H
INK, MUTED = "#102a32", "#587480"
OK, BAD = "#1e9e4a", "#e53935"

img64 = base64.b64encode(Path(sys.argv[1]).read_bytes()).decode()
out = []
A = out.append


def text(x, y, t, size=18, fill=INK, anchor="start", bold=False, halo=False):
    b = ' font-weight="bold"' if bold else ""
    if halo:
        b += ' stroke="#ffffff" stroke-width="6" paint-order="stroke"'
    A(f'<text x="{x}" y="{y}" font-size="{size}" fill="{fill}" text-anchor="{anchor}"{b}>{t}</text>')


def ring(x, y, color):
    A(f'<circle cx="{x}" cy="{y}" r="11" fill="none" stroke="{color}" stroke-width="4"/>')


def badge(x, y, n, color, r=16):
    A(f'<circle cx="{x}" cy="{y}" r="{r}" fill="{color}" stroke="#ffffff" stroke-width="3"/>')
    text(x, y + 7, n, r + 3, "#ffffff", "middle", True)


A(f'<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" '
  f'viewBox="0 0 {W} {H}" width="{W}" height="{H}" font-family="Tahoma, \'Noto Sans Thai\', sans-serif">')
A(f'<rect width="{W}" height="{H}" fill="#ffffff"/>')
A(f'<image x="0" y="0" width="{IMG_W}" height="{IMG_H}" xlink:href="data:image/jpeg;base64,{img64}"/>')

# ---------- ⑦ บริเวณที่น่าจะเสีย (ชิ้นส่วนกำหนดจังหวะของ CD4060) ----------
A('<rect x="590" y="225" width="118" height="222" rx="12" fill="#e53935" fill-opacity="0.18" '
  'stroke="#e53935" stroke-width="5" stroke-dasharray="12 7"/>')
badge(649, 196, "7", BAD, 19)
text(649, 160, "จุดที่น่าจะเสีย", 22, BAD, "middle", True, True)

# ---------- CD4060 ----------
A('<rect x="318" y="392" width="268" height="190" rx="8" fill="none" stroke="#f08c00" stroke-width="3"/>')
text(452, 500, "CD4060", 22, "#ffffff", "middle", True)
text(452, 524, "ตัวชิปยังทำงาน", 15, "#ffd8a8", "middle", True)
text(615, 735, "LMV324", 20, OK, "start", True, True)
# ① TPS60400
A('<rect x="175" y="662" width="107" height="90" rx="8" fill="none" stroke="#1e9e4a" stroke-width="3"/>')
badge(228, 785, "1", OK)
text(228, 832, "TPS60400", 18, OK, "middle", True, True)
# ② LMV324 ขา 11, ③ LMV324 ขา 4
ring(476, 655, OK)
badge(476, 700, "2", OK)
ring(484, 792, OK)
badge(484, 750, "3", OK)
# ④ CD4060 ขา 12 และ 8
ring(475, 415, OK)
badge(475, 378, "4", OK)
ring(585, 560, OK)
badge(585, 600, "4", OK)
# ⑤ CD4060 ขา 9, 10, 11
for x in (509, 542, 576):
    ring(x, 415, OK)
badge(542, 378, "5", OK)
# ⑥ CD4060 ขา 4, 5, 6 (ผิดปกติ)
for x in (450, 483, 516):
    ring(x, 560, BAD)
badge(483, 600, "6", BAD)

# ---------- คำอธิบาย ----------
X = IMG_W + 30
y = 52
text(X, y, "จุดที่เสียบนบอร์ด TDS Meter V1.0", 28, INK, bold=True)
y += 32
text(X, y, "ผลวัดด้วยมัลติมิเตอร์ (DC · สายดำแตะ G · บอร์ดเสียบไฟ)", 17, MUTED)
y += 34

rows = [
    ("1", OK, "TPS60400 (สร้างไฟลบ)", "−3 V", "ปกติ"),
    ("2", OK, "LMV324 ขา 11 (ไฟลบเข้าชิป)", "−3 V", "ปกติ"),
    ("3", OK, "LMV324 ขา 4 (ไฟบวกเข้าชิป)", "+3 V", "ปกติ"),
    ("4", OK, "CD4060 ขา 8, 12 (ไฟ/รีเซ็ต)", "−2.9 V", "ปกติ"),
    ("5", OK, "CD4060 ขา 9, 10, 11 (ตัวสั่น)", "≈ 0 V", "ยังสั่นอยู่"),
    ("6", BAD, "CD4060 ขา 4, 5, 6 (ไฟสลับ)", "+2 ↔ −2 V", "ช้าเกินไป"),
    ("7", BAD, "ชิ้นส่วนกำหนดจังหวะ (กรอบแดง)", "—", "น่าจะค่าผิด"),
]
for n, color, what, val, verdict in rows:
    A(f'<rect x="{X-12}" y="{y-24}" width="{PANEL-36}" height="46" rx="10" '
      f'fill="{"#eef8f1" if color == OK else "#fdecec"}" stroke="{color}" stroke-width="2"/>')
    badge(X + 8, y - 1, n, color, 14)
    text(X + 34, y + 6, what, 17, INK)
    text(X + 432, y + 6, val, 17, INK, "end", True)
    text(X + PANEL - 56, y + 6, ("✓ " if color == OK else "✗ ") + verdict, 17, color, "end", True)
    y += 56

y += 8
A(f'<rect x="{X-12}" y="{y-26}" width="{PANEL-36}" height="300" rx="12" fill="#f7f9fa" stroke="#b9c3c6" stroke-width="2"/>')
text(X, y, "ทำไมจุ่มน้ำแล้วได้ 0 V", 21, INK, bold=True)
y += 34
for line in [
    "1) ปกติ CD4060 สลับขั้วไฟหลายพันครั้งต่อวินาที",
    "    มิเตอร์จะเห็นขา 4, 5, 6 นิ่งที่ประมาณ 0 V",
    "2) บอร์ดนี้สลับช้าจนมิเตอร์เห็นค่ากระโดด +2 ↔ −2 V",
    "    = ช้ากว่าปกติหลายร้อยถึงหลายพันเท่า",
    "3) ไฟที่ป้อนหัววัดจึงเกือบเป็นไฟตรง",
    "    ขั้วโลหะในน้ำเกิดประจุกั้นไฟทันที → ได้ 0 V",
    "4) ตัวต้านทาน / ไขควงไม่เกิดประจุกั้นไฟ จึงยังวัดได้",
    "5) ไฟเลี้ยงทุกเส้นปกติ → ไม่ได้เสียจากการต่อไฟผิด",
]:
    text(X, y, line, 17, INK)
    y += 30

y += 34
A(f'<rect x="{X-12}" y="{y-30}" width="{PANEL-36}" height="96" rx="12" fill="#fdecec" stroke="{BAD}" stroke-width="3"/>')
text(X, y, "สรุป: ชิ้นส่วนกำหนดจังหวะของ CD4060 ผิดค่า", 21, BAD, bold=True)
text(X, y + 32, "น่าจะเสียจากโรงงาน ประมาณ 85–90%", 21, BAD, bold=True)
text(X, y + 86, "กรอบแดง = บริเวณชิ้นส่วนจิ๋วที่อยู่ติดขา 9–11 (ระบุตัวที่เสียแน่ชัดต้องดูแผนผังวงจรจริง)", 13.5, MUTED)
A("</svg>")

OUT.write_text("\n".join(out), encoding="utf-8")
print("สร้าง", OUT)
