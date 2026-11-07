#!/usr/bin/env python3
"""วาดแผนผังการต่อสาย docs/wiring.svg

วิธีใช้:  python3 tools/make_wiring.py
"""
from pathlib import Path

OUT = Path(__file__).resolve().parent.parent / "docs" / "wiring.svg"
W, H = 900, 780
X5, X33, XG = 262, 296, 330   # บัสไฟ 5V / 3V3 / GND
MX = 540                      # ขอบซ้ายของโมดูล
C5, C33, CG = "#d64545", "#e08a1e", "#2b2b2b"

out = []
A = out.append


def line(pts, color, w=3):
    A('<polyline points="' + " ".join(f"{x},{y}" for x, y in pts) +
      f'" fill="none" stroke="{color}" stroke-width="{w}" stroke-linejoin="round"/>')


def dot(x, y, c="#111"):
    A(f'<circle cx="{x}" cy="{y}" r="5" fill="{c}"/>')


def text(x, y, t, size=14, fill="#102a32", anchor="start", bold=False):
    b = ' font-weight="bold"' if bold else ""
    A(f'<text x="{x}" y="{y}" font-size="{size}" fill="{fill}" text-anchor="{anchor}"{b}>{t}</text>')


def res_v(x, y1, y2, label):
    m = (y1 + y2) / 2
    A(f'<rect x="{x-8}" y="{m-22}" width="16" height="44" rx="3" fill="#f3e3c3" stroke="#6b4f1d" stroke-width="2"/>')
    text(x + 14, m + 5, label, 13, "#6b4f1d", bold=True)


def res_h(x1, x2, y, label):
    m = (x1 + x2) / 2
    A(f'<rect x="{m-22}" y="{y-8}" width="44" height="16" rx="3" fill="#f3e3c3" stroke="#6b4f1d" stroke-width="2"/>')
    text(m, y + 26, label, 13, "#6b4f1d", "middle", True)


def module(y1, y2, title, sub, pins):
    A(f'<rect x="{MX}" y="{y1}" width="320" height="{y2-y1}" rx="8" fill="#eef4f5" stroke="#0b7480" stroke-width="2"/>')
    text(MX + 90, y1 + 24, title, 15, "#102a32", bold=True)
    if sub:
        text(MX + 90, y1 + 44, sub, 12, "#587480")
    for lab, y in pins:
        A(f'<rect x="{MX-5}" y="{y-5}" width="10" height="10" fill="#d9c27a"/>')
        text(MX + 12, y + 5, lab, 13, "#102a32", bold=True)


A(f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" width="{W}" height="{H}" '
  "font-family=\"Tahoma, 'Noto Sans Thai', sans-serif\">")
A(f'<rect width="{W}" height="{H}" fill="#ffffff"/>')
text(30, 40, "แผนผังการต่อสาย · เครื่องวัดคุณภาพน้ำ ESP32", 22, bold=True)

# บัสไฟ
for x, c, lab in [(X5, C5, "5V"), (X33, C33, "3V3"), (XG, CG, "GND")]:
    line([(x, 95), (x, 712)], c, 4)
    text(x, 88, lab, 12, c, "middle", True)

# ESP32
A('<rect x="40" y="70" width="190" height="660" rx="10" fill="#1d2f3a"/>')
text(135, 700, "ESP32", 22, "#ffffff", "middle", True)
text(135, 718, "(บนบอร์ดขยาย)", 12, "#b8c7cd", "middle")
pins = {"VIN (5V)": 105, "GND": 128, "D21 (SDA)": 151, "D22 (SCL)": 174, "D15": 245,
        "3V3": 335, "D34": 381, "D35": 491, "D32": 605}
for name, y in pins.items():
    A(f'<rect x="222" y="{y-5}" width="10" height="10" fill="#d9c27a"/>')
    text(214, y + 5, name, 13, "#ffffff", "end")

# โมดูล
module(88, 195, "จอ LCD 20×4", "โมดูล I2C ด้านหลังจอ", [("VCC", 105), ("GND", 128), ("SDA", 151), ("SCL", 174)])
module(222, 292, "ปุ่มกดสีเขียว", "PNC XB7-EA.5 ใช้ขั้ว NO", [("NO", 245), ("NO", 270)])
module(318, 402, "TDS Meter V1.0", "สายหัววัดเสียบช่องขาว 2 ขา", [("+", 335), ("−", 358), ("A", 381)])
module(428, 512, "โมดูล pH", "หัววัดเสียบช่อง BNC", [("V+", 445), ("G", 468), ("Po", 491)])
module(585, 650, "หัววัดอุณหภูมิ", "NTC 10k (สาย 2 เส้น สลับได้)", [("สาย 1", 605), ("สาย 2", 630)])

# ไฟเลี้ยง
line([(230, 105), (MX, 105)], C5); dot(X5, 105, C5)
line([(230, 128), (MX, 128)], CG); dot(XG, 128, CG)
line([(230, 335), (MX, 335)], C33); dot(X33, 335, C33)
line([(MX, 358), (XG, 358)], CG); dot(XG, 358, CG)
line([(MX, 445), (X5, 445)], C5); dot(X5, 445, C5)
line([(MX, 468), (XG, 468)], CG); dot(XG, 468, CG)
line([(MX, 270), (XG, 270)], CG); dot(XG, 270, CG)

# สัญญาณ
line([(230, 151), (MX, 151)], "#2b6fd6")
line([(230, 174), (MX, 174)], "#8e44ad")
line([(230, 245), (MX, 245)], "#1d8a4a")
line([(230, 381), (MX, 381)], "#0b8a96")

# pH: Po -> 10k -> จุดต่อ -> D35 ; จุดต่อ -> 20k -> GND
NP = 400
line([(MX, 491), (230, 491)], "#a0522d")
res_h(440, 500, 491, "10kΩ")
line([(NP, 491), (NP, 560), (XG, 560)], CG)
res_v(NP, 500, 560, "20kΩ")
dot(XG, 560, CG); dot(NP, 491)

# NTC: 3V3 -> 10k -> จุดต่อ -> D32 ; จุดต่อ -> NTC -> GND
NT = 420
line([(MX, 605), (230, 605)], "#c2185b"); dot(NT, 605)
line([(NT, 605), (NT, 680), (X33, 680)], C33)
res_v(NT, 615, 672, "10kΩ"); dot(X33, 680, C33)
line([(MX, 630), (515, 630), (515, 700), (XG, 700)], CG); dot(XG, 700, CG)

text(30, 752, "● จุด = ต่อกัน · เส้นที่ตัดกันโดยไม่มีจุด = ไม่ต่อกัน · 10kΩ = น้ำตาล-ดำ-ส้ม-ทอง · "
     "20kΩ = แดง-ดำ-ส้ม-ทอง (หรือใช้ 10kΩ 2 ตัวต่อกัน)", 12.5, "#587480")
text(30, 770, "ขาอนาล็อก (D32, D34, D35) ต้องอยู่กลุ่ม 32–39 เท่านั้น · จ่ายไฟผ่านสาย USB", 12.5, "#587480")
A("</svg>")

OUT.write_text("\n".join(out), encoding="utf-8")
print("สร้าง", OUT)
