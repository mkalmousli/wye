// The demo itself: typing, the mouse pointer, the F12 key and which computer is "live".
import * as THREE from "three";

const SCRIPT = {
  mine: ["Meeting notes", "- ship Wye 1.0", "- order more Picos"],
  other: ["$ ssh build-02", "$ ./deploy --prod", "deployed in 41 s"],
};

export function makeSwitch({ desk, picos, screens, drawMine, drawOther, motion }) {
  let remote = false, auto = 0, idle = 99, press = 0, t = 0, redraw = 0;
  const text = { mine: { lines: [""], pos: [0, 0], wait: 0 }, other: { lines: [""], pos: [0, 0], wait: 0 } };

  function type(s, script, dt) {
    if ((s.wait -= dt) > 0) return;
    const [line, ch] = s.pos;
    if (line >= script.length) { s.lines = [""]; s.pos = [0, 0]; s.wait = 1.2; return; }
    if (ch < script[line].length) { s.lines[line] = script[line].slice(0, ch + 1); s.pos = [line, ch + 1]; s.wait = 0.07; }
    else { s.lines.push(""); s.pos = [line + 1, 0]; s.wait = 0.45; }
  }
  return {
    anchors: { mine: new THREE.Vector3(-0.85, 0.48, -0.22), other: new THREE.Vector3(0.76, 0.74, -0.3) },
    toggle() { remote = !remote; idle = 0; press = 0.3; },
    /** Returns whether keys currently go to the other computer. */
    update(dt, ch) {
      t += dt; idle += dt; press = Math.max(0, press - dt);
      if (ch === 0 && motion && idle > 10 && (auto += dt) > 4.2) { auto = 0; remote = !remote; press = 0.3; }
      const live = ch === 0 ? remote : true;
      type(live ? text.other : text.mine, live ? SCRIPT.other : SCRIPT.mine, dt);
      desk.glowMine.material.opacity += ((live ? 0 : 0.5) - desk.glowMine.material.opacity) * Math.min(1, dt * 6);
      desk.glowOther.material.opacity += ((live ? 0.5 : 0) - desk.glowOther.material.opacity) * Math.min(1, dt * 6);
      desk.f12.position.y = 0.021 - (press > 0 ? 0.006 : 0);
      desk.f12.material.emissiveIntensity = press > 0 ? 2.2 : 0.25;
      picos.leds.forEach((l, i) => l.material.color.setHex(live && Math.sin(t * 20 + i) > 0 ? 0xffffff : 0x5ee38b));
      if ((redraw -= dt) <= 0) {
        redraw = 1 / 15;
        const cur = [320 + Math.sin(t * 0.9) * 200, 230 + Math.sin(t * 1.4) * 90];
        drawMine(screens.mine, { active: !live, lines: text.mine.lines, cursor: cur });
        drawOther(screens.other, { active: live, lines: text.other.lines, cursor: cur, t: t * 1000,
          mode: ch === 2 ? "bios" : ch === 3 ? "call" : "desk" });
      }
      return live;
    },
  };
}
