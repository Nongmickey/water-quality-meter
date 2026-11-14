#!/usr/bin/env python3
"""วาดภาพการต่อหัววัดอุณหภูมิ NTC (โค้ด temp_test) -> docs/wiring-temp.svg

วิธีใช้:  python3 tools/make_wiring_temp.py
"""
from pathlib import Path

OUT = Path(__file__).resolve().parent.parent / "docs" / "wiring-temp.svg"
W, H = 920, 840
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


def resistor_v(x, y1, y2, label):
    m = (y1 + y2) / 2
    A(f'<rect x="{x-11}" y="{m-34}" width="22" height="68" rx="5" fill="#f3e3c3" stroke="#6b4f1d" stroke-width="2"/>')
    for i, c in enumerate(BANDS):
        A(f'<rect x="{x-11}" y="{m-24 + i*13}" width="22" height="6" fill="{c}"/>')
    text(x + 20, m + 5, label, 14, "#6b4f1d", "start", True)


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
text(30, 40, "การต่อหัววัดอุณหภูมิ NTC กับ ESP32 (โค้ด temp_test)", 22, bold=True)
text(30, 64, "NTC 10k ต่อกับตัวต้านทาน 4.7k เป็นตัวแบ่งแรงดันจากไฟ 3V3 · น้ำร้อนขึ้น แรงดันที่ D32 ลดลง", 13.5, MUTED)

# ---------- บอร์ดขยาย ESP32 ----------
BX = 680
A(f'<rect x="{BX}" y="92" width="210" height="600" rx="12" fill="#1d2f3a"/>')
text(BX + 105, 124, "บอร์ดขยาย ESP32", 15, "#ffffff", "middle", True)
text(BX + 105, 150, "ใช้ช่อง S เท่านั้น ห้ามใช้ช่อง V", 12, "#f0b4ac", "middle", True)
pins = [("3V3", 200), ("S ของ D32", 290), ("G", 460),
        ("S ของ D21", 555), ("S ของ D22", 585), ("VIN (5V)", 615), ("G", 645)]
for lab, y in pins:
    pin(BX, y)
    text(BX + 18, y + 5, lab, 14, "#ffffff")
text(BX + 105, 360, "ช่องอื่นที่ต่อไว้แล้ว", 12, "#9fc3cc", "middle")
text(BX + 105, 378, "TDS (D25 D26 D33) · pH (D35)", 12, "#9fc3cc", "middle")
text(BX + 105, 396, "ปุ่ม (D15) ต่อค้างไว้ได้", 12, "#9fc3cc", "middle")

# ---------- 3V3 -> 4.7k -> จุด T ----------
NX, NY = 470, 290      # จุด T
line([(BX - 7, 200), (NX, 200), (NX, NY)], RED)
resistor_v(NX, 200, 290, "4.7kΩ")
text(590, 192, "สายแดง → 3V3 (ห้ามใช้ 5V)", 12, RED, "middle")

# จุด T -> D32
line([(NX, NY), (BX - 7, NY)], GREEN)
text(590, 282, "สายเขียว → D32", 12, GREEN, "middle")

# จุด T -> 10µF + 104 -> รางกราวด์
GY = 460
line([(NX, NY), (NX, 330), (600, 330)], INK, 3)
line([(540, 330), (540, GY)], INK, 3)
line([(600, 330), (600, GY)], INK, 3)
electrolytic(540, 330, GY)
ceramic(600, 330, GY)
dot(540, 330, INK, 5)
dot(NX, NY)
A(f'<rect x="{NX-118}" y="{NY-42}" width="76" height="26" rx="13" fill="#fff4cc" stroke="#d4a017"/>')
text(NX - 80, NY - 24, "จุด T", 14, INK, "middle", True)

# รางกราวด์
line([(300, GY), (BX - 7, GY)], BLACK, 4)
dot(540, GY, INK, 5)
dot(600, GY, INK, 5)
text(450, 482, "รางกราวด์ → G", 12, BLACK, "middle")

# ---------- หัววัด NTC ----------
# แก้วน้ำ
A('<path d="M35 545 L45 680 L125 680 L135 545" fill="#e3f3f8" stroke="#8fb8c6" stroke-width="2.5"/>')
A('<rect x="38" y="585" width="94" height="93" fill="#bfe3ef" opacity="0.7"/>')
# หัวสแตนเลส
A('<rect x="70" y="520" width="30" height="130" rx="10" fill="#c7cdd0" stroke="#7d878b" stroke-width="2"/>')
text(112, 500, "หัววัด NTC 10k", 14, INK, bold=True)
text(112, 520, "(หัวสแตนเลส) จุ่มน้ำ", 12.5, MUTED)
# สายหัววัด
line([(85, 520), (85, 400), (110, 380)], "#3a3a3a", 7)
dot(110, 380, "#3a3a3a", 6)
# สาย 1 -> จุด T
line([(110, 380), (110, NY), (NX, NY)], ORANGE)
text(122, 340, "สาย 1", 13, ORANGE, bold=True)
# สาย 2 -> G
line([(110, 380), (300, 380), (300, GY)], BLACK)
text(200, 372, "สาย 2", 13, BLACK, "middle", True)

# ---------- จอ LCD ----------
A('<rect x="220" y="540" width="250" height="120" rx="8" fill="#eef4f5" stroke="#0b7480" stroke-width="2"/>')
text(345, 605, "จอ LCD 20×4 (I2C)", 15, INK, "middle", True)
for lab, y, col in [("SDA", 555, BLUE), ("SCL", 585, PURPLE), ("VCC", 615, RED), ("GND", 645, BLACK)]:
    pin(470, y)
    text(456, y + 5, lab, 13, INK, "end", True)
    line([(477, y), (BX - 7, y)], col, 3)

# ---------- คำอธิบาย ----------
notes = [
    ("1) 3V3 → ตัวต้านทาน 4.7kΩ → จุด T · จุด T → สายไป S ของ D32   (ต้องใช้ 3V3 ห้ามใช้ VIN/5V)", True),
    ("2) NTC สาย 1 → จุด T · สาย 2 → G   (สลับสายได้ ไม่มีขั้ว)", True),
    ("3) 10µF: ขายาว (+) ต่อจุด T, ขาข้างแถบขาว (−) ต่อ G · 104: ไม่มีขั้ว ต่อจุด T กับ G", True),
    ("อัปโหลดโค้ด temp_test · ค่าอัปเดตทุก 1 วินาที · กำหัววัดไว้ในมือ ค่าต้องค่อยๆ ขึ้น", False),
    ("โค้ด tds_ac อ่านอุณหภูมิจาก D32 อัตโนมัติ ต่อ NTC แล้วจอ TDS จะขึ้นอุณหภูมิจริงแทน 25(set)", False),
]
for i, (n, b) in enumerate(notes):
    text(30, 725 + i * 23, n, 13.5, INK if b else MUTED, bold=b)
A("</svg>")

OUT.write_text("\n".join(out), encoding="utf-8")
print("สร้าง", OUT)
