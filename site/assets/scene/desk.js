// The desk: your laptop, the other computer's monitor, one keyboard and mouse.
import * as THREE from "three";
import { contactShadow, mat, panel, slab } from "./util.js";

function screenMesh(screen, w, h) {
  const m = new THREE.Mesh(new THREE.PlaneGeometry(w, h), new THREE.MeshBasicMaterial({ map: screen.texture, toneMapped: false }));
  const glow = new THREE.Mesh(new THREE.PlaneGeometry(w * 1.08, h * 1.1), new THREE.MeshBasicMaterial({
    color: 0x7aa7ff, transparent: true, opacity: 0, blending: THREE.AdditiveBlending, depthWrite: false }));
  glow.position.z = -0.002;
  return { m, glow };
}

function laptop(screen) {
  const g = new THREE.Group(), alu = mat(0xb9c0ca, 0.35, 0.55);
  g.add(new THREE.Mesh(slab(0.62, 0.42, 0.02, 0.02), alu));
  const deck = new THREE.Mesh(slab(0.5, 0.2, 0.002, 0.01, 0), mat(0x2a2f37, 0.8));
  deck.position.set(0, 0.02, -0.06);
  g.add(deck);
  const lid = new THREE.Group();
  lid.position.set(0, 0.02, -0.205);
  lid.rotation.x = -0.2;
  lid.add(new THREE.Mesh(panel(0.62, 0.4, 0.012, 0.018), alu));
  const s = screenMesh(screen, 0.57, 0.355);
  s.m.position.set(0, 0.2, 0.0135);
  s.glow.position.set(0, 0.2, 0.0125);
  lid.add(s.glow, s.m);
  g.add(lid, contactShadow(0.8, 0.6));
  g.position.set(-0.85, 0, 0.02);
  return { group: g, glow: s.glow };
}

function monitor(screen) {
  const g = new THREE.Group(), dark = mat(0x1d232c, 0.5, 0.2);
  const body = new THREE.Mesh(panel(0.86, 0.52, 0.03, 0.02), dark);
  body.position.y = 0.17;
  const s = screenMesh(screen, 0.82, 0.48);
  s.m.position.set(0, 0.43, 0.031);
  s.glow.position.set(0, 0.43, 0.03);
  const neck = new THREE.Mesh(new THREE.BoxGeometry(0.05, 0.2, 0.025), dark);
  neck.position.set(0, 0.1, -0.02);
  const foot = new THREE.Mesh(slab(0.3, 0.18, 0.012, 0.03), dark);
  foot.position.z = 0.0;
  g.add(body, neck, foot, s.glow, s.m, contactShadow(0.6, 0.35));
  g.position.set(0.76, 0, -0.32);
  return { group: g, glow: s.glow };
}

function keyboard() {
  const g = new THREE.Group();
  g.add(new THREE.Mesh(slab(0.56, 0.19, 0.016, 0.015), mat(0xece8de, 0.7)));
  const cap = new THREE.BoxGeometry(0.03, 0.01, 0.03), keys = new THREE.InstancedMesh(cap, mat(0x3a404a, 0.7), 59);
  const m4 = new THREE.Matrix4();
  let n = 0;
  for (let row = 0; row < 4; row++) for (let col = 0; col < 15; col++) {
    if (row === 0 && col === 13) continue; // F12 is its own key
    m4.setPosition(-0.252 + col * 0.036, 0.021, -0.058 + row * 0.04);
    keys.setMatrixAt(n++, m4);
  }
  const f12 = new THREE.Mesh(cap, new THREE.MeshStandardMaterial({ color: 0x4f8cff, emissive: 0x7aa7ff, emissiveIntensity: 0.2, roughness: 0.5 }));
  f12.position.set(-0.252 + 13 * 0.036, 0.021, -0.058);
  g.add(keys, f12, contactShadow(0.7, 0.32, 0.35));
  g.position.set(-0.3, 0, 0.33);
  return { group: g, f12 };
}

function mouse() {
  const m = new THREE.Mesh(new THREE.SphereGeometry(0.04, 20, 12), mat(0xece8de, 0.5));
  m.scale.set(0.75, 0.42, 1.15);
  m.position.set(0.14, 0.012, 0.34);
  const s = contactShadow(0.14, 0.18, 0.35);
  s.position.set(0.14, 0.001, 0.34);
  return [m, s];
}

export function buildDesk(scene, screens) {
  const top = new THREE.Mesh(slab(2.5, 1.15, 0.05, 0.04), mat(0x3a2b22, 0.75));
  top.position.y = -0.05;
  const lap = laptop(screens.mine), mon = monitor(screens.other), kb = keyboard();
  scene.add(top, lap.group, mon.group, kb.group, ...mouse());
  return { glowMine: lap.glow, glowOther: mon.glow, f12: kb.f12 };
}
