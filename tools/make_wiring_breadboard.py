#!/usr/bin/env python3
"""วาดการต่อบนเบรดบอร์ด (30 แถว คอลัมน์ a-j รางไฟ 2 ข้าง) -> docs/wiring-breadboard.svg (แปลงเป็น .png ด้วย Playwright แล้วเก็บไว้)

วิธีใช้:  python3 tools/make_wiring_breadboard.py
"""
import math
from pathlib import Path

OUT = Path(__file__).resolve().parent.parent / "docs" / "wiring-breadboard.svg"
W, H = 1200, 1130
INK, MUTED = "#102a32", "#587480"
RED, BLACK, ORANGE, GREEN, BLUE, PURPLE, PINK = "#d64545", "#2b2b2b", "#e08a1e", "#1d8a4a", "#2b6fd6", "#8e44ad", "#c2185b"

P = 24


def Y(r):
    return 84 + r * P


X = {"L+": 380, "L-": 404, "a": 452, "b": 476, "c": 500, "d": 524, "e": 548,
     "f": 604, "g": 628, "h": 652, "i": 676, "j": 700, "R+": 748, "R-": 772}


def hole(col, row):
    return (X[col], Y(row))


out = []
A = out.append


def text(x, y, t, size=14, fill=INK, anchor="start", bold=False, halo=False):
    b = ' font-weight="bold"' if bold else ""
    if halo:
        b += ' stroke="#ffffff" stroke-width="5" paint-order="stroke"'
    A(f'<text x="{x}" y="{y}" font-size="{size}" fill="{fill}" text-anchor="{anchor}"{b}>{t}</text>')


def poly(pts, color, w=4):
    d = " ".join(f"{x},{y}" for x, y in pts)
    A(f'<polyline points="{d}" fill="none" stroke="#ffffff" stroke-width="{w+4}" stroke-linejoin="round" stroke-linecap="round"/>')
    A(f'<polyline points="{d}" fill="none" stroke="{color}" stroke-width="{w}" stroke-linejoin="round" stroke-linecap="round"/>')


def plug(p, color):
    A(f'<circle cx="{p[0]}" cy="{p[1]}" r="6" fill="{color}" stroke="{INK}" stroke-width="2"/>')


def leg(p1, p2):
    A(f'<line x1="{p1[0]}" y1="{p1[1]}" x2="{p2[0]}" y2="{p2[1]}" stroke="#8c8c8c" stroke-width="3"/>')
    for p in (p1, p2):
        A(f'<circle cx="{p[0]}" cy="{p[1]}" r="4" fill="#8c8c8c"/>')


def body(p1, p2):
    mx, my = (p1[0] + p2[0]) / 2, (p1[1] + p2[1]) / 2
    ang = math.degrees(math.atan2(p2[1] - p1[1], p2[0] - p1[0]))
    return mx, my, ang


def resistor(p1, p2):
    leg(p1, p2)
    mx, my, ang = body(p1, p2)
    A(f'<g transform="translate({mx},{my}) rotate({ang})">'
      '<rect x="-17" y="-7" width="34" height="14" rx="5" fill="#f3e3c3" stroke="#6b4f1d" stroke-width="1.5"/>'
      + "".join(f'<rect x="{-12 + i*7}" y="-7" width="3.5" height="14" fill="{c}"/>'
                for i, c in enumerate(["#e0b400", "#7b3fa0", "#c62828", "#c9a227"]))
      + '</g>')


def ceramic(p1, p2):
    leg(p1, p2)
    mx, my, _ = body(p1, p2)
    A(f'<circle cx="{mx}" cy="{my}" r="8" fill="#e3a33a" stroke="#9a6a17" stroke-width="1.5"/>')


def electrolytic(pplus, pminus, big=False):
    leg(pplus, pminus)
    mx, my, ang = body(pplus, pminus)
    w = 30 if big else 24
    A(f'<g transform="translate({mx},{my}) rotate({ang})">'
      f'<rect x="{-w/2}" y="-8" width="{w}" height="16" rx="4" fill="#2f5fb3" stroke="#1d3d75" stroke-width="1.5"/>'
      f'<rect x="{w/2-6}" y="-8" width="6" height="16" fill="#cfd8e8"/></g>')
    text(pplus[0] + (6 if pplus[0] >= pminus[0] else -6), pplus[1] - 7, "+", 13, RED,
         "start" if pplus[0] >= pminus[0] else "end", True, True)


def badge(x, y, t, color):
    A(f'<rect x="{x-14}" y="{y-11}" width="28" height="22" rx="11" fill="{color}"/>')
    text(x, y + 5, t, 13, "#ffffff", "middle", True)


A(f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" width="{W}" height="{H}" '
  "font-family=\"Tahoma, 'Noto Sans Thai', sans-serif\">")
A(f'<rect width="{W}" height="{H}" fill="#ffffff"/>')
text(30, 28, "การต่อบนเบรดบอร์ด (บอร์ดไข่ปลา) แล้วต่อเข้า ESP32", 22, bold=True)

# ---------- เบรดบอร์ด ----------
A('<rect x="356" y="62" width="440" height="766" rx="10" fill="#f7f7f2" stroke="#cfcfc4" stroke-width="2"/>')
A('<rect x="566" y="96" width="20" height="724" fill="#e6e6dd"/>')
A('<line x1="366" y1="100" x2="366" y2="816" stroke="#e0476b" stroke-width="2.5"/>')
A('<line x1="418" y1="100" x2="418" y2="816" stroke="#3aa0e0" stroke-width="2.5"/>')
A('<line x1="734" y1="100" x2="734" y2="816" stroke="#e0476b" stroke-width="2.5"/>')
A('<line x1="786" y1="100" x2="786" y2="816" stroke="#3aa0e0" stroke-width="2.5"/>')
for col, x in X.items():
    for r in range(1, 31):
        A(f'<rect x="{x-4}" y="{Y(r)-4}" width="8" height="8" fill="#55656a"/>')
for col in "abcdefghij":
    text(X[col], 92, col, 12, MUTED, "middle", True)
for col, sym, c in [("L+", "+", RED), ("L-", "−", BLUE), ("R+", "+", RED), ("R-", "−", BLUE)]:
    text(X[col], 92, sym, 15, c, "middle", True)
for r in range(1, 31):
    text(434, Y(r) + 4, str(r), 9, MUTED, "middle")
    text(724, Y(r) + 4, str(r), 9, MUTED, "middle")
# ป้ายราง
text(370, 59, "3V3", 12, RED, "end", True)
text(414, 59, "GND", 12, BLUE, "start", True)
text(738, 59, "5V", 12, RED, "end", True)
text(782, 59, "GND", 12, BLUE, "start", True)

# ---------- ESP32 ----------
A('<rect x="30" y="60" width="200" height="600" rx="12" fill="#1d2f3a"/>')
text(130, 640, "บอร์ดขยาย ESP32", 15, "#ffffff", "middle", True)
epins = [("VIN (5V)", 84), ("3V3", 108), ("G", 144), ("S ของ D34", 216), ("S ของ D35", 384), ("S ของ D32", 576)]
for lab, y in epins:
    A(f'<rect x="223" y="{y-7}" width="14" height="14" fill="#d9c27a" stroke="#8a7330"/>')
    text(212, y + 5, lab, 14, "#ffffff", "end")
text(130, 470, "จอ LCD และปุ่มเขียว", 12, "#9fc3cc", "middle")
text(130, 488, "ต่อตรงที่บอร์ดขยาย", 12, "#9fc3cc", "middle")
text(130, 506, "เหมือนเดิม", 12, "#9fc3cc", "middle")

# ---------- โมดูล ----------
A('<rect x="860" y="140" width="300" height="104" rx="8" fill="#eef4f5" stroke="#0b7480" stroke-width="2"/>')
text(1010, 178, "บอร์ด TDS", 16, INK, "middle", True)
text(1010, 200, "หัววัดเสียบช่องขาว 2 ขา", 12, MUTED, "middle")
for lab, y in [("+", 168), ("−", 192), ("A", 216)]:
    A(f'<rect x="853" y="{y-7}" width="14" height="14" fill="#d9c27a" stroke="#8a7330"/>')
    text(874, y + 5, lab, 14, INK, "start", True)

A('<rect x="860" y="290" width="300" height="160" rx="8" fill="#eef4f5" stroke="#0b7480" stroke-width="2"/>')
text(1010, 330, "โมดูล pH", 16, INK, "middle", True)
text(1010, 352, "หัววัดเสียบช่อง BNC", 12, MUTED, "middle")
for lab, y in [("V+", 312), ("G", 336), ("Po", 384), ("To ไม่ต่อ", 408), ("Do ไม่ต่อ", 432)]:
    A(f'<rect x="853" y="{y-7}" width="14" height="14" fill="#d9c27a" stroke="#8a7330"/>')
    text(874, y + 5, lab, 13, INK if "ไม่ต่อ" not in lab else MUTED, "start", True)

A('<rect x="1090" y="560" width="22" height="90" rx="9" fill="#c7cdd0" stroke="#7d878b" stroke-width="2"/>')
A('<polyline points="1101,560 1101,540 1010,540 970,576 867,576" fill="none" stroke="#3a3a3a" stroke-width="4"/>')
A('<polyline points="1010,540 970,612 867,612" fill="none" stroke="#3a3a3a" stroke-width="4"/>')
text(1060, 690, "หัววัดอุณหภูมิ NTC", 14, INK, "middle", True)
text(1060, 710, "(สลับสาย 1 กับ 2 ได้)", 12, MUTED, "middle")
for lab, y in [("สาย 1", 576), ("สาย 2", 612)]:
    A(f'<rect x="853" y="{y-7}" width="14" height="14" fill="#d9c27a" stroke="#8a7330"/>')
    text(880, y - 8, lab, 12, INK, "start", True)

# ---------- ชิ้นส่วนบนเบรดบอร์ด ----------
# ตัวเก็บประจุ 100µF คร่อมรางไฟ
electrolytic(hole("L+", 3), hole("L-", 3), True)
electrolytic(hole("R+", 3), hole("R-", 3), True)
# TDS แถว 6
resistor(hole("e", 6), hole("f", 6))
ceramic(hole("a", 6), hole("L-", 6))
electrolytic(hole("b", 6), hole("L-", 7))
# pH แถว 13
resistor(hole("e", 13), hole("f", 13))
resistor(hole("a", 13), hole("L-", 16))
ceramic(hole("b", 13), hole("L-", 12))
electrolytic(hole("c", 13), hole("L-", 10))
# NTC แถว 21
resistor(hole("L+", 22), hole("a", 21))
ceramic(hole("b", 21), hole("L-", 20))
electrolytic(hole("c", 21), hole("L-", 24))

# ---------- สาย ----------
# ไฟจาก ESP32
poly([(237, 84), (300, 84), (300, 44), (748, 44), hole("R+", 1)], RED)
plug(hole("R+", 1), RED)
poly([(237, 108), hole("L+", 1)], ORANGE)
plug(hole("L+", 1), ORANGE)
poly([(237, 144), (392, 144), hole("L-", 2)], BLACK)
plug(hole("L-", 2), BLACK)
# กราวด์สองฝั่งต่อกัน
poly([hole("L-", 30), (404, 850), (772, 850), hole("R-", 30)], BLACK)
plug(hole("L-", 30), BLACK)
plug(hole("R-", 30), BLACK)
text(588, 870, "สายดำเชื่อมราง GND ซ้าย-ขวา (แถว 30)", 12, BLACK, "middle", True)
# TDS
poly([(860, 168), (760, 168), hole("R+", 4)], RED, 3)
plug(hole("R+", 4), RED)
poly([(860, 192), (784, 192), hole("R-", 5)], BLACK, 3)
plug(hole("R-", 5), BLACK)
poly([(860, 216), (712, 216), hole("j", 6)], "#c9a000", 3)
plug(hole("j", 6), "#c9a000")
poly([(237, 216), (488, 216), hole("c", 6)], "#0b8fa0", 4)
plug(hole("c", 6), "#0b8fa0")
# pH
poly([(860, 312), (760, 312), hole("R+", 10)], RED, 3)
plug(hole("R+", 10), RED)
poly([(860, 336), (784, 336), hole("R-", 11)], BLACK, 3)
plug(hole("R-", 11), BLACK)
poly([(860, 384), (712, 384), hole("j", 13)], "#a0522d", 3)
plug(hole("j", 13), "#a0522d")
poly([(237, 384), (512, 384), hole("d", 13)], GREEN, 4)
plug(hole("d", 13), GREEN)
# NTC
poly([(860, 576), (560, 576), hole("e", 21)], PURPLE, 3)
plug(hole("e", 21), PURPLE)
poly([(860, 612), hole("R-", 22)], BLACK, 3)
plug(hole("R-", 22), BLACK)
poly([(237, 576), (512, 576), hole("d", 21)], PINK, 4)
plug(hole("d", 21), PINK)

# ป้ายวงจร
for r, n, lab, c in [(6, "1", "TDS", "#0b8fa0"), (13, "2", "pH", GREEN), (21, "3", "อุณหภูมิ", PINK)]:
    badge(624, Y(r) + 30, n, c)
    text(644, Y(r) + 35, lab, 13, c, "start", True, True)

# ---------- ป้ายบอกค่าชิ้นส่วน ----------
CAP, CER, RES = "#1d3d75", "#b36b00", "#6b4f1d"


def mid(p1, p2):
    return ((p1[0] + p2[0]) / 2, (p1[1] + p2[1]) / 2)


def tag(x, y, t, target, color):
    """ป้ายค่าชิ้นส่วนที่ขอบซ้าย/ขวา พร้อมเส้นชี้ไปที่ตัวชิ้นส่วน"""
    tx, ty = target
    sx = x + 4 if x < tx else x - 4
    A(f'<line x1="{sx}" y1="{y-5}" x2="{tx}" y2="{ty}" stroke="{color}" stroke-width="1.5" stroke-dasharray="4 3"/>')
    A(f'<circle cx="{tx}" cy="{ty}" r="3" fill="{color}"/>')
    text(x, y, t, 14, color, "end" if x < tx else "start", True, True)


# 100µF คร่อมรางไฟ
tag(346, 166, "100µF", mid(hole("L+", 3), hole("L-", 3)), CAP)
text(803, 140, "100µF", 14, CAP, "start", True, True)
# TDS แถว 6
tag(346, 248, "104", mid(hole("a", 6), hole("L-", 6)), CER)
tag(346, 272, "10µF", mid(hole("b", 6), hole("L-", 7)), CAP)
# pH แถว 13
tag(346, 350, "10µF", mid(hole("c", 13), hole("L-", 10)), CAP)
tag(346, 410, "104", mid(hole("b", 13), hole("L-", 12)), CER)
tag(346, 440, "4.7kΩ", mid(hole("a", 13), hole("L-", 16)), RES)
# อุณหภูมิ แถว 21
tag(346, 556, "104", mid(hole("b", 21), hole("L-", 20)), CER)
tag(346, 606, "4.7kΩ", mid(hole("L+", 22), hole("a", 21)), RES)
tag(346, 644, "10µF", mid(hole("c", 21), hole("L-", 24)), CAP)
# ตัวต้านทานคร่อมร่องกลาง
for r in (6, 13):
    text(576, Y(r) - 12, "4.7kΩ", 12, RES, "middle", True, True)

# ---------- คำอธิบาย ----------
y = 905
notes = [
    ("ราง + ซ้าย = 3V3 · ราง − ซ้าย = GND · ราง + ขวา = 5V (VIN) · ราง − ขวา = GND   (แต่ละแถว a–e ต่อถึงกัน, f–j ต่อถึงกัน, ร่องกลางไม่ต่อ)", True),
    ("① TDS แถว 6: A→j6 · 4.7k คร่อมร่อง e6–f6 · 104: a6→ราง− · 10µF: ขา+ b6, ขา− ราง− · c6→D34 · บอร์ด TDS + → ราง+ขวา (5V), − → ราง−ขวา", False),
    ("② pH แถว 13: Po→j13 · 4.7k คร่อมร่อง e13–f13 · 4.7k อีกตัว a13→ราง− · 104: b13→ราง− · 10µF: ขา+ c13, ขา− ราง− · d13→D35 · V+→ราง+ขวา, G→ราง−ขวา", False),
    ("③ อุณหภูมิ แถว 21: 4.7k จากราง+ซ้าย (3V3)→a21 · 104: b21→ราง− · 10µF: ขา+ c21, ขา− ราง− · d21→D32 · NTC สาย 1→e21, สาย 2→ราง−ขวา", False),
    ("100µF 2 ตัว: คร่อมราง + กับ − แถว 3 ทั้งซ้ายและขวา (ขายาว + เข้าราง +) · 10µF ขายาว (+) เสียบฝั่งแถววงจร ขาข้างแถบเข้าราง −", False),
    ("ก่อนเสียบไฟ: ใช้มิเตอร์โหมดปี๊บ เช็กว่าราง − แถว 1 กับแถว 30 ต่อถึงกัน (บางบอร์ดรางขาดกลาง ถ้าขาดให้ใช้สายจัมเปอร์เชื่อมแถว 15–16)", False),
]
for n, b in notes:
    text(30, y, n, 13.5, INK if b else INK, bold=b)
    y += 28
text(30, y + 6, "ตัวต้านทานทุกตัว 4.7kΩ (เหลือง-ม่วง-แดง-ทอง) · ตำแหน่งรูอื่นในแถวเดียวกันใช้แทนกันได้ ขอแค่อยู่แถวเดียวกันและฝั่งเดียวกันของร่องกลาง", 13, MUTED)
A("</svg>")

OUT.write_text("\n".join(out), encoding="utf-8")
print("สร้าง", OUT)
