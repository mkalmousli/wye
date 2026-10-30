// Glowing dots that make data visible: blue keystrokes travelling out, green
// sound coming back. Sprites with additive blending: no lights, no post-processing.
import * as THREE from "three";
import { radialTexture } from "./util.js";

function dots(scene, n, texture, size) {
  const material = new THREE.SpriteMaterial({ map: texture, blending: THREE.AdditiveBlending, depthWrite: false, transparent: true });
  return Array.from({ length: n }, (_, i) => {
    const s = new THREE.Sprite(material);
    s.scale.setScalar(size);
    s.userData.t = i / n;
    scene.add(s);
    return s;
  });
}

export function makeFlow(scene, paths) {
  const blue = radialTexture([[0, "rgba(225,235,255,1)"], [0.25, "rgba(122,167,255,.95)"], [1, "rgba(122,167,255,0)"]]);
  const green = radialTexture([[0, "rgba(220,255,230,1)"], [0.25, "rgba(18,161,80,.9)"], [1, "rgba(18,161,80,0)"]]);
  const keys = dots(scene, 7, blue, 0.06), sound = dots(scene, 5, green, 0.05);
  let fade = 0; // 0: keys stay local, 1: keys cross over

  return {
    /** remote: keys go to the other computer; audio: sound flows back. */
    update(dt, remote, audio) {
      fade += ((remote ? 1 : 0) - fade) * Math.min(1, dt * 4);
      keys.forEach((s) => {
        s.userData.t = (((s.userData.t + dt * (remote ? 0.2 : 0.55)) % 1) + 1) % 1;
        (remote ? paths.out : paths.local).getPointAt(s.userData.t, s.position);
        s.material.opacity = 1;
      });
      sound.forEach((s) => {
        s.visible = audio;
        if (!audio) return;
        s.userData.t = (((s.userData.t + dt * 0.16) % 1) + 1) % 1;
        paths.out.getPointAt(1 - s.userData.t, s.position);
        s.position.y += 0.02;
      });
      return fade;
    },
  };
}
