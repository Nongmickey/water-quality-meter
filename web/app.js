
/* ===== คณิตศาสตร์ ===== */
function erfc(x) { // Abramowitz & Stegun 7.1.26
  const z = Math.abs(x), t = 1 / (1 + 0.3275911 * z);
  const y = t * (0.254829592 + t * (-0.284496736 + t * (1.421413741 + t * (-1.453152027 + t * 1.061405429)))) * Math.exp(-z * z);
  return x >= 0 ? y : 2 - y;
}
function logPdf(x, m, s) { const z = (x - m) / s; return -0.5 * z * z - Math.log(s) - 0.9189385; }
function logSurvival(x, m, s) { return Math.log(Math.max(1e-300, 0.5 * erfc((x - m) / (s * Math.SQRT2)))); }
function logCdf(x, m, s) { return Math.log(Math.max(1e-300, 0.5 * erfc(-(x - m) / (s * Math.SQRT2)))); }
// ESP32 อ่าน TDS ต่ำกว่าประมาณ 60 ppm ไม่ได้ (เครื่องส่งมาเป็น 0) จึงถือว่า "ต่ำกว่า 60"
const tdsMin = () => MODEL.tds_min || 60;
const tdsUnder = v => has(v) && v < tdsMin();
const has = v => typeof v === "number" && isFinite(v);

/* ===== ประเมินแหล่งน้ำ ===== */
function classify(v) {
  if (!has(v.ph) && !has(v.tds)) return null;
  const tdsMax = MODEL.tds_max || 1000;
  const overRange = has(v.tds) && v.tds >= tdsMax * 0.99;
  const underRange = tdsUnder(v.tds);
  const x = [v.ph, has(v.tds) ? Math.log10(Math.max(v.tds, 1)) : null, v.temp];
  const scores = MODEL.classes.map(c => {
    let lp = Math.log(c.prior || 1);
    for (let i = 0; i < 3; i++) {
      if (!has(x[i])) continue;
      if (i === 1 && overRange) lp += logSurvival(Math.log10(tdsMax * 0.99), c.mean[1], c.std[1]);
      else if (i === 1 && underRange) lp += logCdf(Math.log10(tdsMin()), c.mean[1], c.std[1]);
      else lp += logPdf(x[i], c.mean[i], c.std[i]);
    }
    return { c, lp };
  });
  const mx = Math.max(...scores.map(s => s.lp));
  const sum = scores.reduce((a, s) => a + Math.exp(s.lp - mx), 0);
  return scores.map(s => ({ id: s.c.id, name: s.c.name, clue: s.c.clue, p: Math.exp(s.lp - mx) / sum }))
               .sort((a, b) => b.p - a.p);
}

/* ===== วิเคราะห์คุณภาพน้ำ ===== */
const ORDER = { good: 0, warn: 1, bad: 2 };
function worst(list) { return list.reduce((w, s) => (ORDER[s] > ORDER[w] ? s : w), "good"); }

function analyzePh(ph) {
  if (!has(ph)) return { st: "none", label: "ไม่ได้วัด", score: null, desc: "ต่อหัววัด pH แล้วกดปุ่มเขียวอีกครั้ง" };
  let st, label;
  if (ph >= 6.5 && ph <= 8.5) { st = "good"; label = "อยู่ในเกณฑ์"; }
  else if (ph >= 6.0 && ph < 6.5) { st = "warn"; label = "ค่อนข้างเป็นกรด"; }
  else if (ph > 8.5 && ph <= 9.0) { st = "warn"; label = "ค่อนข้างเป็นด่าง"; }
  else if (ph < 6.0) { st = "bad"; label = "เป็นกรด"; }
  else { st = "bad"; label = "เป็นด่าง"; }
  const dist = ph < 6.5 ? 6.5 - ph : ph > 8.5 ? ph - 8.5 : 0;
  const score = Math.max(0, 100 - 40 * dist);
  const desc = st === "good" ? "pH 6.5–8.5 ตามเกณฑ์น้ำดื่มของกระทรวงสาธารณสุข"
    : ph < 6.5 ? "น้ำเป็นกรด อาจกัดกร่อนท่อโลหะ และมีรสเปรี้ยวเล็กน้อย เกณฑ์น้ำดื่มคือ 6.5–8.5"
    : "น้ำเป็นด่าง อาจมีรสขม ทำให้เกิดคราบตะกรัน เกณฑ์น้ำดื่มคือ 6.5–8.5";
  return { st, label, score, desc };
}

function analyzeTds(tds) {
  if (!has(tds)) return { st: "none", label: "ไม่ได้วัด", score: null, desc: "จุ่มหัววัด TDS ลงในน้ำแล้วกดปุ่มเขียวอีกครั้ง" };
  const over = tds >= (MODEL.tds_max || 1000) * 0.99;
  let st, label, score, desc;
  if (tds < 50) { st = "good"; label = "แร่ธาตุน้อยมาก"; score = 85; desc = "น้ำบริสุทธิ์มาก ดื่มได้ แต่แทบไม่มีแร่ธาตุ (แบบน้ำ RO หรือน้ำฝน)"; }
  else if (tds <= 300) { st = "good"; label = "ดีมาก"; score = 100; desc = "ระดับที่ WHO ถือว่ารสชาติดีมาก (ต่ำกว่า 300 mg/L)"; }
  else if (tds <= 500) { st = "good"; label = "ดี"; score = 90; desc = "ไม่เกิน 500 mg/L ผ่านเกณฑ์น้ำดื่มบรรจุขวด"; }
  else if (tds <= 600) { st = "warn"; label = "ดีพอใช้"; score = 80; desc = "เกินเกณฑ์น้ำดื่มบรรจุขวด (500) เล็กน้อย แต่ WHO ยังถือว่าอยู่ในระดับดี"; }
  else if (tds <= 900) { st = "warn"; label = "พอใช้"; score = 60; desc = "แร่ธาตุค่อนข้างมาก อาจมีรสกร่อยหรือเกิดตะกรัน"; }
  else if (tds <= 1200) { st = "bad"; label = "ไม่ดี"; score = 35; desc = "สารละลายสูง ไม่เหมาะดื่ม ควรกรองด้วยระบบ RO"; }
  else { st = "bad"; label = "แย่"; score = 15; desc = "สารละลายสูงมาก ไม่ควรดื่ม"; }
  const under = tdsUnder(tds);
  if (under) { label = "ต่ำกว่า " + tdsMin() + " ppm"; desc = "ต่ำกว่าค่าต่ำสุดที่เครื่องวัดได้ น้ำมีแร่ธาตุน้อยมาก เช่น น้ำ RO น้ำกลั่น น้ำฝน หรือน้ำดื่มบางยี่ห้อ (หรือหัววัดไม่ได้จุ่มน้ำ)"; }
  if (over) { label = "เกินช่วงวัด"; desc = "ค่าเกิน 1,000 ppm ซึ่งเป็นขีดสูงสุดของเซนเซอร์ ค่าจริงอาจสูงกว่านี้มาก " + desc; st = "bad"; score = 15; }
  return { st, label, score, desc, ec: tds / 0.5, over, under };
}

function analyzeTemp(t) {
  if (!has(t)) return { st: "none", label: "ไม่ได้วัด", desc: "ระบบใช้ 25 °C ในการคำนวณ TDS แทน ค่า TDS อาจคลาดเคลื่อนเล็กน้อย" };
  if (t < 15) return { st: "warn", label: "เย็นมาก", desc: "น้ำเย็นจัด อาจเป็นน้ำแช่เย็นหรือเพิ่งผสมน้ำแข็ง" };
  if (t < 25) return { st: "good", label: "เย็น", desc: "เย็นกว่าอุณหภูมิห้องในไทย" };
  if (t <= 32) return { st: "good", label: "ปกติ", desc: "ใกล้อุณหภูมิห้องในไทย" };
  if (t <= 35) return { st: "warn", label: "ค่อนข้างอุ่น", desc: "น้ำอุ่นละลายออกซิเจนได้น้อยลง ส่งผลต่อสัตว์น้ำ" };
  return { st: "warn", label: "ร้อน", desc: "น้ำร้อน ออกซิเจนในน้ำต่ำ ไม่เหมาะกับสัตว์น้ำ" };
}

function grade(score) {
  if (score === null) return { st: "none", text: "ข้อมูลไม่พอประเมิน" };
  if (score >= 90) return { st: "good", text: "คุณภาพดีมาก" };
  if (score >= 75) return { st: "good", text: "คุณภาพดี" };
  if (score >= 55) return { st: "warn", text: "คุณภาพพอใช้" };
  if (score >= 35) return { st: "bad", text: "ควรปรับปรุงก่อนใช้" };
  return { st: "bad", text: "ไม่เหมาะกับการใช้งาน" };
}

function band(x, good, maybe) { // good/maybe = [min,max]
  if (x >= good[0] && x <= good[1]) return "good";
  if (x >= maybe[0] && x <= maybe[1]) return "warn";
  return "bad";
}

function suitability(v) {
  const out = [];
  const partialNote = " (บางค่าไม่ได้วัด)";
  // ดื่ม
  if (!has(v.ph) || !has(v.tds)) out.push({ what: "ดื่ม", st: "none", note: "ต้องวัดทั้ง pH และ TDS ถึงจะประเมินได้" });
  else {
    const st = worst([band(v.ph, [6.5, 8.5], [6.0, 9.0]), band(v.tds, [0, 500], [0, 1000])]);
    out.push({ what: "ดื่ม", st, note: st === "good" ? "ผ่านเกณฑ์เบื้องต้น แต่ยังตรวจเชื้อโรคไม่ได้ ควรต้มหรือกรองก่อนดื่ม"
      : st === "warn" ? "ดื่มได้บ้าง แต่ไม่แนะนำให้ดื่มประจำ ควรกรองก่อน" : "ไม่ควรดื่ม" });
  }
  // อุปโภค
  {
    const parts = [];
    if (has(v.ph)) parts.push(band(v.ph, [6.0, 9.5], [5.0, 10.5]));
    if (has(v.tds)) parts.push(band(v.tds, [0, 1500], [0, 3000]));
    if (!parts.length) out.push({ what: "ใช้ในบ้าน", st: "none", note: "ไม่มีค่าที่ใช้ประเมิน" });
    else {
      const st = worst(parts);
      out.push({ what: "ใช้ในบ้าน", st, note: (st === "good" ? "อาบน้ำ ซักผ้า ล้างจานได้" : st === "warn" ? "ใช้ได้ แต่อาจเกิดคราบหรือตะกรัน" : "ไม่เหมาะ อาจระคายเคืองผิวหรือทำให้อุปกรณ์เสียหาย")
        + (parts.length < 2 ? partialNote : "") });
    }
  }
  // เกษตร (FAO Irrigation and Drainage Paper 29)
  {
    const parts = [];
    if (has(v.tds)) parts.push(v.tds < 450 ? "good" : v.tds <= 2000 ? "warn" : "bad");
    if (has(v.ph)) parts.push(band(v.ph, [6.5, 8.4], [5.5, 9.0]));
    if (!parts.length) out.push({ what: "รดน้ำพืช", st: "none", note: "ไม่มีค่าที่ใช้ประเมิน" });
    else {
      const st = worst(parts);
      out.push({ what: "รดน้ำพืช", st, note: (st === "good" ? "เหมาะกับพืชทั่วไป" : st === "warn" ? "ใช้ได้กับพืชทนเค็ม ควรระวังความเค็มสะสมในดิน" : "ไม่เหมาะ ความเค็มหรือ pH จะทำให้พืชเสียหาย")
        + (parts.length < 2 ? partialNote : "") });
    }
  }
  // เลี้ยงปลาน้ำจืด
  {
    const parts = [];
    if (has(v.ph)) parts.push(band(v.ph, [6.5, 9.0], [6.0, 9.5]));
    if (has(v.tds)) parts.push(v.tds < 990 ? "good" : "warn");
    if (has(v.temp)) parts.push(band(v.temp, [25, 32], [20, 34]));
    if (!parts.length) out.push({ what: "เลี้ยงปลา", st: "none", note: "ไม่มีค่าที่ใช้ประเมิน" });
    else {
      const st = worst(parts);
      out.push({ what: "เลี้ยงปลา", st, note: (st === "good" ? "เหมาะกับปลาน้ำจืดทั่วไป" : st === "warn" ? "ปลาบางชนิดอาจเครียด ควรปรับสภาพน้ำ" : "ไม่เหมาะกับปลาน้ำจืด")
        + (parts.length < 3 ? partialNote : "") });
    }
  }
  return out;
}

function advice(v, a) {
  const tips = [];
  if (has(v.ph) && v.ph < 6.5) tips.push("pH ต่ำ: ถ้าจะดื่มหรือใช้ประจำ ใช้ไส้กรองปรับสภาพกรด (แคลไซต์) หรือผสมน้ำที่ pH สูงกว่า");
  if (has(v.ph) && v.ph > 8.5) tips.push("pH สูง: อาจมีหินปูนหรือสารเคมีปน ควรกรองหรือตรวจหาที่มา");
  if (has(v.tds) && v.tds > 600) tips.push("TDS สูง: ถ้าจะดื่ม ควรผ่านเครื่องกรอง RO ซึ่งลด TDS ได้ประมาณ 90–99%");
  if (has(v.tds) && v.tds < 50) tips.push("TDS ต่ำมาก: ดื่มได้ปลอดภัย แต่ควรได้แร่ธาตุจากอาหารให้เพียงพอ");
  if (a.tds.over) tips.push("ค่า TDS เกินช่วงของเซนเซอร์ ถ้าสงสัยน้ำกร่อย ให้ส่งตรวจค่าความเค็มเพิ่ม");
  if (!has(v.temp)) tips.push("ไม่ได้วัดอุณหภูมิ: ต่อหัววัดอุณหภูมิเพื่อให้ค่า TDS แม่นขึ้น");
  if (!has(v.ph) || !has(v.tds)) tips.push("วัดให้ครบทั้ง 3 ค่า ผลวิเคราะห์และการประเมินแหล่งน้ำจะแม่นยำขึ้น");
  tips.push("ล้างหัววัดด้วยน้ำสะอาดทุกครั้งก่อนย้ายไปวัดน้ำตัวอย่างอื่น");
  return tips;
}

function analyze(v) {
  const a = { ph: analyzePh(v.ph), tds: analyzeTds(v.tds), temp: analyzeTemp(v.temp) };
  const sc = [a.ph.score, a.tds.score].filter(s => s !== null);
  a.score = sc.length ? Math.round(sc.reduce((x, y) => x + y, 0) / sc.length) : null;
  a.grade = grade(a.score);
  a.suit = suitability(v);
  a.source = classify(v);
  a.tips = advice(v, a);
  return a;
}

/* ===== แสดงผล ===== */
const $ = id => document.getElementById(id);
const esc = s => String(s).replace(/[&<>"]/g, c => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[c]));
const f = (v, d) => (has(v) ? v.toFixed(d) : "-");
const fTds = v => (tdsUnder(v) ? "<" + tdsMin() : f(v, 0));
const ST_TEXT = { good: "เหมาะ", warn: "ใช้ได้บางส่วน", bad: "ไม่เหมาะ", none: "ไม่ทราบ" };

function fmtTime(ms) {
  try { return new Date(ms).toLocaleString("th-TH", { dateStyle: "long", timeStyle: "medium" }); }
  catch (e) { return new Date(ms).toLocaleString(); }
}

function scaleHtml(kind, value) {
  if (kind === "ph") {
    const pos = has(value) ? (value / 14) * 100 : null;
    return `<div class="scale" aria-hidden="true"><div class="bar ph-bar"></div>
      <div class="ok" style="left:${6.5 / 14 * 100}%;width:${2 / 14 * 100}%"></div>
      ${pos !== null ? `<div class="mark" style="left:${pos}%"></div>` : ""}
      <div class="ticks"><span style="left:0">0 กรด</span><span style="left:50%">7</span><span style="left:100%">14 ด่าง</span></div></div>`;
  }
  // TDS: สเกลลอการิทึม 1–2000 ppm
  const lp = x => (Math.log10(Math.max(x, 1)) / Math.log10(2000)) * 100;
  const pos = has(value) ? Math.min(100, lp(value)) : null;
  return `<div class="scale" aria-hidden="true"><div class="bar tds-bar"></div>
    <div class="ok" style="left:0;width:${lp(500)}%"></div>
    ${pos !== null ? `<div class="mark" style="left:${pos}%"></div>` : ""}
    <div class="ticks"><span style="left:0">1</span><span style="left:${lp(50)}%">50</span><span style="left:${lp(500)}%">500</span><span style="left:100%">2000 ppm</span></div></div>`;
}

function renderReport(rec) {
  const v = rec.values, a = analyze(v);
  const src = a.source;
  let srcHtml;
  if (!src) srcHtml = `<p class="muted" style="margin:0">ต้องมีค่า pH หรือ TDS อย่างน้อย 1 ค่า ถึงจะประเมินแหล่งน้ำได้</p>`;
  else {
    const top = src[0];
    const unsure = top.p < 0.45;
    srcHtml = `<div class="guess"><b>${esc(top.name)}</b><span class="pill ${unsure ? "s-warn" : "s-good"}">มั่นใจ ${Math.round(top.p * 100)}%</span></div>
      <p style="margin:0">${esc(top.clue)}</p>
      ${unsure ? `<p class="hint" style="margin:0">ค่าที่วัดได้ใกล้เคียงกับหลายแหล่ง จึงยังระบุได้ไม่ชัด ถ้ามีกลิ่นคลอรีนมักเป็นน้ำประปา ถ้าขุ่นมักเป็นน้ำผิวดิน</p>` : ""}
      <div class="bars">${src.slice(0, 4).map(s => `<div><span class="nm">${esc(s.name)}</span><span class="track"><span class="fill" style="display:block;width:${(s.p * 100).toFixed(1)}%"></span></span><span class="pct">${Math.round(s.p * 100)}%</span></div>`).join("")}</div>`;
  }
  $("report").innerHTML = `
  <article class="slip" aria-live="polite">
    <div class="slip-head">
      <div class="meta">
        <span class="eyebrow">ใบรายงานผล${rec.example ? ` <span class="tag">ตัวอย่าง</span>` : ""}</span>
        <span>🕒 ${esc(fmtTime(rec.time))}${rec.timeSource === "phone" ? " (เวลาจากมือถือ)" : ""}</span>
        <span>📍 ${rec.location ? esc(rec.location) : "ไม่ได้ระบุสถานที่"}${rec.gps ? ` · <a href="https://maps.google.com/?q=${rec.gps.lat},${rec.gps.lon}" target="_blank" rel="noopener">${rec.gps.lat.toFixed(5)}, ${rec.gps.lon.toFixed(5)}</a>` : ""}</span>
      </div>
      <div class="sample-no">ครั้งที่<br>${esc(rec.label)}</div>
    </div>

    <section class="verdict">
      <div class="score s-${a.grade.st}">${a.score === null ? "–" : a.score}<small>/100</small></div>
      <div>
        <h2>${esc(a.grade.text)}</h2>
        <div class="hint">คะแนนรวมจาก pH และ TDS</div>
      </div>
    </section>

    <section>
      <div class="param">
        <div><div class="name">pH ความเป็นกรด-ด่าง</div><span class="pill s-${a.ph.st}">${esc(a.ph.label)}</span></div>
        <div class="val">${f(v.ph, 2)}</div>
        ${scaleHtml("ph", v.ph)}
        <div class="desc">${esc(a.ph.desc)}</div>
      </div>
      <div class="param">
        <div><div class="name">TDS สารที่ละลายในน้ำ</div><span class="pill s-${a.tds.st}">${esc(a.tds.label)}</span></div>
        <div class="val">${fTds(v.tds)} <small>ppm</small></div>
        ${scaleHtml("tds", v.tds)}
        <div class="desc">${esc(a.tds.desc)}${has(v.tds) && !a.tds.under ? ` · ค่าการนำไฟฟ้าโดยประมาณ <span class="num">${Math.round(a.tds.ec)}</span> µS/cm` : ""}</div>
      </div>
      <div class="param">
        <div><div class="name">อุณหภูมิน้ำ</div><span class="pill s-${a.temp.st}">${esc(a.temp.label)}</span></div>
        <div class="val">${f(v.temp, 1)} <small>°C</small></div>
        <div class="desc">${esc(a.temp.desc)}</div>
      </div>
    </section>

    <section>
      <h3>น้ำนี้น่าจะมาจาก</h3>
      ${srcHtml}
    </section>

    <section>
      <h3>เหมาะกับการใช้งาน</h3>
      <div class="suit">${a.suit.map(s => `<div><span class="what">${esc(s.what)}</span><span><span class="pill s-${s.st}">${ST_TEXT[s.st]}</span></span><span class="note">${esc(s.note)}</span></div>`).join("")}</div>
    </section>

    <section>
      <h3>คำแนะนำ</h3>
      <ul class="advice">${a.tips.map(t => `<li>${esc(t)}</li>`).join("")}</ul>
      <p class="fine" style="margin:0">เครื่องนี้วัดได้ 3 ค่า ยังตรวจไม่ได้: เชื้อโรค โลหะหนัก คลอรีน ไนเตรต ถ้าจะใช้น้ำนี้ดื่มประจำ ควรส่งตรวจที่ห้องปฏิบัติการ การประเมินแหล่งน้ำเป็นการคาดคะเนจากค่าที่วัดได้</p>
    </section>
  </article>`;
}

function renderEmpty() {
  $("report").innerHTML = `<div class="slip"><div class="empty">
    <div class="big-btn" aria-hidden="true"></div>
    <h2>กดปุ่มสีเขียวที่เครื่องวัด</h2>
    <p style="margin:0">จุ่มหัววัดลงในน้ำ รอให้ค่าบนจอนิ่งประมาณ 30 วินาที แล้วกดปุ่ม ผลวิเคราะห์จะแสดงที่นี่</p>
    <button id="web-measure">หรือกดวัดจากหน้านี้</button></div></div>`;
  $("web-measure").onclick = webMeasure;
}

/* ===== ประวัติ (เก็บในมือถือ) ===== */
const store = {
  get(k, d) { try { const s = localStorage.getItem(k); return s ? JSON.parse(s) : d; } catch (e) { return d; } },
  set(k, v) { try { localStorage.setItem(k, JSON.stringify(v)); } catch (e) {} }
};
let records = store.get("wqm-history", []);

function renderHistory() {
  const opts = [`<option value="">ไม่ทราบ</option>`].concat(MODEL.classes.map(c => `<option value="${c.id}">${esc(c.name)}</option>`)).join("");
  $("hist-body").innerHTML = records.length ? records.slice().reverse().map(h => {
    const a = analyze(h.values);
    const top = a.source ? a.source[0].name : "-";
    return `<tr><td>${esc(fmtTime(h.time))}</td><td>${esc(h.location || "-")}</td>
      <td class="n">${f(h.values.ph, 2)}</td><td class="n">${fTds(h.values.tds)}</td><td class="n">${f(h.values.temp, 1)}</td>
      <td><span class="pill s-${a.grade.st}">${a.score === null ? "–" : a.score}</span></td><td>${esc(top)}</td>
      <td><select data-key="${esc(h.key)}" aria-label="แหล่งน้ำจริง">${opts.replace(`value="${h.truth || ""}"`, `value="${h.truth || ""}" selected`)}</select></td></tr>`;
  }).join("") : `<tr><td colspan="8" class="muted">ยังไม่มีประวัติ ผลตรวจทุกครั้งจะถูกบันทึกไว้ที่นี่</td></tr>`;
  $("hist-body").querySelectorAll("select").forEach(sel => sel.onchange = () => {
    const h = records.find(x => x.key === sel.dataset.key);
    if (h) { h.truth = sel.value; store.set("wqm-history", records); }
  });
}

function addRecord(rec) {
  if (!rec.example && !records.some(h => h.key === rec.key)) {
    records.push(rec);
    if (records.length > 500) records.shift();
    store.set("wqm-history", records);
  }
  renderReport(rec);
  renderHistory();
}

function toCsv() {
  const rows = [["time_iso", "location", "lat", "lon", "ph", "tds", "temp", "source_guess", "source_true"]];
  records.forEach(h => {
    const a = analyze(h.values);
    rows.push([new Date(h.time).toISOString(), h.location || "", h.gps ? h.gps.lat : "", h.gps ? h.gps.lon : "",
      has(h.values.ph) ? h.values.ph : "", has(h.values.tds) ? h.values.tds : "", has(h.values.temp) ? h.values.temp : "",
      a.source ? a.source[0].id : "", h.truth || ""]);
  });
  return rows.map(r => r.map(c => /[",\n]/.test(String(c)) ? `"${String(c).replace(/"/g, '""')}"` : c).join(",")).join("\n");
}

$("export").onclick = () => {
  const csv = toCsv();
  $("csv-box").hidden = false;
  $("csv-text").value = csv;
  $("csv-text").select();
  const done = () => ($("csv-msg").textContent = "คัดลอกแล้ว");
  const manual = () => ($("csv-msg").textContent = "เลือกข้อความด้านบนแล้วคัดลอกเอง");
  if (navigator.clipboard && window.isSecureContext) navigator.clipboard.writeText(csv).then(done, manual); else manual();
  if (location.protocol === "http:") {
    const dl = $("csv-dl");
    dl.href = URL.createObjectURL(new Blob(["﻿" + csv], { type: "text/csv" }));
    dl.hidden = false;
  }
};
$("clear-hist").onclick = () => ($("clear-confirm").hidden = false);
$("clear-no").onclick = () => ($("clear-confirm").hidden = true);
$("clear-yes").onclick = () => {
  records = []; store.set("wqm-history", records);
  $("clear-confirm").hidden = true; renderHistory();
};

/* ===== สถานที่ ===== */
let gps = null;
$("loc").value = store.get("wqm-loc", "");
$("loc").oninput = () => store.set("wqm-loc", $("loc").value.trim());
if (window.isSecureContext && "geolocation" in navigator) $("gps").hidden = false;
$("gps").onclick = () => {
  $("loc-hint").textContent = "กำลังหาตำแหน่ง…";
  navigator.geolocation.getCurrentPosition(
    p => { gps = { lat: p.coords.latitude, lon: p.coords.longitude }; $("loc-hint").textContent = `ได้ตำแหน่งแล้ว ${gps.lat.toFixed(5)}, ${gps.lon.toFixed(5)}`; },
    () => { $("loc-hint").textContent = "ใช้ GPS ไม่ได้ในหน้านี้ ให้พิมพ์ชื่อสถานที่แทน"; },
    { enableHighAccuracy: true, timeout: 10000 });
};

function newRecord(values, extra) {
  return Object.assign({ values, time: Date.now(), timeSource: "phone", location: $("loc").value.trim(), gps }, extra);
}

/* ===== เชื่อมต่อเครื่องวัด =====
   - เปิดจาก ESP32 (http://192.168.4.1)     -> ใช้ API ของเครื่องเดียวกัน
   - เปิดจาก VS Code (Live Server) / ไฟล์ตรง -> ต่อคอมเข้า WiFi "WaterMeter" แล้วระบบเรียก http://192.168.4.1 ให้เอง
   - หาเครื่องไม่เจอ                         -> โหมดทดลอง (ใส่ค่าเอง) */
const DEFAULT_DEVICE = "192.168.4.1";
const canReachDevice = location.protocol !== "https:"; // เว็บ https เรียกเครื่องที่เป็น http ไม่ได้
let apiBase = null;   // null = ยังไม่ได้ต่อเครื่อง
let lastKey = null;
let mode = "";

function setConn(kind, text) { $("conn-dot").className = "dot " + kind; $("conn-text").textContent = text; }
function setLive(v) { $("live-ph").textContent = f(v.ph, 2); $("live-tds").textContent = fTds(v.tds); $("live-temp").textContent = f(v.temp, 1); }

async function getJson(url, opts, timeoutMs) {
  const ctl = new AbortController(); const t = setTimeout(() => ctl.abort(), timeoutMs || 3000);
  try { const r = await fetch(url, Object.assign({ cache: "no-store", signal: ctl.signal }, opts)); return await r.json(); }
  finally { clearTimeout(t); }
}

function deviceAddr() {
  return (store.get("wqm-device", DEFAULT_DEVICE) || DEFAULT_DEVICE).trim().replace(/^https?:\/\//, "").replace(/\/+$/, "");
}

async function findDevice() {
  const list = [];
  if (location.protocol === "http:") list.push("");        // หน้านี้อาจเปิดจาก ESP32 เอง
  const remote = "http://" + deviceAddr();
  if (location.origin !== remote) list.push(remote);
  for (const base of list) {
    try { const s = await getJson(base + "/api/state", {}, 2500); if (s && "live" in s) return base; } catch (e) { /* ลองที่อยู่ถัดไป */ }
  }
  return null;
}

async function poll() {
  try {
    const s = await getJson(apiBase + "/api/state");
    setConn("on", "เชื่อมต่อเครื่องวัดแล้ว" + (apiBase ? " (" + apiBase.replace("http://", "") + ")" : ""));
    setLive(s.live);
    if (!s.time_set) fetch(apiBase + "/api/time?epoch=" + Math.floor(Date.now() / 1000), { method: "POST" }).catch(() => {});
    if (s.last) {
      const key = s.boot + "-" + s.last.id;
      if (key !== lastKey) {
        lastKey = key;
        const old = records.find(h => h.key === key);
        if (old) { renderReport(old); }
        else addRecord(newRecord(s.last.values, {
          key, label: "#" + s.last.id,
          time: s.last.epoch ? s.last.epoch * 1000 : Date.now(),
          timeSource: s.last.epoch ? "device" : "phone"
        }));
      }
    }
  } catch (e) {
    setConn("off", "ขาดการเชื่อมต่อ ตรวจว่าต่อ WiFi WaterMeter อยู่");
  }
  setTimeout(poll, 1000);
}

async function webMeasure() {
  if (apiBase === null) { demoSend(); return; }
  try { await getJson(apiBase + "/api/measure", { method: "POST" }, 8000); } catch (e) { setConn("off", "สั่งวัดไม่สำเร็จ"); }
}

/* ===== โหมดทดลอง ===== */
function demoValues() {
  const val = (id) => ($(id + "-off").checked || $(id).value === "" ? null : parseFloat($(id).value));
  return { ph: val("d-ph"), tds: val("d-tds"), temp: val("d-temp") };
}
let demoCount = 0;
function demoSend() {
  const v = demoValues();
  demoCount++;
  addRecord(newRecord(v, { key: "demo-" + Date.now(), label: "ทดลอง " + demoCount }));
  setLive(v);
}

function enterDeviceMode(base) {
  apiBase = base;
  mode = "device";
  $("demo-panel").hidden = true;
  renderEmpty();
  poll();
}

function enterDemoMode() {
  if (mode === "demo") return;
  mode = "demo";
  setConn("demo", "โหมดทดลอง (ไม่ได้ต่อเครื่องวัด)");
  $("demo-panel").hidden = false;
  const sample = { ph: 7.62, tds: 184, temp: 29.4 };
  setLive(sample);
  renderReport(newRecord(sample, { key: "example", label: "ตัวอย่าง", example: true, location: "ตัวอย่าง: ก๊อกน้ำในโรงเรียน" }));
}

async function connect(manual) {
  if (manual) $("dev-msg").textContent = "กำลังค้นหาเครื่องวัด…";
  const base = await findDevice();
  if (base !== null) { enterDeviceMode(base); return; }
  enterDemoMode();
  if (manual) $("dev-msg").textContent = "ไม่พบเครื่องวัดที่ " + deviceAddr() + " ตรวจว่าคอม/มือถือต่อ WiFi WaterMeter อยู่";
  else if (canReachDevice) setTimeout(() => { if (mode === "demo") connect(false); }, 5000); // ลองหาใหม่เรื่อยๆ
}

/* ===== เริ่มทำงาน ===== */
$("model-info").textContent = `โมเดลประเมินแหล่งน้ำ: Naive Bayes รุ่น ${MODEL.version} · ข้อมูลที่ใช้: ${MODEL.trained_on}` +
  (MODEL.accuracy !== null ? ` · ความแม่นยำจากการทดสอบ ${Math.round(MODEL.accuracy * 100)}% (${MODEL.samples} ตัวอย่าง)` : " · ยังไม่ได้วัดความแม่นยำกับข้อมูลจริง");
renderHistory();
$("d-send").onclick = demoSend;
$("dev-row").hidden = !canReachDevice;
$("dev-addr").value = deviceAddr();
$("dev-connect").onclick = () => { store.set("wqm-device", $("dev-addr").value); connect(true); };

if (canReachDevice) { setConn("", "กำลังค้นหาเครื่องวัด…"); connect(false); }
else enterDemoMode();
