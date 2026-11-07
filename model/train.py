#!/usr/bin/env python3
"""เทรนโมเดลประเมินแหล่งน้ำ (Gaussian Naive Bayes) จากข้อมูลที่วัดจริง

ขั้นตอน
  1) วัดน้ำที่รู้แหล่งที่มาแน่นอน (เช่น น้ำประปาที่บ้าน น้ำบ่อ น้ำฝน) หลาย ๆ ครั้ง
  2) ในหน้าเว็บ ช่อง "แหล่งน้ำจริง" ให้เลือกแหล่งที่ถูกต้อง แล้วกด "ส่งออก CSV"
  3) บันทึกเป็นไฟล์ เช่น model/samples.csv แล้วรัน
         python3 model/train.py model/samples.csv
     (ใส่ได้หลายไฟล์)
  4) สคริปต์จะ
       - คำนวณโมเดลใหม่ โดยเริ่มจากค่าอ้างอิงใน reference_model.json แล้วปรับด้วยข้อมูลจริง
       - ทดสอบความแม่นยำแบบ leave-one-out (เอาตัวอย่างออกทีละตัวแล้วทาย)
       - เขียน model/model.json และอัปเดต web/model.js + firmware/web_page.h
  5) อัปโหลดโค้ดลง ESP32 ใหม่

ใช้แค่ Python มาตรฐาน ไม่ต้องติดตั้งอะไรเพิ่ม
"""
import csv
import json
import math
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
REF = ROOT / "model" / "reference_model.json"
OUT = ROOT / "model" / "model.json"
WEB = ROOT / "web" / "model.js"

PRIOR_WEIGHT = 3.0          # ค่าอ้างอิงมีน้ำหนักเท่ากับข้อมูลจริงกี่ตัวอย่าง
MIN_STD = [0.08, 0.04, 0.5]  # ค่า std ต่ำสุด กันโมเดลมั่นใจเกินจริง


def feats(row):
    def num(k):
        v = (row.get(k) or "").strip()
        try:
            return float(v)
        except ValueError:
            return None
    ph, tds, temp = num("ph"), num("tds"), num("temp")
    return [ph, math.log10(max(tds, 1.0)) if tds is not None else None, temp]


def load(paths):
    rows = []
    for p in paths:
        with open(p, encoding="utf-8-sig", newline="") as fh:
            for r in csv.DictReader(fh):
                label = (r.get("source_true") or "").strip()
                if label:
                    rows.append((feats(r), label))
    return rows


def fit(ref, rows):
    model = json.loads(json.dumps(ref))
    for c in model["classes"]:
        xs = [x for x, y in rows if y == c["id"]]
        c["n"] = len(xs)
        for i in range(3):
            vals = [x[i] for x in xs if x[i] is not None]
            n = len(vals)
            if n == 0:
                continue
            m0, s0 = c["mean"][i], c["std"][i]
            mean = (sum(vals) + PRIOR_WEIGHT * m0) / (n + PRIOR_WEIGHT)
            ss = sum((v - mean) ** 2 for v in vals) + PRIOR_WEIGHT * (s0 ** 2 + (m0 - mean) ** 2)
            std = math.sqrt(ss / (n + PRIOR_WEIGHT))
            c["mean"][i] = round(mean, 4)
            c["std"][i] = round(max(std, MIN_STD[i]), 4)
    return model


def log_pdf(x, m, s):
    z = (x - m) / s
    return -0.5 * z * z - math.log(s)


def predict(model, x):
    best, best_lp = None, -1e300
    for c in model["classes"]:
        lp = math.log(c.get("prior", 1))
        for i in range(3):
            if x[i] is not None:
                lp += log_pdf(x[i], c["mean"][i], c["std"][i])
        if lp > best_lp:
            best, best_lp = c["id"], lp
    return best


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    ref = json.loads(REF.read_text(encoding="utf-8"))
    ids = {c["id"] for c in ref["classes"]}
    rows = load(sys.argv[1:])
    unknown = sorted({y for _, y in rows if y not in ids})
    if unknown:
        print("ข้ามป้ายกำกับที่ไม่รู้จัก:", ", ".join(unknown))
        rows = [(x, y) for x, y in rows if y in ids]
    if not rows:
        sys.exit("ไม่พบแถวที่มี source_true — เลือก 'แหล่งน้ำจริง' ในหน้าเว็บก่อนส่งออก CSV")

    # ทดสอบแบบ leave-one-out
    correct = 0
    for i, (x, y) in enumerate(rows):
        m = fit(ref, rows[:i] + rows[i + 1:])
        correct += predict(m, x) == y
    acc = correct / len(rows)

    model = fit(ref, rows)
    model["version"] = f"trained-{len(rows)}"
    model["trained_on"] = f"ข้อมูลวัดจริง {len(rows)} ตัวอย่าง + ค่าอ้างอิง"
    model["accuracy"] = round(acc, 4)
    model["samples"] = len(rows)
    OUT.write_text(json.dumps(model, ensure_ascii=False, indent=2), encoding="utf-8")

    print(f"ตัวอย่างที่ใช้เทรน: {len(rows)}")
    for c in model["classes"]:
        print(f"  {c['name']:<34} {c['n']:>4} ตัวอย่าง")
    print(f"ความแม่นยำ (leave-one-out): {acc:.1%}")

    html = WEB.read_text(encoding="utf-8")
    block = json.dumps(model, ensure_ascii=False, indent=2)
    html, k = re.subn(r"/\*MODEL_START\*/.*?/\*MODEL_END\*/",
                      lambda _: "/*MODEL_START*/" + block + "/*MODEL_END*/", html, flags=re.S)
    if k != 1:
        sys.exit("หา MODEL_START/MODEL_END ใน web/model.js ไม่เจอ")
    WEB.write_text(html, encoding="utf-8")
    print("อัปเดตโมเดลใน web/model.js แล้ว")

    sys.path.insert(0, str(ROOT / "tools"))
    import build_web
    build_web.build()
    print("เสร็จ! อัปโหลดโค้ดลง ESP32 ใหม่เพื่อใช้โมเดลนี้")


if __name__ == "__main__":
    main()
