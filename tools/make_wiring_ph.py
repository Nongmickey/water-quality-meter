#!/usr/bin/env python3
"""วาดภาพการต่อเซนเซอร์ pH (โค้ด ph_test) -> docs/wiring-ph.svg

วิธีใช้:  python3 tools/make_wiring_ph.py
"""
from pathlib import Path

OUT = Path(__file__).resolve().parent.parent / "docs" / "wiring-ph.svg"
W, H = 920, 930
INK, MUTED = "#102a32", "#587480"
RED, BLACK, ORANGE, GREEN, BLUE, PURPLE = "#d64545", "#2b2b2b", "#e08a1e", "#1d8a4a", "#2b6fd6", "#8e44ad"

out = []
A = out.append


def line(pts, color, w=3.5, dash=None):
    d = f' stroke-dasharray="{dash}"' if dash else ""
    A('<polyline points="' + " ".join(f"{x},{y}" for x, y in pts) +
      f'" fill="none" stroke="{color}" stroke-width="{w}" stroke-linejoin="round"{d}/>')


def dot(x, y, c=INK, r=7):
    A(f'<circle cx="{x}" cy="{y}" r="{r}" fill="{c}"/>')


def text(x, y, t, size=14, fill=INK, anchor="start", bold=False, halo=False):
    b = ' font-weight="bold"' if bold else ""
    if halo:   # ขอบขาวรอบตัวอักษร ให้อ่านออกเมื่อทับเส้น
        b += ' stroke="#ffffff" stroke-width="5" paint-order="stroke"'
    A(f'<text x="{x}" y="{y}" font-size="{size}" fill="{fill}" text-anchor="{anchor}"{b}>{t}</text>')


def pin(x, y):
    A(f'<rect x="{x-7}" y="{y-7}" width="14" height="14" fill="#d9c27a" stroke="#8a7330"/>')


BANDS = ["#e0b400", "#7b3fa0", "#c62828", "#c9a227"]   # เหลือง ม่วง แดง ทอง = 4.7kΩ


def resistor_h(x1, x2, y, label):
    m = (x1 + x2) / 2
    A(f'<rect x="{m-34}" y="{y-11}" width="68" height="22" rx="5" fill="#f3e3c3" stroke="#6b4f1d" stroke-width="2"/>')
    for i, c in enumerate(BANDS):
        A(f'<rect x="{m-24 + i*13}" y="{y-11}" width="6" height="22" fill="{c}"/>')
    text(m, y - 20, label, 15, "#6b4f1d", "middle", True)


def resistor_v(x, y1, y2, label):
    m = (y1 + y2) / 2
    A(f'<rect x="{x-11}" y="{m-34}" width="22" height="68" rx="5" fill="#f3e3c3" stroke="#6b4f1d" stroke-width="2"/>')
    for i, c in enumerate(BANDS):
        A(f'<rect x="{x-11}" y="{m-24 + i*13}" width="22" height="6" fill="{c}"/>')
    text(x - 18, m + 5, label, 14, "#6b4f1d", "end", True)


def electrolytic(x, y1, y2):
    m = (y1 + y2) / 2
    A(f'<rect x="{x-15}" y="{m-30}" width="30" height="60" rx="6" fill="#2f5fb3" stroke="#1d3d75" stroke-width="2"/>')
    A(f'<rect x="{x+5}" y="{m-30}" width="10" height="60" fill="#cfd8e8"/>')       # แถบขาว = ขา −
    text(x - 22, m - 14, "+", 18, RED, "end", True)
    text(x - 22, m + 26, "−", 18, INK, "end", True)
    text(x, m + 48, "10µF", 13.5, INK, "middle", True, True)


def ceramic(x, y1, y2):
    m = (y1 + y2) / 2
    A(f'<ellipse cx="{x}" cy="{m}" rx="17" ry="14" fill="#e3a33a" stroke="#9a6a17" stroke-width="2"/>')
    text(x, m + 5, "104", 11, INK, "middle", True)
    text(x, m + 34, "0.1µF", 12.5, INK, "middle", True, True)


A(f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" width="{W}" height="{H}" '
  "font-family=\"Tahoma, 'Noto Sans Thai', sans-serif\">")
A(f'<rect width="{W}" height="{H}" fill="#ffffff"/>')
text(30, 40, "การต่อเซนเซอร์ pH กับ ESP32 (โค้ด ph_test)", 22, bold=True)
text(30, 64, "โมดูล pH ใช้ไฟ 5V · ขา Po ออกได้ถึง 5V จึงต้องผ่านตัวแบ่งแรงดัน 4.7k + 4.7k ให้เหลือครึ่งหนึ่งก่อนเข้า ESP32", 13.5, MUTED)

# ---------- บอร์ดขยาย ESP32 ----------
BX = 680
A(f'<rect x="{BX}" y="92" width="210" height="660" rx="12" fill="#1d2f3a"/>')
text(BX + 105, 124, "บอร์ดขยาย ESP32", 15, "#ffffff", "middle", True)
text(BX + 105, 150, "ใช้ช่อง S เท่านั้น ห้ามใช้ช่อง V", 12, "#f0b4ac", "middle", True)
pins = [("S ของ D35", 210), ("G", 430), ("VIN (5V)", 500),
        ("S ของ D15", 560), ("G", 590),
        ("S ของ D21", 630), ("S ของ D22", 660), ("VIN (5V)", 690), ("G", 720)]
for lab, y in pins:
    pin(BX, y)
    text(BX + 18, y + 5, lab, 14, "#ffffff")
text(BX + 105, 290, "ช่อง D25 D26 D33 (TDS)", 12, "#9fc3cc", "middle")
text(BX + 105, 308, "และ D32 (NTC) ต่อค้างไว้ได้", 12, "#9fc3cc", "middle")

# ---------- โมดูล pH ----------
MX1, MX2, MY1, MY2 = 50, 290, 110, 330
A(f'<rect x="{MX1}" y="{MY1}" width="{MX2-MX1}" height="{MY2-MY1}" rx="8" fill="#1f4f8f" stroke="#123461" stroke-width="2.5"/>')
text((MX1 + MX2) / 2, MY1 - 8, "โมดูล pH (บอร์ดสีน้ำเงิน)", 14, INK, "middle", True)
# BNC
A('<circle cx="100" cy="165" r="27" fill="#c9cfd2" stroke="#7d878b" stroke-width="3"/>')
A('<circle cx="100" cy="165" r="10" fill="#7d878b"/>')
text(100, 212, "ช่อง BNC", 12.5, "#ffffff", "middle", True)
# ปุ่มปรับ 2 ตัว
for x, lab in [(165, "1"), (215, "2")]:
    A(f'<rect x="{x-15}" y="250" width="30" height="30" rx="3" fill="#2d6fd0" stroke="#d9e3f2" stroke-width="2"/>')
    A(f'<circle cx="{x}" cy="265" r="8" fill="#e8c766" stroke="#8a7330"/>')
    text(x, 300, f"ปรับ {lab}", 11.5, "#ffffff", "middle", True)
# ขา header
hdr = [("To", 140), ("Do", 170), ("Po", 200), ("G", 230), ("G", 260), ("V+", 290)]
for lab, y in hdr:
    pin(MX2, y)
    text(MX2 - 14, y + 5, lab, 13, "#ffffff", "end", True)

# ---------- หัววัด pH ----------
line([(73, 165), (58, 165), (40, 190), (40, 360), (95, 380)], "#3a3a3a", 6)
A('<rect x="80" y="380" width="30" height="170" rx="8" fill="#e6eef0" stroke="#7f9196" stroke-width="2"/>')
A('<ellipse cx="95" cy="565" rx="18" ry="20" fill="#d4ecf2" stroke="#7f9196" stroke-width="2"/>')
text(125, 440, "หัววัด pH", 14, INK, bold=True)
text(125, 460, "ถอดฝาครอบ", 12.5, MUTED)
text(125, 478, "แล้วจุ่มน้ำ", 12.5, MUTED)

# ---------- สาย: To, Do ไม่ใช้ ----------
for y in (140, 170):
    line([(MX2 + 7, y), (MX2 + 40, y)], "#b9c3c6", 2.5, "5 4")
text(MX2 + 46, 160, "To, Do ไม่ใช้", 12, MUTED)

# ---------- Po -> 4.7k -> จุด P -> D35 ----------
NX, NY = 470, 200      # จุด P
line([(MX2 + 7, 200), (NX, 200)], ORANGE)
resistor_h(330, 440, 200, "R1 4.7kΩ")
line([(NX, NY), (BX - 7, NY)], GREEN)
text(580, 192, "สายเขียว → D35", 12, GREEN, "middle")

# จุด P ลงกราวด์: R2, 10µF, 104
GY = 430               # รางกราวด์
line([(NX, NY), (NX, GY)], INK, 3)
line([(NX, 255), (600, 255)], INK, 3)
line([(540, 255), (540, GY)], INK, 3)
line([(600, 255), (600, GY)], INK, 3)
resistor_v(NX, 300, 390, "R2 4.7kΩ")
electrolytic(540, 300, 380)
ceramic(600, 320, 360)
dot(NX, NY)
dot(NX, 255, INK, 5)
dot(540, 255, INK, 5)
A(f'<rect x="{NX-38}" y="{NY-50}" width="76" height="26" rx="13" fill="#fff4cc" stroke="#d4a017"/>')
text(NX, NY - 32, "จุด P", 14, INK, "middle", True)

# รางกราวด์ + G ของโมดูล
line([(345, GY), (BX - 7, GY)], BLACK, 4)
line([(MX2 + 7, 230), (345, 230), (345, GY)], BLACK)
dot(NX, GY, INK, 5)
dot(540, GY, INK, 5)
dot(600, GY, INK, 5)
text(560, 452, "รางกราวด์ → G", 12, BLACK, "middle")

# V+ -> VIN (5V)
line([(MX2 + 7, 290), (320, 290), (320, 500), (BX - 7, 500)], RED)
text(560, 492, "สายแดง → VIN (5V)", 12, RED, "middle")

# ---------- ปุ่มเขียว ----------
A('<circle cx="330" cy="575" r="26" fill="#2e9e5b" stroke="#1d6b3c" stroke-width="3"/>')
text(290, 580, "ปุ่มเขียว", 13, INK, "end", True)
text(370, 552, "ขั้ว 23", 12, MUTED)
text(370, 610, "ขั้ว 24", 12, MUTED)
line([(356, 567), (360, 567), (360, 560), (BX - 7, 560)], "#1d8a4a", 3)
line([(356, 583), (360, 583), (360, 590), (BX - 7, 590)], BLACK, 3)

# ---------- จอ LCD ----------
A('<rect x="220" y="620" width="250" height="120" rx="8" fill="#eef4f5" stroke="#0b7480" stroke-width="2"/>')
text(345, 685, "จอ LCD 20×4 (I2C)", 15, INK, "middle", True)
for lab, y, col in [("SDA", 630, BLUE), ("SCL", 660, PURPLE), ("VCC", 690, RED), ("GND", 720, BLACK)]:
    pin(470, y)
    text(456, y + 5, lab, 13, INK, "end", True)
    line([(477, y), (BX - 7, y)], col, 3)

# ---------- คำอธิบาย ----------
notes = [
    ("1) Po → R1 4.7kΩ → จุด P · จุด P → สายไป S ของ D35 · จุด P → R2 4.7kΩ → G   (แรงดันเหลือครึ่งหนึ่ง)", True),
    ("2) 10µF: ขายาว (+) ต่อจุด P, ขาข้างแถบขาว (−) ต่อ G · 104: ไม่มีขั้ว ต่อจุด P กับ G", True),
    ("3) V+ → VIN (5V) · G → G · ขา To, Do ไม่ต่อ · ตัวต้านทานเหลือง-ม่วง-แดง-ทอง = 4.7kΩ", True),
    ("4) ตั้งศูนย์ก่อนใช้: ถอดหัววัด ช็อตรูกลางของ BNC กับขอบนอก แล้วหมุน \"ปรับ 1\" (ใกล้ BNC) จน Po ≈ 2.50 V", True),
    ("   (วัดด้วยมิเตอร์ที่ขา Po กับ G · ที่ D35 จะเห็นครึ่งหนึ่ง ≈ 1.25 V) · \"ปรับ 2\" ไม่ต้องหมุน", False),
    ("อัปโหลดโค้ด ph_test · กดสั้น = วัด · กดค้าง 3 วิ แล้วปล่อย = คาลิเบรต 6.86 · กดค้าง 6 วิ แล้วปล่อย = คาลิเบรต 4.00", False),
]
for i, (n, b) in enumerate(notes):
    text(30, 785 + i * 23, n, 13.5, INK if b else MUTED, bold=b)
A("</svg>")

OUT.write_text("\n".join(out), encoding="utf-8")
print("สร้าง", OUT)
