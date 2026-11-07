#!/usr/bin/env python3
"""รวมไฟล์หน้าเว็บ (web/index.html + style.css + model.js + app.js) เป็นไฟล์เดียว
แล้วเขียนเป็น firmware/water_quality_meter/web_page.h ให้ ESP32 ใช้

วิธีใช้:
    python3 tools/build_web.py              สร้าง web_page.h
    python3 tools/build_web.py --html OUT   สร้างหน้าเว็บไฟล์เดียว (ไว้เปิด/แชร์) ด้วย

ใน VS Code: กด Ctrl+Shift+B (งาน "สร้างหน้าเว็บให้ ESP32")
ต้องรันทุกครั้งหลังแก้ไฟล์ในโฟลเดอร์ web/ แล้วค่อยอัปโหลดใน Arduino IDE
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
WEB = ROOT / "web"
OUT = ROOT / "firmware" / "water_quality_meter" / "web_page.h"
DELIM = "WQMHTML"


def bundle() -> str:
    html = (WEB / "index.html").read_text(encoding="utf-8")

    def css(m):
        return "<style>\n" + (WEB / m.group(1)).read_text(encoding="utf-8") + "</style>"

    def js(m):
        code = (WEB / m.group(1)).read_text(encoding="utf-8")
        if "</script" in code.lower():
            raise SystemExit(f"{m.group(1)} มีข้อความ </script ซึ่งจะทำให้หน้าเว็บพัง")
        return "<script>\n" + code + "</script>"

    html = re.sub(r'<link rel="stylesheet" href="([^"]+)">', css, html)
    html = re.sub(r'<script src="([^"]+)"></script>', js, html)
    return html


def build() -> Path:
    page = bundle()
    if f'){DELIM}"' in page:
        raise SystemExit(f'หน้าเว็บมีข้อความ ){DELIM}" ซึ่งชนกับตัวคั่น')
    OUT.write_text(
        "// ไฟล์นี้สร้างอัตโนมัติจากโฟลเดอร์ web/ ด้วย tools/build_web.py — อย่าแก้ไฟล์นี้โดยตรง\n"
        "#pragma once\n#include <pgmspace.h>\n\n"
        f'const char INDEX_HTML[] PROGMEM = R"{DELIM}(' + page + f'){DELIM}";\n',
        encoding="utf-8")
    print(f"สร้าง {OUT.relative_to(ROOT)} แล้ว ({len(page.encode('utf-8')):,} ไบต์)")
    return OUT


if __name__ == "__main__":
    build()
    if "--html" in sys.argv:
        out = Path(sys.argv[sys.argv.index("--html") + 1])
        out.write_text(bundle(), encoding="utf-8")
        print("สร้าง", out)
