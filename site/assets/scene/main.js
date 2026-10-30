// Wye's 3D story: one desk, two computers, and the switch between them.
// Loaded after the page is interactive; until then (or without WebGL) a poster shows.
import * as THREE from "three";
import { buildDesk } from "./desk.js";
import { makeFlow } from "./flow.js";
import { buildPicos } from "./picos.js";
import { drawMine, drawOther, makeScreen } from "./screens.js";
import { makeStory } from "./story.js";
import { makeSwitch } from "./switch.js";

const $ = (s) => document.querySelector(s);
const canvas = $("#scene"), stage = $("#stage"), section = $("#story");
const motion = !matchMedia("(prefers-reduced-motion: reduce)").matches;

function renderer() {
  const r = new THREE.WebGLRenderer({ canvas, alpha: true, antialias: devicePixelRatio < 2, powerPreference: "high-performance" });
  r.setPixelRatio(Math.min(devicePixelRatio, innerWidth < 700 ? 1.3 : 1.6));
  r.toneMapping = THREE.ACESFilmicToneMapping;
  r.outputColorSpace = THREE.SRGBColorSpace;
  return r;
}

function lights(scene) {
  const sun = new THREE.DirectionalLight(0xfff1dd, 2.4);
  sun.position.set(1.4, 3, 2.2);
  const wire = new THREE.PointLight(0x7aa7ff, 0.9, 1.2);
  wire.position.set(0, 0.35, -0.2);
  scene.add(new THREE.HemisphereLight(0xdfe8ff, 0x2a1d14, 1.2), sun, wire);
}

try {
  const gl = renderer(), scene = new THREE.Scene(), camera = new THREE.PerspectiveCamera(35, 1, 0.05, 20);
  lights(scene);
  const screens = { mine: makeScreen(), other: makeScreen() };
  const desk = buildDesk(scene, screens), picos = buildPicos(scene);
  const flow = makeFlow(scene, picos), story = makeStory(camera, section);
  const sw = makeSwitch({ desk, picos, screens, drawMine, drawOther, motion });
  const labels = [...document.querySelectorAll("[data-label]")].map((el) => ({
    el, at: picos.anchors[el.dataset.label] || sw.anchors[el.dataset.label], chapters: el.dataset.ch.split(",").map(Number) }));
  const pointer = { x: 0, y: 0 };
  let w = 0, h = 0, portrait = false, last = performance.now(), visible = true, frame = 0;

  function resize() {
    w = stage.clientWidth; h = stage.clientHeight; portrait = w / h < 1.15;
    gl.setSize(w, h, false);
    camera.aspect = w / h;
    // Room for the captions: on the left on wide screens, below the scene on tall ones.
    if (portrait) camera.setViewOffset(w, h, 0, h * 0.13, w, h);
    else camera.setViewOffset(w, h, -w * 0.17, 0, w, h);
    camera.updateProjectionMatrix();
  }
  function tick(now) {
    frame = 0;
    const dt = Math.min(0.05, Math.max(0, (now - last) / 1000));
    last = now;
    const c = story.progress(), ch = Math.round(c);
    const remote = sw.update(dt, ch);
    flow.update(motion ? dt : 0, remote, ch === 3);
    story.place(c, dt, pointer, motion, portrait);
    document.querySelectorAll("[data-ch-caption]").forEach((el) => el.classList.toggle("on", +el.dataset.chCaption === ch));
    const shown = labels.filter((l) => l.chapters.includes(ch));
    labels.forEach((l) => l.el.classList.toggle("on", shown.includes(l)));
    gl.render(scene, camera);
    story.labels(shown, w, h);
    if (!canvas.classList.contains("ready")) canvas.classList.add("ready");
    schedule();
  }
  function schedule() {
    if (!frame && visible && !document.hidden) frame = requestAnimationFrame(tick);
  }
  addEventListener("resize", resize);
  addEventListener("pointermove", (e) => { pointer.x = e.clientX / innerWidth * 2 - 1; pointer.y = e.clientY / innerHeight * 2 - 1; });
  document.addEventListener("visibilitychange", () => { last = performance.now(); schedule(); });
  new IntersectionObserver(([e]) => { visible = e.isIntersecting; last = performance.now(); schedule(); }).observe(stage);
  addEventListener("keydown", (e) => {
    if ((e.key === "F12" || e.key === "h") && section.getBoundingClientRect().top > -innerHeight * 0.5) {
      e.preventDefault();
      sw.toggle();
    }
  });
  $("#switchKey")?.addEventListener("click", () => sw.toggle());
  if (location.hash === "#debug") window.__wye = gl;
  resize();
  schedule();
} catch (e) {
  document.documentElement.classList.add("no3d");
  console.warn("3D scene unavailable, showing the poster", e);
}
