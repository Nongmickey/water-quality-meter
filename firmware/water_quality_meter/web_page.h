// ไฟล์นี้สร้างอัตโนมัติจากโฟลเดอร์ web/ ด้วย tools/build_web.py — อย่าแก้ไฟล์นี้โดยตรง
#pragma once
#include <pgmspace.h>

const char INDEX_HTML[] PROGMEM = R"WQMHTML(<!doctype html>
<html lang="th">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<meta name="theme-color" content="#0b7480">
<title>Smart Water</title>
<meta name="claude:viewport-fit-sides" content="cover">
<style>
/* Smart Water — mobile first: app bar, live tiles, big measure button, report slip, charts, history. Bottom tab bar. */
:root {
  --paper: #eef4f6;
  --panel: #ffffff;
  --ink: #0f2a33;
  --muted: #5b7581;
  --line: #d6e3e8;
  --accent: #0b7480;
  --accent-2: #12a4a0;
  --accent-ink: #ffffff;
  --accent-soft: #d8eff1;
  --good: #1d7f48;
  --good-soft: #dcf1e4;
  --warn: #9a6512;
  --warn-soft: #f6ead2;
  --bad: #b8352a;
  --bad-soft: #f8dfdb;
  --none: #7d8f95;
  --none-soft: #e6eced;
  --c-ph: #6d4bd8;
  --c-tds: #0c8fb0;
  --c-temp: #e0683a;
  --shadow: 0 1px 2px rgba(15, 42, 51, .06), 0 6px 18px rgba(15, 42, 51, .07);
  --radius: 16px;
  --font-ui: system-ui, -apple-system, "Segoe UI", "Noto Sans Thai", "Leelawadee UI", Tahoma, sans-serif;
  --font-num: ui-monospace, "SF Mono", Menlo, Consolas, "Roboto Mono", monospace;
}
@media (prefers-color-scheme: dark) {
  :root:not([data-theme="light"]) {
    --paper: #08161a; --panel: #0f2329; --ink: #e2eff2; --muted: #8ea9b2; --line: #1f3a42;
    --accent: #3cc2c9; --accent-2: #2aa58f; --accent-ink: #04201f; --accent-soft: #123a40;
    --good: #5cc98d; --good-soft: #113626; --warn: #e2ad55; --warn-soft: #3a2c12;
    --bad: #f07a6c; --bad-soft: #3f1d19; --none: #8a9da3; --none-soft: #1b2e33;
    --c-ph: #a48cff; --c-tds: #4cc3e0; --c-temp: #ff9a6b;
    --shadow: 0 1px 2px rgba(0, 0, 0, .3), 0 6px 18px rgba(0, 0, 0, .25);
    color-scheme: dark;
  }
}
:root[data-theme="dark"] {
  --paper: #08161a; --panel: #0f2329; --ink: #e2eff2; --muted: #8ea9b2; --line: #1f3a42;
  --accent: #3cc2c9; --accent-2: #2aa58f; --accent-ink: #04201f; --accent-soft: #123a40;
  --good: #5cc98d; --good-soft: #113626; --warn: #e2ad55; --warn-soft: #3a2c12;
  --bad: #f07a6c; --bad-soft: #3f1d19; --none: #8a9da3; --none-soft: #1b2e33;
  --c-ph: #a48cff; --c-tds: #4cc3e0; --c-temp: #ff9a6b;
  --shadow: 0 1px 2px rgba(0, 0, 0, .3), 0 6px 18px rgba(0, 0, 0, .25);
  color-scheme: dark;
}
* { box-sizing: border-box; }
[hidden] { display: none !important; }
html { -webkit-text-size-adjust: 100%; }
body {
  margin: 0;
  background: var(--paper);
  color: var(--ink);
  font: 15px/1.6 var(--font-ui);
  padding-bottom: calc(84px + env(safe-area-inset-bottom, 0px));
}
.wrap {
  max-width: 720px; margin: 0 auto; display: flex; flex-direction: column; gap: 14px;
  padding: 14px max(16px, env(safe-area-inset-left, 0px)) 24px max(16px, env(safe-area-inset-right, 0px));
}
.tab { display: flex; flex-direction: column; gap: 14px; }
h1, h2, h3 { margin: 0; line-height: 1.3; text-wrap: balance; }
h1 { font-size: 20px; font-weight: 700; }
h2 { font-size: 17px; font-weight: 700; }
h3 { font-size: 15px; font-weight: 700; }
.eyebrow { font-size: 11.5px; letter-spacing: .08em; text-transform: uppercase; color: var(--muted); }
.muted { color: var(--muted); }
.num { font-family: var(--font-num); font-variant-numeric: tabular-nums; }
.center { text-align: center; }
button, select, input { font: inherit; color: inherit; }
button {
  border: 1px solid var(--line); background: var(--panel); border-radius: 10px;
  padding: 8px 14px; cursor: pointer; min-height: 40px;
}
button:disabled { opacity: .55; cursor: default; }
button.primary { background: var(--accent); color: var(--accent-ink); border-color: var(--accent); font-weight: 600; }
button.danger { background: var(--bad); color: #fff; border-color: var(--bad); font-weight: 600; }
button:focus-visible, input:focus-visible, select:focus-visible { outline: 2px solid var(--accent); outline-offset: 2px; }
input, select {
  background: var(--panel); border: 1px solid var(--line); border-radius: 10px; padding: 8px 10px; min-width: 0; min-height: 40px;
}
a { color: var(--accent); }

/* ===== app bar ===== */
.appbar {
  position: sticky; top: 0; z-index: 10;
  display: flex; flex-wrap: wrap; align-items: center; justify-content: space-between; gap: 6px 12px;
  padding: calc(10px + env(safe-area-inset-top, 0px)) max(16px, env(safe-area-inset-right, 0px)) 10px max(16px, env(safe-area-inset-left, 0px));
  background: linear-gradient(120deg, #0b5f6a, #0b7480 45%, #12a4a0);
  color: #fff;
  box-shadow: 0 2px 12px rgba(11, 95, 106, .25);
}
.appbar .eyebrow { color: rgba(255, 255, 255, .75); }
.brand { display: flex; align-items: center; gap: 10px; }
.logo { width: 34px; height: 34px; color: #fff; background: rgba(255, 255, 255, .16); border-radius: 10px; padding: 5px; }
.conn {
  display: inline-flex; align-items: center; gap: 6px; font-size: 12.5px;
  background: rgba(255, 255, 255, .16); padding: 4px 10px; border-radius: 99px;
}
.dot { width: 9px; height: 9px; border-radius: 50%; background: #c9d6da; flex: none; }
.dot.on { background: #6be3a0; box-shadow: 0 0 0 3px rgba(107, 227, 160, .25); }
.dot.off { background: #ff8a7a; }
.dot.demo { background: #ffd27a; }

/* ===== live tiles ===== */
.live { display: grid; grid-template-columns: repeat(3, minmax(0, 1fr)); gap: 10px; }
.tile {
  background: var(--panel); border-radius: var(--radius); padding: 10px 12px 6px; box-shadow: var(--shadow);
  border-top: 4px solid var(--tc); display: flex; flex-direction: column; min-width: 0;
}
.tile span { font-size: 12px; color: var(--muted); white-space: nowrap; }
.tile b { font-family: var(--font-num); font-size: 24px; font-variant-numeric: tabular-nums; line-height: 1.2; color: var(--tc); }
.t-ph { --tc: var(--c-ph); }
.t-tds { --tc: var(--c-tds); }
.t-temp { --tc: var(--c-temp); }
.spark { width: 100%; height: 30px; margin-top: 2px; }
.spark polyline { fill: none; stroke: var(--tc); stroke-width: 2; vector-effect: non-scaling-stroke; }
.spark polygon { fill: var(--tc); opacity: .12; }

/* ===== panels ===== */
.panel {
  background: var(--panel); border-radius: var(--radius); padding: 16px; box-shadow: var(--shadow);
  display: flex; flex-direction: column; gap: 12px;
}
.row { display: flex; flex-wrap: wrap; gap: 8px; align-items: center; }
.row.between { justify-content: space-between; }
.row > input[type="text"] { flex: 1 1 200px; }
.field { display: flex; flex-direction: column; gap: 4px; flex: 1 1 120px; min-width: 0; }
.field label { font-size: 13px; color: var(--muted); }
.check { font-size: 13px; color: var(--muted); display: flex; gap: 6px; align-items: center; }
.hint { font-size: 13px; color: var(--muted); }
.dev-row { display: flex; flex-direction: column; gap: 6px; padding-top: 12px; border-top: 1px dashed var(--line); }
.dev-row input { flex: 1 1 180px; }

/* ===== measure button ===== */
.measure-btn {
  display: flex; align-items: center; justify-content: center; gap: 12px;
  width: 100%; min-height: 64px; border: 0; border-radius: 99px;
  background: linear-gradient(135deg, #1fae5f, #12935a);
  color: #fff; font-size: 20px; font-weight: 700; letter-spacing: .02em;
  box-shadow: 0 8px 20px rgba(18, 147, 90, .35), inset 0 -3px 0 rgba(0, 0, 0, .15);
}
.measure-btn:active { transform: translateY(1px); box-shadow: 0 4px 10px rgba(18, 147, 90, .35); }
.mb-dot { width: 22px; height: 22px; border-radius: 50%; background: rgba(255, 255, 255, .9); box-shadow: 0 0 0 5px rgba(255, 255, 255, .25); }
.busy {
  display: grid; grid-template-columns: auto 1fr; gap: 8px 12px; align-items: center;
  background: var(--accent-soft); border-radius: 12px; padding: 12px 14px;
}
.busy-txt { display: flex; flex-direction: column; }
.busy-bar { grid-column: 1 / -1; height: 6px; background: rgba(11, 116, 128, .15); border-radius: 3px; overflow: hidden; }
.busy-bar span { display: block; height: 100%; width: 0; background: var(--accent); border-radius: 3px; }
.spinner {
  width: 26px; height: 26px; border-radius: 50%;
  border: 3px solid rgba(11, 116, 128, .2); border-top-color: var(--accent);
}
@media (prefers-reduced-motion: no-preference) {
  .spinner { animation: spin .9s linear infinite; }
  .busy-bar span { transition: width .9s linear; }
  .fill, .mark { transition: all .5s ease; }
}
@keyframes spin { to { transform: rotate(360deg); } }

/* ===== report slip ===== */
.slip { background: var(--panel); border-radius: var(--radius); overflow: hidden; box-shadow: var(--shadow); }
.slip-head {
  display: grid; grid-template-columns: 1fr auto; gap: 4px 16px; padding: 14px 16px;
  border-bottom: 1px dashed var(--line);
}
.slip-head .meta { font-size: 13px; color: var(--muted); display: flex; flex-direction: column; }
.sample-no { font-family: var(--font-num); font-size: 13px; color: var(--muted); text-align: right; }
.tag { display: inline-block; font-size: 12px; padding: 1px 8px; border-radius: 99px; background: var(--warn-soft); color: var(--warn); }
.slip section { padding: 16px; border-bottom: 1px solid var(--line); display: flex; flex-direction: column; gap: 12px; }
.slip section:last-child { border-bottom: 0; }
.warnbox { background: var(--warn-soft); color: var(--warn); border-radius: 10px; padding: 8px 12px; font-size: 13.5px; }

.slip section.verdict { display: grid; grid-template-columns: auto 1fr; gap: 12px 16px; align-items: center; }
.score {
  width: 88px; height: 88px; border-radius: 50%; display: grid; place-items: center; text-align: center;
  font-family: var(--font-num); font-size: 27px; font-weight: 700; line-height: 1;
  background: conic-gradient(var(--c) calc(var(--p, 0) * 1%), var(--cs) 0);
  position: relative;
}
.score::before { content: ""; position: absolute; inset: 7px; border-radius: 50%; background: var(--panel); }
.score span { position: relative; }
.score small { display: block; font-size: 11px; font-weight: 400; color: var(--muted); font-family: var(--font-ui); }
.verdict h2 { font-size: 20px; }

.s-good { --c: var(--good); --cs: var(--good-soft); }
.s-warn { --c: var(--warn); --cs: var(--warn-soft); }
.s-bad  { --c: var(--bad);  --cs: var(--bad-soft); }
.s-none { --c: var(--none); --cs: var(--none-soft); }
.score.s-good, .score.s-warn, .score.s-bad, .score.s-none { color: var(--c); }
.pill { display: inline-block; font-size: 12.5px; font-weight: 600; padding: 2px 10px; border-radius: 99px; background: var(--cs); color: var(--c); white-space: nowrap; }

/* parameter rows */
.param { display: grid; grid-template-columns: minmax(0, 1fr) auto; gap: 4px 12px; }
.param .name { font-weight: 600; }
.param .val { font-family: var(--font-num); font-size: 24px; font-variant-numeric: tabular-nums; text-align: right; line-height: 1.1; }
.param .val small { font-size: 13px; color: var(--muted); }
.param .desc { grid-column: 1 / -1; font-size: 13.5px; color: var(--muted); }
.scale { grid-column: 1 / -1; position: relative; height: 42px; }
.scale .bar { position: absolute; left: 0; right: 0; top: 6px; height: 10px; border-radius: 5px; }
.scale .ok { position: absolute; top: 2px; height: 18px; border: 2px solid var(--ink); border-radius: 6px; opacity: .35; }
.scale .mark { position: absolute; top: 0; width: 4px; height: 22px; margin-left: -2px; background: var(--ink); border-radius: 2px; box-shadow: 0 0 0 2px var(--panel); }
.scale .ticks { position: absolute; left: 0; right: 0; top: 21px; font-size: 11px; color: var(--muted); font-family: var(--font-num); }
.scale .ticks span { position: absolute; transform: translateX(-50%); white-space: nowrap; }
.scale .ticks span:first-child { transform: none; }
.scale .ticks span:last-child { transform: translateX(-100%); }
.ph-bar { background: linear-gradient(90deg, #d7263d 0%, #f46036 14%, #f9c80e 30%, #8bc34a 46%, #2e9e5b 54%, #1b8a9e 66%, #2b59c3 80%, #5a2a9e 100%); }
.tds-bar { background: linear-gradient(90deg, #8fd3e0 0%, #3fae8c 35%, #d6c04a 62%, #e08a3a 78%, #c0392b 100%); }

/* source */
.guess { display: flex; flex-wrap: wrap; align-items: baseline; gap: 4px 10px; }
.guess b { font-size: 19px; }
.bars { display: flex; flex-direction: column; gap: 6px; }
.bars div { display: grid; grid-template-columns: minmax(0, 9.5em) 1fr 3.2em; gap: 8px; align-items: center; font-size: 13.5px; }
.bars .track { height: 8px; background: var(--accent-soft); border-radius: 4px; overflow: hidden; }
.bars .fill { height: 100%; background: linear-gradient(90deg, var(--accent), var(--accent-2)); border-radius: 4px; }
.bars .pct { font-family: var(--font-num); text-align: right; color: var(--muted); }
.bars .nm { overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }

/* suitability */
.suit { display: grid; grid-template-columns: minmax(0, 1fr); gap: 8px; }
.suit div { display: grid; grid-template-columns: 7.5em minmax(0, 1fr); gap: 4px 10px; align-items: start; }
.suit .what { font-weight: 600; }
.suit .note { font-size: 13.5px; color: var(--muted); grid-column: 2; }
ul.advice { margin: 0; padding-left: 20px; display: flex; flex-direction: column; gap: 6px; }
.fine { font-size: 12.5px; color: var(--muted); }

.empty { text-align: center; padding: 28px 16px; color: var(--muted); display: flex; flex-direction: column; gap: 8px; align-items: center; }
.empty .big-btn { width: 52px; height: 52px; border-radius: 50%; background: var(--good); box-shadow: inset 0 -4px 0 rgba(0,0,0,.2); }

/* ===== charts ===== */
.stats { display: grid; grid-template-columns: repeat(4, minmax(0, 1fr)); gap: 8px; }
.stat { background: var(--paper); border-radius: 12px; padding: 8px 10px; min-width: 0; }
.stat span { display: block; font-size: 11.5px; color: var(--muted); white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
.stat b { font-family: var(--font-num); font-size: 19px; font-variant-numeric: tabular-nums; }
.chart-card { gap: 6px; }
.chart { width: 100%; height: auto; display: block; overflow: visible; }
.chart .grid line { stroke: var(--line); stroke-width: 1; }
.chart .grid text, .chart .xl { fill: var(--muted); font-size: 10px; font-family: var(--font-num); }
.chart .bandr { fill: var(--good); opacity: .12; }
.chart .ln { fill: none; stroke-width: 2.5; stroke-linejoin: round; stroke-linecap: round; }
.chart .area { opacity: .1; }
.chart circle { stroke: var(--panel); stroke-width: 2; }
.chart .lastv { font-family: var(--font-num); font-size: 11px; font-weight: 700; }
.legend { font-size: 12px; color: var(--muted); display: inline-flex; align-items: center; gap: 6px; }
.legend .band { width: 14px; height: 10px; border-radius: 3px; background: var(--good); opacity: .25; display: inline-block; }

/* ===== history ===== */
.dev-sync {
  display: flex; flex-wrap: wrap; justify-content: space-between; align-items: center; gap: 8px;
  background: var(--accent-soft); border-radius: 12px; padding: 10px 12px;
}
.hist-scroll { overflow-x: auto; }
table { border-collapse: collapse; width: 100%; font-size: 13.5px; min-width: 600px; }
th, td { text-align: left; padding: 8px 6px; border-bottom: 1px solid var(--line); vertical-align: middle; }
th { font-size: 12px; color: var(--muted); font-weight: 600; }
td.n { font-family: var(--font-num); font-variant-numeric: tabular-nums; text-align: right; }
td select { padding: 4px 6px; font-size: 13px; max-width: 11em; min-height: 32px; }
tbody tr.clickable { cursor: pointer; }
tbody tr.clickable:hover td { background: var(--accent-soft); }
textarea { width: 100%; min-height: 120px; font: 12px/1.4 var(--font-num); background: var(--paper); color: var(--ink); border: 1px solid var(--line); border-radius: 10px; padding: 8px; }

/* ===== bottom tab bar ===== */
.tabbar {
  position: fixed; left: 0; right: 0; bottom: 0; z-index: 20;
  display: grid; grid-template-columns: repeat(3, 1fr);
  background: var(--panel); border-top: 1px solid var(--line);
  padding: 6px max(8px, env(safe-area-inset-left, 0px)) calc(6px + env(safe-area-inset-bottom, 0px)) max(8px, env(safe-area-inset-right, 0px));
  box-shadow: 0 -4px 16px rgba(15, 42, 51, .08);
}
.tabbar button {
  border: 0; background: none; display: flex; flex-direction: column; align-items: center; gap: 2px;
  font-size: 12.5px; color: var(--muted); padding: 4px 0; min-height: 52px; border-radius: 12px;
}
.tabbar svg { width: 24px; height: 24px; fill: none; stroke: currentColor; stroke-width: 2; stroke-linecap: round; stroke-linejoin: round; }
.tabbar button[aria-selected="true"] { color: var(--accent); font-weight: 700; background: var(--accent-soft); }
.tabbar button[aria-selected="true"] svg { fill: currentColor; fill-opacity: .15; }

@media (max-width: 460px) {
  .tile b { font-size: 20px; }
  .stats { grid-template-columns: repeat(2, minmax(0, 1fr)); }
  .suit div { grid-template-columns: 1fr; }
  .suit .note { grid-column: 1; }
}
@media (min-width: 760px) {
  .wrap { padding-top: 20px; }
}
</style>
</head>
<body>
<header class="appbar">
  <div class="brand">
    <svg class="logo" viewBox="0 0 24 24" aria-hidden="true"><path d="M12 2.5c3.6 4.3 6.5 8 6.5 11.4A6.5 6.5 0 0 1 5.5 13.9C5.5 10.5 8.4 6.8 12 2.5z" fill="currentColor"/><path d="M9 14.5a3 3 0 0 0 3 3" fill="none" stroke="#fff" stroke-width="1.6" stroke-linecap="round"/></svg>
    <div>
      <div class="eyebrow">Smart Water System</div>
      <h1>ตรวจคุณภาพน้ำ</h1>
    </div>
  </div>
  <div class="conn"><span class="dot" id="conn-dot"></span><span id="conn-text">กำลังเชื่อมต่อ…</span></div>
</header>

<main class="wrap">
  <!-- ===== แท็บ: วัด ===== -->
  <section class="tab" id="tab-measure" aria-label="วัด">
    <div class="live" aria-label="ค่าสดจากเซนเซอร์">
      <div class="tile t-ph"><span>pH</span><b id="live-ph">-</b><svg class="spark" id="spark-ph" viewBox="0 0 100 30" preserveAspectRatio="none" aria-hidden="true"></svg></div>
      <div class="tile t-tds"><span>TDS ppm</span><b id="live-tds">-</b><svg class="spark" id="spark-tds" viewBox="0 0 100 30" preserveAspectRatio="none" aria-hidden="true"></svg></div>
      <div class="tile t-temp"><span>อุณหภูมิ °C</span><b id="live-temp">-</b><svg class="spark" id="spark-temp" viewBox="0 0 100 30" preserveAspectRatio="none" aria-hidden="true"></svg></div>
    </div>

    <div class="panel measure-card">
      <div class="row">
        <input type="text" id="loc" placeholder="สถานที่เก็บตัวอย่าง เช่น บ่อหลังโรงเรียน" autocomplete="off" aria-label="สถานที่เก็บตัวอย่าง">
        <button id="gps" hidden>ใช้ GPS</button>
      </div>
      <div class="hint" id="loc-hint">ชื่อสถานที่จะถูกบันทึกไปกับผลตรวจครั้งต่อไป</div>
      <button class="measure-btn" id="measure-btn"><span class="mb-dot" aria-hidden="true"></span><span id="measure-label">เริ่มวัด</span></button>
      <div class="busy" id="busy" hidden>
        <div class="spinner" aria-hidden="true"></div>
        <div class="busy-txt"><b>กำลังวัด · รอให้ค่านิ่ง</b><span class="hint" id="busy-text">0 วินาที</span></div>
        <div class="busy-bar"><span id="busy-fill"></span></div>
      </div>
      <p class="hint center" style="margin:0">หรือกดปุ่มสีเขียวที่ตัวเครื่อง · จุ่มหัววัดทีละตัว อย่าจุ่ม pH กับ TDS ในแก้วเดียวกันพร้อมกัน</p>
    </div>

    <div class="panel" id="demo-panel" hidden>
      <div>
        <h2>โหมดทดลอง</h2>
        <p class="hint" style="margin:0">หน้านี้ยังไม่ได้ต่อกับเครื่องวัด ใส่ค่าเองเพื่อลองระบบวิเคราะห์ได้ ค่าที่ไม่ได้วัดให้ติ๊ก “ไม่ได้วัด”</p>
      </div>
      <div class="row">
        <div class="field"><label for="d-ph">pH</label><input id="d-ph" type="number" step="0.01" min="0" max="14" value="7.62"><span class="check"><input type="checkbox" id="d-ph-off">ไม่ได้วัด</span></div>
        <div class="field"><label for="d-tds">TDS (ppm)</label><input id="d-tds" type="number" step="1" min="0" max="2000" value="184"><span class="check"><input type="checkbox" id="d-tds-off">ไม่ได้วัด</span></div>
        <div class="field"><label for="d-temp">อุณหภูมิ (°C)</label><input id="d-temp" type="number" step="0.1" min="0" max="60" value="29.4"><span class="check"><input type="checkbox" id="d-temp-off">ไม่ได้วัด</span></div>
      </div>
      <div id="dev-row" class="dev-row">
        <label for="dev-addr" class="eyebrow">ต่อกับเครื่องวัดจริง</label>
        <div class="row">
          <input id="dev-addr" type="text" inputmode="url" autocomplete="off" aria-describedby="dev-msg">
          <button id="dev-connect">เชื่อมต่อ</button>
        </div>
        <span class="hint" id="dev-msg">ต่อมือถือเข้า WiFi “WaterMeter” ก่อน แล้วกดเชื่อมต่อ</span>
      </div>
    </div>

    <div id="report"></div>
  </section>

  <!-- ===== แท็บ: กราฟ ===== -->
  <section class="tab" id="tab-charts" aria-label="กราฟ" hidden>
    <div class="panel">
      <div class="row between">
        <h2>แนวโน้มคุณภาพน้ำ</h2>
        <div class="row">
          <select id="chart-loc" aria-label="เลือกสถานที่"><option value="">ทุกสถานที่</option></select>
          <select id="chart-range" aria-label="จำนวนผลตรวจ">
            <option value="10">10 ครั้งล่าสุด</option>
            <option value="30" selected>30 ครั้งล่าสุด</option>
            <option value="0">ทั้งหมด</option>
          </select>
        </div>
      </div>
      <div class="stats" id="stats"></div>
    </div>
    <div class="panel chart-card"><div class="row between"><h3>pH ความเป็นกรด-ด่าง</h3><span class="legend"><i class="band"></i>เกณฑ์น้ำดื่ม 6.5–8.5</span></div><div id="chart-ph"></div></div>
    <div class="panel chart-card"><div class="row between"><h3>TDS สารที่ละลายในน้ำ (ppm)</h3><span class="legend"><i class="band"></i>ไม่เกิน 500</span></div><div id="chart-tds"></div></div>
    <div class="panel chart-card"><div class="row between"><h3>อุณหภูมิน้ำ (°C)</h3><span class="legend"><i class="band"></i>ปกติ 25–32</span></div><div id="chart-temp"></div></div>
    <div class="panel"><h3>AI คาดว่าน้ำมาจาก (นับจากผลตรวจในช่วงนี้)</h3><div id="chart-src"></div></div>
  </section>

  <!-- ===== แท็บ: ประวัติ ===== -->
  <section class="tab" id="tab-history" aria-label="ประวัติ" hidden>
    <div class="panel">
      <div class="row between">
        <h2>ประวัติการตรวจ</h2>
        <div class="row">
          <button id="export">ส่งออก CSV</button>
          <button id="clear-hist">ล้างประวัติ</button>
        </div>
      </div>
      <div class="row" id="clear-confirm" hidden>
        <span>ลบประวัติทั้งหมดในมือถือนี้?</span>
        <button id="clear-yes" class="danger">ลบเลย</button>
        <button id="clear-no">ยกเลิก</button>
      </div>
      <div class="dev-sync" id="dev-sync" hidden>
        <div><b>ข้อมูลที่บันทึกในเครื่องวัด</b><div class="hint" id="sync-msg">-</div></div>
        <div class="row">
          <button id="sync-dev">ดึงข้อมูลจากเครื่อง</button>
          <button id="clear-dev">ล้างข้อมูลในเครื่อง</button>
        </div>
      </div>
      <p class="hint" style="margin:0">แตะแถวเพื่อดูใบรายงานผล · เลือก “แหล่งน้ำจริง” เมื่อรู้ว่าน้ำมาจากไหน แล้วส่งออก CSV ไปเทรนโมเดลใหม่ (model/train.py) ยิ่งมีข้อมูลจริงมาก AI ยิ่งแม่นขึ้น</p>
      <div class="hist-scroll"><table>
        <thead><tr><th>วันเวลา</th><th>สถานที่</th><th>pH</th><th>TDS</th><th>°C</th><th>คะแนน</th><th>AI คาดว่าเป็น</th><th>แหล่งน้ำจริง</th></tr></thead>
        <tbody id="hist-body"></tbody>
      </table></div>
      <div id="csv-box" hidden>
        <textarea id="csv-text" readonly></textarea>
        <div class="row"><span class="hint" id="csv-msg"></span><a id="csv-dl" hidden download="water-samples.csv">ดาวน์โหลดไฟล์</a></div>
      </div>
    </div>
    <p class="fine" id="model-info"></p>
  </section>
</main>

<nav class="tabbar" role="tablist" aria-label="เมนู">
  <button role="tab" data-go="measure" aria-selected="true">
    <svg viewBox="0 0 24 24" aria-hidden="true"><path d="M12 3c3 3.6 5.5 6.7 5.5 9.6A5.5 5.5 0 0 1 6.5 12.6C6.5 9.7 9 6.6 12 3z"/></svg>วัด</button>
  <button role="tab" data-go="charts" aria-selected="false">
    <svg viewBox="0 0 24 24" aria-hidden="true"><path d="M4 19h16M6 15l4-5 3 3 5-7"/></svg>กราฟ</button>
  <button role="tab" data-go="history" aria-selected="false">
    <svg viewBox="0 0 24 24" aria-hidden="true"><path d="M5 5h14v14H5zM8 9h8M8 12h8M8 15h5"/></svg>ประวัติ</button>
</nav>

<script>
/* โมเดลประเมินแหล่งน้ำ (Gaussian Naive Bayes)
   ไฟล์นี้ถูกเขียนทับโดย model/train.py — ถ้าจะแก้ค่าอ้างอิง ให้แก้ model/reference_model.json */
const MODEL = /*MODEL_START*/{
  "version": "ref-1",
  "trained_on": "ค่าอ้างอิงจากเอกสาร (ยังไม่ได้เทรนด้วยข้อมูลจริง)",
  "accuracy": null,
  "samples": 0,
  "features": ["ph", "log_tds", "temp"],
  "tds_max": 1000,
  "classes": [
    {"id": "ro", "name": "น้ำกรอง RO / น้ำกลั่น", "prior": 1, "mean": [6.3, 1.08, 29], "std": [0.5, 0.3, 4],
     "clue": "TDS ต่ำมาก และ pH เป็นกรดอ่อน เพราะแร่ธาตุถูกกรองออกเกือบหมด"},
    {"id": "rain", "name": "น้ำฝน", "prior": 1, "mean": [5.8, 1.3, 29], "std": [0.5, 0.3, 4],
     "clue": "TDS ต่ำมากคล้ายน้ำ RO แต่ pH มักต่ำกว่า (ประมาณ 5–6.5) เพราะละลายคาร์บอนไดออกไซด์จากอากาศ"},
    {"id": "bottled", "name": "น้ำดื่มบรรจุขวด", "prior": 1, "mean": [6.9, 1.78, 29], "std": [0.4, 0.25, 4],
     "clue": "TDS ต่ำ (ประมาณ 20–150 ppm) และ pH ใกล้ 7"},
    {"id": "mineral", "name": "น้ำแร่", "prior": 1, "mean": [7.4, 2.4, 29], "std": [0.35, 0.18, 4],
     "clue": "TDS ค่อนข้างสูง (150–500 ppm) จากแร่ธาตุที่ละลายอยู่ และ pH 7–8"},
    {"id": "tap", "name": "น้ำประปา", "prior": 1, "mean": [7.6, 2.23, 29], "std": [0.35, 0.18, 4],
     "clue": "TDS ปานกลาง (100–300 ppm) และ pH สูงกว่า 7 เล็กน้อยจากการปรับสภาพน้ำ มักมีกลิ่นคลอรีน"},
    {"id": "surface", "name": "น้ำผิวดิน (แม่น้ำ คลอง หนอง บึง)", "prior": 1, "mean": [7.2, 2.34, 29], "std": [0.6, 0.3, 4],
     "clue": "TDS เปลี่ยนตามฤดูกาล pH 6.5–8.5 มักขุ่นหรือมีตะกอน"},
    {"id": "ground", "name": "น้ำบาดาล / น้ำบ่อ", "prior": 1, "mean": [6.9, 2.74, 29], "std": [0.6, 0.25, 4],
     "clue": "TDS สูง (300–1000 ppm ขึ้นไป) เพราะน้ำซึมผ่านชั้นดินและหินจนละลายแร่ธาตุมา"},
    {"id": "brackish", "name": "น้ำกร่อย / มีน้ำทะเลปน", "prior": 1, "mean": [7.8, 3.3, 29], "std": [0.4, 0.3, 4],
     "clue": "TDS สูงมากจนมักเกินช่วงของเซนเซอร์ เพราะมีเกลือจากน้ำทะเลปน"}
  ]
}/*MODEL_END*/;
</script>
<script>

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
function fmtRow(ms) {
  try { return new Date(ms).toLocaleString("th-TH", { day: "numeric", month: "short", year: "2-digit", hour: "2-digit", minute: "2-digit" }); }
  catch (e) { return new Date(ms).toLocaleString(); }
}
function fmtShort(ms) {
  try { return new Date(ms).toLocaleDateString("th-TH", { day: "numeric", month: "short" }); }
  catch (e) { return ""; }
}
function timeNote(rec) {
  if (rec.timeSource === "phone") return " (เวลาจากมือถือ)";
  if (rec.timeSource === "unknown") return " (ไม่ทราบเวลาจริง: วัดตอนไม่ได้ต่อมือถือ)";
  return "";
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
    <div class="ticks"><span style="left:0">1</span><span style="left:${lp(50)}%">50</span><span style="left:${lp(500)}%">500</span><span style="left:100%">2000</span></div></div>`;
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
  const stab = rec.stable === false
    ? `<div class="warnbox">⚠️ ค่ายังไม่นิ่งภายใน ${rec.wait || 30} วินาที ผลอาจคลาดเคลื่อน ลองวัดใหม่โดยถือหัววัดให้นิ่งในน้ำ</div>`
    : "";
  $("report").innerHTML = `
  <article class="slip" aria-live="polite">
    <div class="slip-head">
      <div class="meta">
        <span class="eyebrow">ใบรายงานผล${rec.example ? ` <span class="tag">ตัวอย่าง</span>` : ""}</span>
        <span>🕒 ${esc(fmtTime(rec.time))}${esc(timeNote(rec))}</span>
        <span>📍 ${rec.location ? esc(rec.location) : "ไม่ได้ระบุสถานที่"}${rec.gps ? ` · <a href="https://maps.google.com/?q=${rec.gps.lat},${rec.gps.lon}" target="_blank" rel="noopener">${rec.gps.lat.toFixed(5)}, ${rec.gps.lon.toFixed(5)}</a>` : ""}</span>
        ${rec.stable === true ? `<span>✅ ค่านิ่งใน ${rec.wait} วินาที แล้วเฉลี่ย 6 ค่าล่าสุด</span>` : ""}
      </div>
      <div class="sample-no">ครั้งที่<br>${esc(rec.label)}</div>
    </div>

    <section class="verdict">
      <div class="score s-${a.grade.st}" style="--p:${a.score === null ? 0 : a.score}"><span>${a.score === null ? "–" : a.score}<small>/100</small></span></div>
      <div>
        <h2>${esc(a.grade.text)}</h2>
        <div class="hint">คะแนนรวมจาก pH และ TDS เทียบเกณฑ์น้ำดื่มของกรมอนามัย</div>
      </div>
      ${stab ? `<div style="grid-column:1/-1">${stab}</div>` : ""}
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
      <h3>🤖 AI วิเคราะห์: น้ำนี้น่าจะมาจาก</h3>
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
    <h2>พร้อมวัดแล้ว</h2>
    <p style="margin:0">จุ่มหัววัดลงในน้ำ แล้วกด “เริ่มวัด” หรือปุ่มสีเขียวที่เครื่อง ระบบจะรอให้ค่านิ่ง เฉลี่ยค่า แล้วให้ AI วิเคราะห์ให้อัตโนมัติ</p>
  </div></div>`;
}

/* ===== แท็บ ===== */
const store = {
  get(k, d) { try { const s = localStorage.getItem(k); return s ? JSON.parse(s) : d; } catch (e) { return d; } },
  set(k, v) { try { localStorage.setItem(k, JSON.stringify(v)); } catch (e) {} }
};
const TABS = ["measure", "charts", "history"];
function showTab(name) {
  if (!TABS.includes(name)) name = "measure";
  TABS.forEach(t => { $("tab-" + t).hidden = t !== name; });
  document.querySelectorAll(".tabbar button").forEach(b => b.setAttribute("aria-selected", b.dataset.go === name ? "true" : "false"));
  store.set("wqm-tab", name);
  if (name === "charts") renderCharts();
  window.scrollTo(0, 0);
}
document.querySelectorAll(".tabbar button").forEach(b => (b.onclick = () => showTab(b.dataset.go)));

/* ===== ค่าสด + กราฟเส้นเล็ก ===== */
const liveBuf = { ph: [], tds: [], temp: [] };
function drawSpark(k) {
  const el = $("spark-" + k), arr = liveBuf[k], ys = arr.filter(has);
  if (ys.length < 2) { el.innerHTML = ""; return; }
  let lo = Math.min(...ys), hi = Math.max(...ys);
  const minSpan = k === "ph" ? 0.2 : k === "tds" ? 10 : 0.5;
  if (hi - lo < minSpan) { const m = (hi + lo) / 2; lo = m - minSpan / 2; hi = m + minSpan / 2; }
  const n = 60, pts = [];
  arr.forEach((y, i) => { if (has(y)) pts.push([((i + n - arr.length) / (n - 1)) * 100, 28 - ((y - lo) / (hi - lo)) * 26]); });
  const line = pts.map(p => p[0].toFixed(1) + "," + p[1].toFixed(1)).join(" ");
  el.innerHTML = `<polygon points="${line} ${pts[pts.length - 1][0].toFixed(1)},30 ${pts[0][0].toFixed(1)},30"/><polyline points="${line}"/>`;
}
function setLive(v) {
  $("live-ph").textContent = f(v.ph, 2);
  $("live-tds").textContent = fTds(v.tds);
  $("live-temp").textContent = f(v.temp, 1);
  ["ph", "tds", "temp"].forEach(k => {
    liveBuf[k].push(has(v[k]) ? v[k] : null);
    if (liveBuf[k].length > 60) liveBuf[k].shift();
    drawSpark(k);
  });
}

/* ===== ประวัติ (เก็บในมือถือ) ===== */
let records = store.get("wqm-history", []);
const byTime = () => records.slice().sort((a, b) => a.time - b.time);

function renderHistory() {
  const opts = [`<option value="">ไม่ทราบ</option>`].concat(MODEL.classes.map(c => `<option value="${c.id}">${esc(c.name)}</option>`)).join("");
  $("hist-body").innerHTML = records.length ? byTime().reverse().map(h => {
    const a = analyze(h.values);
    const top = a.source ? a.source[0].name : "-";
    return `<tr class="clickable" data-key="${esc(h.key)}"><td>${esc(fmtRow(h.time))}${h.timeSource === "unknown" ? " *" : ""}</td><td>${esc(h.location || "-")}</td>
      <td class="n">${f(h.values.ph, 2)}</td><td class="n">${fTds(h.values.tds)}</td><td class="n">${f(h.values.temp, 1)}</td>
      <td><span class="pill s-${a.grade.st}">${a.score === null ? "–" : a.score}</span></td><td>${esc(top)}</td>
      <td><select data-key="${esc(h.key)}" aria-label="แหล่งน้ำจริง">${opts.replace(`value="${h.truth || ""}"`, `value="${h.truth || ""}" selected`)}</select></td></tr>`;
  }).join("") : `<tr><td colspan="8" class="muted">ยังไม่มีประวัติ ผลตรวจทุกครั้งจะถูกบันทึกไว้ที่นี่</td></tr>`;
  $("hist-body").querySelectorAll("select").forEach(sel => (sel.onchange = () => {
    const h = records.find(x => x.key === sel.dataset.key);
    if (h) { h.truth = sel.value; store.set("wqm-history", records); }
  }));
  $("hist-body").querySelectorAll("tr[data-key]").forEach(tr => (tr.onclick = e => {
    if (e.target.closest("select")) return;
    const h = records.find(x => x.key === tr.dataset.key);
    if (h) { showTab("measure"); renderReport(h); $("report").scrollIntoView({ behavior: "smooth", block: "start" }); }
  }));
}

function saveRecords() {
  if (records.length > 500) records.splice(0, records.length - 500);
  store.set("wqm-history", records);
}

function addRecord(rec) {
  if (!rec.example && !records.some(h => h.key === rec.key)) {
    records.push(rec);
    saveRecords();
  }
  renderReport(rec);
  renderHistory();
  renderCharts();
}

function toCsv() {
  const rows = [["time_iso", "location", "lat", "lon", "ph", "tds", "temp", "stable", "source_guess", "source_true"]];
  byTime().forEach(h => {
    const a = analyze(h.values);
    rows.push([h.timeSource === "unknown" ? "" : new Date(h.time).toISOString(), h.location || "", h.gps ? h.gps.lat : "", h.gps ? h.gps.lon : "",
      has(h.values.ph) ? h.values.ph : "", has(h.values.tds) ? h.values.tds : "", has(h.values.temp) ? h.values.temp : "",
      h.stable === false ? "no" : "yes", a.source ? a.source[0].id : "", h.truth || ""]);
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
  if (location.protocol === "http:" || location.protocol === "file:") {
    const dl = $("csv-dl");
    dl.href = URL.createObjectURL(new Blob(["﻿" + csv], { type: "text/csv" }));
    dl.hidden = false;
  }
};
$("clear-hist").onclick = () => ($("clear-confirm").hidden = false);
$("clear-no").onclick = () => ($("clear-confirm").hidden = true);
$("clear-yes").onclick = () => {
  records = []; store.set("wqm-history", records);
  $("clear-confirm").hidden = true; renderHistory(); renderCharts();
};

/* ===== กราฟ ===== */
function niceTicks(lo, hi, count) {
  const step0 = (hi - lo) / count, mag = Math.pow(10, Math.floor(Math.log10(step0))), err = step0 / mag;
  const step = (err >= 7.5 ? 10 : err >= 3.5 ? 5 : err >= 1.5 ? 2 : 1) * mag;
  const t = [];
  for (let v = Math.ceil(lo / step) * step; v <= hi + 1e-9; v += step) t.push(+v.toFixed(6));
  return t;
}

function lineChart(list, key, o) {
  const pts = list.map(r => ({ y: r.values[key], t: r.time, r }));
  const ys = pts.map(p => p.y).filter(has);
  if (!ys.length) return `<p class="muted" style="margin:0">ยังไม่มีข้อมูล${o.name}ในช่วงนี้</p>`;
  const W = 340, H = 170, L = 34, R = 38, T = 10, B = 24;
  let lo = Math.min(...ys, o.min), hi = Math.max(...ys, o.max);
  const pad = (hi - lo) * 0.06 || 1;
  lo = Math.max(o.floor !== undefined ? o.floor : -Infinity, lo - pad);
  hi += pad;
  const X = i => L + (pts.length === 1 ? (W - L - R) / 2 : (i * (W - L - R)) / (pts.length - 1));
  const Y = v => T + ((hi - v) / (hi - lo)) * (H - T - B);
  let s = `<svg class="chart" viewBox="0 0 ${W} ${H}" role="img" aria-label="กราฟ${esc(o.name)} ${pts.length} ครั้ง">`;
  s += `<g class="grid">` + niceTicks(lo, hi, 4).map(t =>
    `<line x1="${L}" x2="${W - R}" y1="${Y(t).toFixed(1)}" y2="${Y(t).toFixed(1)}"/><text x="${L - 6}" y="${(Y(t) + 3.5).toFixed(1)}" text-anchor="end">${+t.toFixed(o.dec)}</text>`).join("") + `</g>`;
  if (o.band) {
    const b0 = Math.max(o.band[0], lo), b1 = Math.min(o.band[1], hi);
    if (b1 > b0) s += `<rect class="bandr" x="${L}" y="${Y(b1).toFixed(1)}" width="${W - L - R}" height="${(Y(b0) - Y(b1)).toFixed(1)}"/>`;
  }
  let seg = [];
  const flush = () => { if (seg.length) s += `<polyline class="ln" style="stroke:${o.color}" points="${seg.join(" ")}"/>`; seg = []; };
  pts.forEach((p, i) => { if (has(p.y)) seg.push(`${X(i).toFixed(1)},${Y(p.y).toFixed(1)}`); else flush(); });
  flush();
  pts.forEach((p, i) => {
    if (!has(p.y)) return;
    const st = o.status(p.y);
    s += `<circle cx="${X(i).toFixed(1)}" cy="${Y(p.y).toFixed(1)}" r="4.5" style="fill:var(--${st})"><title>${esc(fmtTime(p.t))} · ${esc(o.fmt(p.y))}${p.r.location ? " · " + esc(p.r.location) : ""}</title></circle>`;
  });
  s += `<text class="xl" x="${L}" y="${H - 6}">${esc(fmtShort(pts[0].t))}</text>`;
  if (pts.length > 1) s += `<text class="xl" x="${W - R}" y="${H - 6}" text-anchor="end">${esc(fmtShort(pts[pts.length - 1].t))}</text>`;
  s += `<text class="xl" x="${(L + W - R) / 2}" y="${H - 6}" text-anchor="middle">${pts.length} ครั้ง</text>`;
  const li = pts.map(p => has(p.y)).lastIndexOf(true);
  s += `<text class="lastv" x="${(X(li) + 8).toFixed(1)}" y="${(Y(pts[li].y) + 4).toFixed(1)}" style="fill:${o.color}">${esc(o.fmt(pts[li].y))}</text>`;
  return s + `</svg>`;
}

function renderCharts() {
  const locs = [...new Set(records.map(r => r.location || "").filter(Boolean))];
  const cur = $("chart-loc").value;
  $("chart-loc").innerHTML = `<option value="">ทุกสถานที่</option>` + locs.map(l => `<option value="${esc(l)}"${l === cur ? " selected" : ""}>${esc(l)}</option>`).join("");
  const n = +$("chart-range").value;
  let list = byTime().filter(r => !cur || (r.location || "") === cur);
  if (n > 0) list = list.slice(-n);

  const phs = list.map(r => r.values.ph).filter(has), tdss = list.map(r => r.values.tds).filter(has);
  const both = list.filter(r => has(r.values.ph) && has(r.values.tds));
  const pass = both.filter(r => r.values.ph >= 6.5 && r.values.ph <= 8.5 && r.values.tds <= 500).length;
  const avg = a => a.reduce((x, y) => x + y, 0) / a.length;
  $("stats").innerHTML = [
    ["จำนวนครั้ง", String(list.length)],
    ["pH เฉลี่ย", phs.length ? avg(phs).toFixed(2) : "-"],
    ["TDS เฉลี่ย", tdss.length ? fTds(avg(tdss)) : "-"],
    ["ผ่านเกณฑ์น้ำดื่ม", both.length ? Math.round((pass / both.length) * 100) + "%" : "-"]
  ].map(([k, v]) => `<div class="stat"><span>${k}</span><b>${esc(v)}</b></div>`).join("");

  $("chart-ph").innerHTML = lineChart(list, "ph", { name: "pH", min: 6, max: 9, floor: 0, band: [6.5, 8.5], dec: 1,
    color: "var(--c-ph)", fmt: v => v.toFixed(2), status: v => analyzePh(v).st });
  $("chart-tds").innerHTML = lineChart(list, "tds", { name: "TDS", min: 0, max: 600, floor: 0, band: [0, 500], dec: 0,
    color: "var(--c-tds)", fmt: v => (tdsUnder(v) ? "<" + tdsMin() : String(Math.round(v))), status: v => analyzeTds(v).st });
  $("chart-temp").innerHTML = lineChart(list, "temp", { name: "อุณหภูมิ", min: 24, max: 33, band: [25, 32], dec: 0,
    color: "var(--c-temp)", fmt: v => v.toFixed(1), status: v => analyzeTemp(v).st });

  const cnt = {};
  let tot = 0;
  list.forEach(r => { const c = classify(r.values); if (c) { cnt[c[0].name] = (cnt[c[0].name] || 0) + 1; tot++; } });
  const rows = Object.entries(cnt).sort((a, b) => b[1] - a[1]);
  $("chart-src").innerHTML = tot
    ? `<div class="bars">${rows.map(([nm, c]) => `<div><span class="nm">${esc(nm)}</span><span class="track"><span class="fill" style="display:block;width:${((c / tot) * 100).toFixed(1)}%"></span></span><span class="pct">${c} ครั้ง</span></div>`).join("")}</div>`
    : `<p class="muted" style="margin:0">ยังไม่มีข้อมูล</p>`;
}
$("chart-range").onchange = renderCharts;
$("chart-loc").onchange = renderCharts;

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
let sawBusy = false;
let mode = "";

function setConn(kind, text) { $("conn-dot").className = "dot " + kind; $("conn-text").textContent = text; }

function setBusy(busy, sec, max) {
  $("busy").hidden = !busy;
  $("measure-btn").disabled = !!busy;
  $("measure-label").textContent = busy ? "กำลังวัด…" : "เริ่มวัด";
  if (busy) {
    $("busy-text").textContent = `${sec} วินาที · รอนานสุด ${max} วินาที`;
    $("busy-fill").style.width = Math.min(100, (sec / max) * 100) + "%";
  }
}

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

function recordFromDevice(it, live) {
  const known = it.epoch > 0;
  const rec = newRecord(it.values, {
    key: "dev-" + it.id, label: "#" + it.id,
    time: known ? it.epoch * 1000 : Date.now(),
    timeSource: known ? "device" : (live ? "phone" : "unknown"),
    stable: it.stable, wait: it.wait
  });
  if (!live) { rec.location = ""; rec.gps = null; }   // วัดตอนมือถือไม่ได้ดูอยู่ ไม่รู้สถานที่
  return rec;
}

async function poll() {
  try {
    const s = await getJson(apiBase + "/api/state");
    setConn("on", "เชื่อมต่อเครื่องวัดแล้ว");
    setLive(s.live);
    setBusy(s.busy, s.busy_s || 0, s.busy_max || 30);
    if (s.busy) sawBusy = true;
    if (!s.time_set) fetch(apiBase + "/api/time?epoch=" + Math.floor(Date.now() / 1000), { method: "POST" }).catch(() => {});
    if (s.last) {
      const key = "dev-" + s.last.id;
      if (key !== lastKey) {
        const fresh = lastKey !== null || sawBusy;     // เพิ่งวัดเสร็จตอนเปิดหน้านี้อยู่
        lastKey = key;
        sawBusy = false;
        const old = records.find(h => h.key === key);
        if (old) { if (fresh) renderReport(old); }
        else addRecord(recordFromDevice(s.last, fresh));
      }
    }
  } catch (e) {
    setConn("off", "ขาดการเชื่อมต่อ ตรวจว่าต่อ WiFi WaterMeter อยู่");
  }
  setTimeout(poll, 1000);
}

async function syncDevice() {
  if (apiBase === null) return;
  $("sync-msg").textContent = "กำลังดึงข้อมูล…";
  try {
    const d = await getJson(apiBase + "/api/log", {}, 6000);
    const items = d.items || [];
    let added = 0;
    items.forEach(it => {
      const old = records.find(h => h.key === "dev-" + it.id);
      if (old) {
        if (it.epoch > 0 && old.timeSource === "unknown") { old.time = it.epoch * 1000; old.timeSource = "device"; }
        return;
      }
      records.push(recordFromDevice(it, false));
      added++;
    });
    saveRecords();
    $("sync-msg").textContent = `เครื่องเก็บไว้ ${items.length} ครั้ง · ดึงมาเพิ่ม ${added} ครั้ง`;
    renderHistory();
    renderCharts();
  } catch (e) {
    $("sync-msg").textContent = "ดึงข้อมูลไม่สำเร็จ ลองใหม่อีกครั้ง";
  }
}

async function webMeasure() {
  if (apiBase === null) { demoSend(); return; }
  setBusy(true, 0, 30);
  try { await getJson(apiBase + "/api/measure", { method: "POST" }, 5000); } catch (e) { setBusy(false); setConn("off", "สั่งวัดไม่สำเร็จ"); }
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
  $("dev-sync").hidden = false;
  renderEmpty();
  poll();
  syncDevice();
}

function enterDemoMode() {
  if (mode === "demo") return;
  mode = "demo";
  setConn("demo", "โหมดทดลอง");
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
  if (manual) $("dev-msg").textContent = "ไม่พบเครื่องวัดที่ " + deviceAddr() + " ตรวจว่ามือถือต่อ WiFi WaterMeter อยู่";
  else if (canReachDevice) setTimeout(() => { if (mode === "demo") connect(false); }, 5000); // ลองหาใหม่เรื่อยๆ
}

/* ===== เริ่มทำงาน ===== */
$("model-info").textContent = `AI ประเมินแหล่งน้ำ: Naive Bayes รุ่น ${MODEL.version} · ข้อมูลที่ใช้: ${MODEL.trained_on}` +
  (MODEL.accuracy !== null ? ` · ความแม่นยำจากการทดสอบ ${Math.round(MODEL.accuracy * 100)}% (${MODEL.samples} ตัวอย่าง)` : " · ยังไม่ได้วัดความแม่นยำกับข้อมูลจริง");
renderHistory();
renderCharts();
$("measure-btn").onclick = webMeasure;
$("sync-dev").onclick = syncDevice;
$("clear-dev").onclick = async () => {
  if (apiBase === null || !confirm("ลบผลตรวจที่เก็บในเครื่องวัดทั้งหมด? (ประวัติในมือถือยังอยู่)")) return;
  try { await getJson(apiBase + "/api/log/clear", { method: "POST" }, 4000); $("sync-msg").textContent = "ล้างข้อมูลในเครื่องแล้ว"; }
  catch (e) { $("sync-msg").textContent = "ล้างไม่สำเร็จ"; }
};
$("dev-row").hidden = !canReachDevice;
$("dev-addr").value = deviceAddr();
$("dev-connect").onclick = () => { store.set("wqm-device", $("dev-addr").value); connect(true); };
showTab(store.get("wqm-tab", "measure"));

if (canReachDevice) { setConn("", "กำลังค้นหาเครื่องวัด…"); connect(false); }
else enterDemoMode();
</script>
</body>
</html>
)WQMHTML";
