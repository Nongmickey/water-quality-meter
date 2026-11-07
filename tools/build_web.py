#!/usr/bin/env python3
"""สร้าง firmware/water_quality_meter/web_page.h จาก web/index.html

วิธีใช้:  python3 tools/build_web.py
ต้องรันทุกครั้งหลังแก้ web/index.html หรือหลังเทรนโมเดลใหม่ (train.py รันให้อัตโนมัติ)
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "web" / "index.html"
OUT = ROOT / "firmware" / "water_quality_meter" / "web_page.h"
DELIM = "WQMHTML"

HEAD = ('<!doctype html>\n<html lang="th">\n<head>\n<meta charset="utf-8">\n'
        '<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">\n')


def build() -> Path:
    body = SRC.read_text(encoding="utf-8")
    page = HEAD + body + "\n</html>\n"
    if f'){DELIM}"' in page:
        raise SystemExit(f"หน้าเว็บมีข้อความ ){DELIM}\" ซึ่งชนกับตัวคั่น")
    header = (
        "// ไฟล์นี้สร้างอัตโนมัติจาก web/index.html ด้วย tools/build_web.py — อย่าแก้ไฟล์นี้โดยตรง\n"
        "#pragma once\n#include <pgmspace.h>\n\n"
        f'const char INDEX_HTML[] PROGMEM = R"{DELIM}(' + page + f'){DELIM}";\n'
    )
    OUT.write_text(header, encoding="utf-8")
    print(f"สร้าง {OUT.relative_to(ROOT)} แล้ว ({len(page.encode('utf-8')):,} ไบต์)")
    return OUT


if __name__ == "__main__":
    build()
