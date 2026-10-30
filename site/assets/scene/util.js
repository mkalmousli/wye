// Small geometry and texture helpers shared by the scene modules.
import * as THREE from "three";

/** A slab with rounded corners: footprint w x d on the floor, h tall, bottom at y = 0. */
export function slab(w, d, h, r, bevel = 0.003) {
  const s = new THREE.Shape(), x = -w / 2, y = -d / 2;
  s.moveTo(x + r, y);
  s.lineTo(x + w - r, y);
  s.quadraticCurveTo(x + w, y, x + w, y + r);
  s.lineTo(x + w, y + d - r);
  s.quadraticCurveTo(x + w, y + d, x + w - r, y + d);
  s.lineTo(x + r, y + d);
  s.quadraticCurveTo(x, y + d, x, y + d - r);
  s.lineTo(x, y + r);
  s.quadraticCurveTo(x, y, x + r, y);
  const g = new THREE.ExtrudeGeometry(s, {
    depth: Math.max(0.0005, h - 2 * bevel), bevelEnabled: bevel > 0, bevelThickness: bevel,
    bevelSize: bevel, bevelSegments: 2, curveSegments: 5,
  });
  g.rotateX(-Math.PI / 2);
  g.translate(0, bevel, 0);
  return g;
}

/** Same slab standing up: w wide, d tall (bottom at y = 0), h thick towards +z. */
export function panel(w, d, h, r) {
  const g = slab(w, d, h, r);
  g.rotateX(Math.PI / 2);
  g.translate(0, d / 2, 0);
  return g;
}

export const mat = (color, rough = 0.6, metal = 0) =>
  new THREE.MeshStandardMaterial({ color, roughness: rough, metalness: metal });

/** Soft round glow, used for data dots and the contact shadows. */
export function radialTexture(stops) {
  const c = document.createElement("canvas");
  c.width = c.height = 64;
  const g = c.getContext("2d"), grd = g.createRadialGradient(32, 32, 0, 32, 32, 32);
  stops.forEach(([at, color]) => grd.addColorStop(at, color));
  g.fillStyle = grd;
  g.fillRect(0, 0, 64, 64);
  const t = new THREE.CanvasTexture(c);
  t.colorSpace = THREE.SRGBColorSpace;
  return t;
}

/** A blurred shadow under an object: cheap stand-in for real shadow maps. */
export function contactShadow(w, d, opacity = 0.45) {
  const tex = radialTexture([[0, "rgba(0,0,0,1)"], [0.55, "rgba(0,0,0,.55)"], [1, "rgba(0,0,0,0)"]]);
  const m = new THREE.Mesh(new THREE.PlaneGeometry(w, d),
    new THREE.MeshBasicMaterial({ map: tex, transparent: true, opacity, depthWrite: false }));
  m.rotation.x = -Math.PI / 2;
  m.position.y = 0.001;
  return m;
}

export function tube(points, radius, color, segments = 64) {
  const curve = new THREE.CatmullRomCurve3(points.map((p) => new THREE.Vector3(...p)));
  const mesh = new THREE.Mesh(new THREE.TubeGeometry(curve, segments, radius, 8, false), mat(color, 0.45));
  return { curve, mesh };
}
