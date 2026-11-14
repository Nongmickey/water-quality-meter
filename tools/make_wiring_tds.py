#!/usr/bin/env python3
"""วาดภาพการต่อเซนเซอร์ TDS แบบบัดกรี (ไม่ใช้เบรดบอร์ด) -> docs/wiring-tds.svg

วิธีใช้:  python3 tools/make_wiring_tds.py
"""
from pathlib import Path

OUT = Path(__file__).resolve().parent.parent / "docs" / "wiring-tds.svg"
W, H = 900, 640
INK, MUTED, TEAL = "#102a32", "#587480", "#0b7480"
RED, BLACK, YELLOW, SIG, CAP = "#d64545", "#2b2b2b", "#d4a017", "#0b8a96", "#1d5fa8"

out = []
A = out.append


def line(pts, color, w=3):
    A('<polyline points="' + " ".join(f"{x},{y}" for x, y in pts) +
      f'" fill="none" stroke="{color}" stroke-width="{w}" stroke-linejoin="round"/>')


def dot(x, y, c=INK):
    A(f'<circle cx="{x}" cy="{y}" r="6" fill="{c}"/>')


def text(x, y, t, size=14, fill=INK, anchor="start", bold=False):
    b = ' font-weight="bold"' if bold else ""
    A(f'<text x="{x}" y="{y}" font-size="{size}" fill="{fill}" text-anchor="{anchor}"{b}>{t}</text>')


def pin(x, y):
    A(f'<rect x="{x-6}" y="{y-6}" width="12" height="12" fill="#d9c27a" stroke="#8a7330"/>')


def resistor(x1, x2, y, label):
    m = (x1 + x2) / 2
    A(f'<rect x="{m-30}" y="{y-10}" width="60" height="20" rx="4" fill="#f3e3c3" stroke="#6b4f1d" stroke-width="2"/>')
    for i, c in enumerate(["#e0b400", "#7b3fa0", "#c62828", "#c9a227"]):   # เหลือง ม่วง แดง ทอง
        A(f'<rect x="{m-20 + i*11}" y="{y-10}" width="5" height="20" fill="{c}"/>')
    text(m, y - 18, label, 14, "#6b4f1d", "middle", True)


def cap_v(x, y1, y2, label, polar=True):
    m = (y1 + y2) / 2
    line([(x, y1), (x, m - 5)], BLACK, 2.5)
    line([(x, m + 5), (x, y2)], BLACK, 2.5)
    A(f'<rect x="{x-14}" y="{m-8}" width="28" height="4" fill="{CAP}"/>')
    A(f'<rect x="{x-14}" y="{m+4}" width="28" height="4" fill="{CAP}"/>')
    if polar:
        text(x - 18, m - 6, "+", 14, CAP, "end", True)
    text(x + 20, m + 5, label, 13, CAP, bold=True)


A(f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" width="{W}" height="{H}" '
  "font-family=\"Tahoma, 'Noto Sans Thai', sans-serif\">")
A(f'<rect width="{W}" height="{H}" fill="#ffffff"/>')
text(30, 40, "การต่อเซนเซอร์ TDS (บัดกรี ไม่ใช้เบรดบอร์ด)", 22, bold=True)
text(30, 64, "สายเหลือง (A) ผ่านตัวต้านทาน 4.7kΩ และตัวเก็บประจุกรองสัญญาณ ก่อนเข้าขา D34", 13.5, MUTED)

# ---- โมดูล TDS ----
A(f'<rect x="40" y="110" width="220" height="250" rx="10" fill="#eef4f5" stroke="{TEAL}" stroke-width="2"/>')
text(150, 140, "TDS Meter V1.0", 16, bold=True, anchor="middle")
for lab, y in [("−", 180), ("+", 240), ("A", 300)]:
    pin(260, y)
    text(280, y - 10, lab, 16, INK, "start", True)
# 100µF หลังบอร์ด คร่อม + กับ −
cap_v(160, 180, 240, "", polar=False)
line([(160, 180), (254, 180)], BLACK, 2.5)
line([(160, 240), (254, 240)], RED, 2.5)
text(180, 230, "+", 14, CAP, "start", True)
text(140, 204, "100µF", 13, CAP, "end", True)
text(140, 222, "บัดกรีหลังบอร์ด", 12, CAP, "end")
# ช่องหัววัด + หัววัด
A('<rect x="40" y="260" width="34" height="44" rx="4" fill="#ffffff" stroke="#9aa9ad" stroke-width="2"/>')
text(84, 278, "ช่องหัววัด", 12, MUTED)
text(84, 294, "(ขั้วขาว 2 ขา)", 12, MUTED)
line([(57, 304), (57, 400)], "#9aa9ad", 5)
A('<rect x="44" y="400" width="26" height="90" rx="6" fill="#c9d3d6" stroke="#7f9196" stroke-width="2"/>')
text(84, 440, "หัววัด TDS", 13, INK, bold=True)
text(84, 458, "จุ่มลงน้ำ", 12, MUTED)

# ---- บอร์ดขยาย ESP32 ----
BX = 660
A(f'<rect x="{BX}" y="110" width="210" height="370" rx="10" fill="#1d2f3a"/>')
text(BX + 105, 140, "บอร์ดขยาย ESP32", 15, "#ffffff", "middle", True)
for lab, y in [("G (ช่องไหนก็ได้)", 180), ("3V3", 240), ("S ของ D34", 300), ("V ของ D34", 360), ("G ของ D34", 420)]:
    pin(BX, y)
    col = "#7f8c91" if lab.startswith("V") else "#ffffff"
    text(BX + 16, y + 5, lab, 14, col)
text(BX + 16, 382, "ห้ามใช้", 12, "#f08a7e", bold=True)

# ---- สาย ----
line([(266, 180), (BX - 6, 180)], BLACK)                      # − → G
line([(266, 240), (BX - 6, 240)], RED)                        # + → 3V3
line([(266, 300), (330, 300)], YELLOW)                        # A → TDS-1
resistor(330, 410, 300, "4.7kΩ")
line([(330, 300), (360, 300)], YELLOW)
line([(400, 300), (BX - 6, 300)], SIG)                        # TDS-2 → S ของ D34
# ตัวเก็บประจุ 10µF และ 104 จาก TDS-2 ลง TDS-3
line([(470, 300), (470, 330)], SIG, 2.5)
line([(560, 300), (560, 330)], SIG, 2.5)
cap_v(470, 330, 410, "10µF")
cap_v(560, 330, 410, "104", polar=False)
line([(470, 410), (470, 420), (BX - 6, 420)], BLACK)        # TDS-3 → G ของ D34
line([(560, 410), (560, 420)], BLACK, 2.5)
dot(470, 300); dot(560, 300); dot(470, 420); dot(560, 420)
dot(330, 300, YELLOW)

# ป้ายจุดต่อ
for lab, x, y in [("TDS-1", 330, 336), ("TDS-2", 515, 288), ("TDS-3", 515, 446)]:
    A(f'<rect x="{x-30}" y="{y-15}" width="60" height="22" rx="11" fill="#fff4cc" stroke="#d4a017"/>')
    text(x, y + 1, lab, 12.5, INK, "middle", True)
text(BX - 14, 230, "ใช้ร่วมกับหัววัดอุณหภูมิ (จุด NTC-1)", 12, RED, "end")

# ---- คำอธิบาย ----
notes = [
    "TDS-1 = สาย A (เหลือง) + ขาหนึ่งของ 4.7kΩ",
    "TDS-2 = อีกขาของ 4.7kΩ + ขา + (ขายาว) ของ 10µF + ขาหนึ่งของ 104 + สายไป S ของ D34",
    "TDS-3 = ขา − (ฝั่งแถบสี) ของ 10µF + อีกขาของ 104 + สายไป G ของ D34",
    "บัดกรีทุกจุดแล้วหุ้มท่อหด · 4.7kΩ = เหลือง-ม่วง-แดง-ทอง · 104 ไม่มีขั้ว · 100µF ขา + ลงขา + ของบอร์ด TDS",
]
for i, n in enumerate(notes):
    text(30, 540 + i * 24, n, 13.5, INK if i < 3 else MUTED, bold=(i < 3))
A("</svg>")

OUT.write_text("\n".join(out), encoding="utf-8")
print("สร้าง", OUT)
