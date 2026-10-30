// Scroll-driven camera: each chapter of the story has a viewpoint; scrolling
// glides between them. Captions and 3D labels follow the nearest chapter.
import * as THREE from "three";

const v = (x, y, z) => new THREE.Vector3(x, y, z);
// [camera position, look-at] per chapter: overview, the Picos, the other screen, overview.
const KEYS = [
  [v(0.4, 1.4, 2.7), v(0.08, 0.1, -0.12)],
  [v(-0.08, 0.62, 0.66), v(-0.01, 0.07, -0.27)],
  [v(0.12, 0.58, 1.05), v(0.58, 0.34, -0.3)],
  [v(-0.25, 1.3, 2.55), v(0.1, 0.12, -0.1)],
];
const ease = (x) => x * x * (3 - 2 * x);

export function makeStory(camera, section) {
  const pos = new THREE.Vector3(), look = new THREE.Vector3(), tmp = new THREE.Vector3();
  let sway = 0;
  return {
    /** Chapter position 0..3 (fractional) from the page scroll. */
    progress() {
      const r = section.getBoundingClientRect(), span = r.height - innerHeight;
      return Math.min(3, Math.max(0, (-r.top / Math.max(1, span)) * 3.999));
    },
    place(c, dt, pointer, motion, portrait) {
      const i = Math.min(2, Math.floor(c)), f = ease(Math.min(1, c - i));
      pos.lerpVectors(KEYS[i][0], KEYS[i + 1][0], f);
      look.lerpVectors(KEYS[i][1], KEYS[i + 1][1], f);
      if (portrait) pos.sub(look).multiplyScalar(c < 0.5 || c > 2.5 ? 2.3 : 1.55).add(look);
      if (motion) {
        sway += dt;
        pos.x += Math.sin(sway * 0.3) * 0.04 + pointer.x * 0.08;
        pos.y += pointer.y * 0.05;
      }
      camera.position.copy(pos);
      camera.lookAt(look);
    },
    /** Moves the HTML labels onto their 3D anchors. */
    labels(list, w, h) {
      for (const { el, at } of list) {
        tmp.copy(at).project(camera);
        el.style.transform = `translate(${((tmp.x + 1) / 2) * w}px, ${((1 - tmp.y) / 2) * h}px)`;
      }
    },
  };
}
