#!/usr/bin/env python3
"""วาดภาพการต่อวงจรวัด TDS แบบ ESP32 สร้างไฟสลับเอง (โค้ด tds_ac) -> docs/wiring-tds-ac.svg

วิธีใช้:  python3 tools/make_wiring_ac.py
"""
from pathlib import Path

OUT = Path(__file__).resolve().parent.parent / "docs" / "wiring-tds-ac.svg"
W, H = 920, 800
INK, MUTED, TEAL = "#102a32", "#587480", "#0b7480"
RED, BLACK, ORANGE, GREEN, BLUE, PURPLE = "#d64545", "#2b2b2b", "#e08a1e", "#1d8a4a", "#2b6fd6", "#8e44ad"

out = []
A = out.append


def line(pts, color, w=3.5, dash=None):
    d = f' stroke-dasharray="{dash}"' if dash else ""
    A('<polyline points="' + " ".join(f"{x},{y}" for x, y in pts) +
      f'" fill="none" stroke="{color}" stroke-width="{w}" stroke-linejoin="round"{d}/>')


def dot(x, y, c=INK, r=7):
    A(f'<circle cx="{x}" cy="{y}" r="{r}" fill="{c}"/>')


def text(x, y, t, size=14, fill=INK, anchor="start", bold=False):
    b = ' font-weight="bold"' if bold else ""
    A(f'<text x="{x}" y="{y}" font-size="{size}" fill="{fill}" text-anchor="{anchor}"{b}>{t}</text>')


def pin(x, y):
    A(f'<rect x="{x-7}" y="{y-7}" width="14" height="14" fill="#d9c27a" stroke="#8a7330"/>')


def resistor(x1, x2, y, label):
    m = (x1 + x2) / 2
    A(f'<rect x="{m-34}" y="{y-11}" width="68" height="22" rx="5" fill="#f3e3c3" stroke="#6b4f1d" stroke-width="2"/>')
    for i, c in enumerate(["#e0b400", "#7b3fa0", "#c62828", "#c9a227"]):   # เหลือง ม่วง แดง ทอง
        A(f'<rect x="{m-24 + i*13}" y="{y-11}" width="6" height="22" fill="{c}"/>')
    text(m, y - 20, label, 15, "#6b4f1d", "middle", True)


A(f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" width="{W}" height="{H}" '
  "font-family=\"Tahoma, 'Noto Sans Thai', sans-serif\">")
A(f'<rect width="{W}" height="{H}" fill="#ffffff"/>')
text(30, 40, "การต่อวัด TDS แบบ ESP32 สร้างไฟสลับเอง (โค้ด tds_ac)", 22, bold=True)
text(30, 64, "ไม่ใช้บอร์ด TDS · ใช้หัววัดเดิม + ตัวต้านทาน 4.7kΩ 1 ตัว · ไม่ต้องใช้ตัวเก็บประจุ", 13.5, MUTED)

# ---------- บอร์ด TDS (ไม่ใช้) ----------
A('<rect x="40" y="92" width="230" height="74" rx="8" fill="#f1f3f4" stroke="#b9c3c6" stroke-width="2" stroke-dasharray="6 5"/>')
text(155, 122, "บอร์ด TDS", 15, "#8a979b", "middle", True)
text(155, 145, "ถอดสายออกทั้งหมด (ไม่ใช้)", 13, "#c0392b", "middle", True)
line([(52, 100), (258, 158)], "#d9a3a0", 2)
line([(258, 100), (52, 158)], "#d9a3a0", 2)

# ---------- บอร์ดขยาย ESP32 ----------
BX = 680
A(f'<rect x="{BX}" y="92" width="210" height="590" rx="12" fill="#1d2f3a"/>')
text(BX + 105, 124, "บอร์ดขยาย ESP32", 15, "#ffffff", "middle", True)
pins = [("S ของ D25", 190), ("S ของ D33", 280), ("S ของ D26", 370),
        ("S ของ D15", 450), ("G", 490),
        ("S ของ D21", 560), ("S ของ D22", 590), ("VIN (5V)", 620), ("G", 650)]
for lab, y in pins:
    pin(BX, y)
    text(BX + 18, y + 5, lab, 14, "#ffffff")
text(BX + 105, 150, "ใช้ช่อง S เท่านั้น ห้ามใช้ช่อง V", 12, "#f0b4ac", "middle", True)

# ---------- หัววัด TDS เดิม ----------
A('<rect x="60" y="248" width="96" height="150" rx="8" fill="#ffffff" stroke="#9aa9ad" stroke-width="2.5"/>')
text(108, 238, "หัวต่อสีขาวของหัววัด", 13, MUTED, "middle", True)
for lab, y in [("รู 1", 280), ("รู 2", 370)]:
    A(f'<rect x="132" y="{y-9}" width="18" height="18" rx="3" fill="#2b2b2b"/>')
    text(122, y + 5, lab, 14, INK, "end", True)
text(108, 330, "เสียบจัมเปอร์", 11.5, MUTED, "middle")
text(108, 345, "ตัวผู้เข้ารู", 11.5, MUTED, "middle")
line([(108, 398), (108, 470)], "#9aa9ad", 6)
A('<rect x="92" y="470" width="32" height="120" rx="7" fill="#c9d3d6" stroke="#7f9196" stroke-width="2"/>')
for x in (101, 115):
    A(f'<rect x="{x-2}" y="590" width="4" height="14" fill="#7f9196"/>')
text(138, 520, "หัววัด TDS เดิม", 14, INK, bold=True)
text(138, 540, "จุ่มลงน้ำ", 12.5, MUTED)

# ---------- สาย ----------
NX = 360   # จุด X
# D25 -> ตัวต้านทาน -> จุด X
line([(BX - 7, 190), (504, 190)], RED)
resistor(420, 520, 190, "4.7kΩ")
line([(436, 190), (NX, 190), (NX, 280)], RED)
# จุด X -> D33
line([(NX, 280), (BX - 7, 280)], GREEN)
# รู 1 -> จุด X
line([(150, 280), (NX, 280)], ORANGE)
dot(NX, 280)
A(f'<rect x="{NX-40}" y="300" width="80" height="26" rx="13" fill="#fff4cc" stroke="#d4a017"/>')
text(NX, 318, "จุด X", 14, INK, "middle", True)
text(NX, 346, "ห้ามมีตัวเก็บประจุ", 12, "#c0392b", "middle", True)
# รู 2 -> D26
line([(150, 370), (BX - 7, 370)], BLUE)

# ป้ายสาย
text(560, 180, "สายแดง", 12, RED, "middle")
text(560, 272, "สายเขียว", 12, GREEN, "middle")
text(250, 272, "สายส้ม", 12, ORANGE, "middle")
text(560, 362, "สายน้ำเงิน", 12, BLUE, "middle")

# ---------- ปุ่มเขียว ----------
A('<circle cx="420" cy="470" r="26" fill="#2e9e5b" stroke="#1d6b3c" stroke-width="3"/>')
text(420, 512, "ปุ่มเขียว", 13, INK, "middle", True)
text(470, 446, "ขั้ว 23", 12, MUTED)
text(480, 514, "ขั้ว 24", 12, MUTED)
line([(446, 462), (470, 462), (470, 450), (BX - 7, 450)], "#1d8a4a", 3)
line([(446, 478), (470, 478), (470, 490), (BX - 7, 490)], BLACK, 3)

# ---------- จอ LCD ----------
A('<rect x="220" y="545" width="250" height="120" rx="8" fill="#eef4f5" stroke="#0b7480" stroke-width="2"/>')
text(345, 605, "จอ LCD 20×4 (I2C)", 15, INK, "middle", True)
for lab, y, col in [("SDA", 560, BLUE), ("SCL", 590, PURPLE), ("VCC", 620, RED), ("GND", 650, BLACK)]:
    pin(470, y)
    text(456, y + 5, lab, 13, INK, "end", True)
    line([(477, y), (BX - 7, y)], col, 3)

# ---------- คำอธิบาย ----------
notes = [
    ("1) ตัวต้านทาน 4.7kΩ (เหลือง-ม่วง-แดง-ทอง) ต่อระหว่าง S ของ D25 กับจุด X", True),
    ("2) จุด X = ขาตัวต้านทาน + สายไป S ของ D33 + สายไปรู 1 ของหัวต่อหัววัด (บัดกรีหรือมัดรวมให้แน่น แล้วหุ้มเทป)", True),
    ("3) รู 2 ของหัวต่อหัววัด ต่อไป S ของ D26 · สลับรู 1 กับรู 2 ได้ ไม่มีขั้ว", True),
    ("ปุ่มและจอต่อเหมือนเดิม · อัปโหลดโค้ด tds_ac · กดปุ่มสั้น = วัด, กดค้าง 3 วินาที = คาลิเบรต", False),
]
for i, (n, b) in enumerate(notes):
    text(30, 708 + i * 23, n, 13.5, INK if b else MUTED, bold=b)
A("</svg>")

OUT.write_text("\n".join(out), encoding="utf-8")
print("สร้าง", OUT)
