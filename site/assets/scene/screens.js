// What the two screens show, drawn into small canvases (redrawn ~15 times a second).
import * as THREE from "three";

const W = 640, H = 400, MONO = "500 22px 'JetBrains Mono', ui-monospace, monospace";

export function makeScreen() {
  const canvas = document.createElement("canvas");
  canvas.width = W;
  canvas.height = H;
  const texture = new THREE.CanvasTexture(canvas);
  texture.colorSpace = THREE.SRGBColorSpace;
  texture.anisotropy = 4;
  return { canvas, ctx: canvas.getContext("2d"), texture };
}

function windowFrame(g, title, bg) {
  g.fillStyle = bg;
  g.fillRect(0, 0, W, H);
  g.fillStyle = "rgba(255,255,255,.06)";
  g.fillRect(0, 0, W, 40);
  ["#ff5f57", "#febc2e", "#28c840"].forEach((c, i) => {
    g.fillStyle = c;
    g.beginPath();
    g.arc(24 + i * 22, 20, 7, 0, 7);
    g.fill();
  });
  g.fillStyle = "#8b98ab";
  g.font = "500 17px Inter, sans-serif";
  g.fillText(title, 100, 26);
}

function typed(g, lines, x, y, color, caret) {
  g.font = MONO;
  g.fillStyle = color;
  lines.forEach((l, i) => g.fillText(l, x, y + i * 34));
  if (caret) {
    const last = lines[lines.length - 1];
    g.fillStyle = "#7aa7ff";
    g.fillRect(x + g.measureText(last).width + 3, y + (lines.length - 1) * 34 - 20, 12, 26);
  }
}

function cursor(g, x, y) {
  g.fillStyle = "#fff";
  g.strokeStyle = "#000";
  g.lineWidth = 2;
  g.beginPath();
  [[0, 0], [0, 30], [8, 23], [14, 36], [19, 34], [13, 21], [24, 21]].forEach(([dx, dy], i) =>
    (i ? g.lineTo : g.moveTo).call(g, x + dx, y + dy));
  g.closePath();
  g.fill();
  g.stroke();
}

function bios(g, t) {
  g.fillStyle = "#0b1fa8";
  g.fillRect(0, 0, W, H);
  g.font = "700 24px 'JetBrains Mono', monospace";
  g.fillStyle = "#e7ecff";
  g.fillText("BIOS SETUP UTILITY", 170, 50);
  const items = ["Main", "Boot order", "Secure Boot", "Save & Exit"];
  items.forEach((s, i) => {
    const on = i === Math.floor(t / 900) % items.length;
    g.fillStyle = on ? "#e7ecff" : "transparent";
    g.fillRect(60, 92 + i * 56, 520, 42);
    g.fillStyle = on ? "#0b1fa8" : "#e7ecff";
    g.fillText(s, 80, 121 + i * 56);
  });
}

function call(g, t) {
  windowFrame(g, "Team call", "#111821");
  g.fillStyle = "#4f8cff";
  g.beginPath();
  g.arc(W / 2, 160, 58, 0, 7);
  g.fill();
  g.fillStyle = "#111821";
  g.font = "700 46px Inter, sans-serif";
  g.fillText("JM", W / 2 - 34, 177);
  for (let i = 0; i < 24; i++) {
    const h = 8 + Math.abs(Math.sin(t / 160 + i * 0.7)) * 46;
    g.fillStyle = "#12a150";
    g.fillRect(170 + i * 13, 300 - h / 2, 8, h);
  }
}

/** state: {active, lines, mode: "desk" | "bios" | "call", cursor: [x, y], t} */
export function drawMine(s, state) {
  windowFrame(s.ctx, "notes.txt  ·  your computer", "#141b25");
  typed(s.ctx, state.lines, 34, 92, "#d7e3f4", state.active);
  if (state.active) cursor(s.ctx, ...state.cursor);
  s.texture.needsUpdate = true;
}

export function drawOther(s, state) {
  if (state.mode === "bios") bios(s.ctx, state.t);
  else if (state.mode === "call") call(s.ctx, state.t);
  else {
    windowFrame(s.ctx, "terminal  ·  other computer", "#0c1117");
    typed(s.ctx, state.lines, 34, 92, "#9be3a6", state.active);
  }
  if (state.active && state.mode !== "bios") cursor(s.ctx, ...state.cursor);
  s.texture.needsUpdate = true;
}
