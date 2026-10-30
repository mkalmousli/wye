// The two Raspberry Pi Picos (shown about 5x life size), their USB cables and
// the three-wire link that arches between them.
import * as THREE from "three";
import { contactShadow, mat, slab, tube } from "./util.js";

const L = 0.255, W = 0.105; // board footprint

function board(label) {
  const g = new THREE.Group();
  g.add(new THREE.Mesh(slab(L, W, 0.012, 0.01, 0.002), mat(0x17633a, 0.55)));
  const chip = new THREE.Mesh(new THREE.BoxGeometry(0.05, 0.008, 0.05), mat(0x1b1d21, 0.4));
  chip.position.set(0.01, 0.016, 0);
  const usb = new THREE.Mesh(new THREE.BoxGeometry(0.04, 0.02, 0.036), mat(0xc9cfd6, 0.25, 0.8));
  usb.position.set(-L / 2 + 0.012, 0.021, 0);
  const led = new THREE.Mesh(new THREE.BoxGeometry(0.008, 0.004, 0.005),
    new THREE.MeshBasicMaterial({ color: 0x5ee38b }));
  led.position.set(-L / 2 + 0.05, 0.014, -0.03);
  const pin = new THREE.CylinderGeometry(0.0045, 0.0045, 0.004, 10);
  const pins = new THREE.InstancedMesh(pin, mat(0xd8b04e, 0.3, 0.9), 40);
  const m4 = new THREE.Matrix4();
  for (let i = 0; i < 40; i++) {
    m4.setPosition(-L / 2 + 0.0085 + (i % 20) * 0.0127, 0.0125, (i < 20 ? 1 : -1) * (W / 2 - 0.006));
    pins.setMatrixAt(i, m4);
  }
  g.add(chip, usb, led, pins, contactShadow(L * 1.4, W * 1.8, 0.4));
  g.name = label;
  return { group: g, led };
}

export function buildPicos(scene) {
  const bridge = board("bridge"), target = board("target");
  bridge.group.position.set(-0.2, 0, -0.27);
  target.group.position.set(0.2, 0, -0.27);
  target.group.rotation.y = Math.PI; // USB end faces the monitor
  const usbA = tube([[-0.345, 0.02, -0.27], [-0.42, 0.008, -0.23], [-0.5, 0.008, -0.12], [-0.54, 0.012, -0.02]], 0.007, 0x2b3038);
  const usbB = tube([[0.345, 0.02, -0.27], [0.47, 0.008, -0.31], [0.6, 0.008, -0.3], [0.68, 0.012, -0.3]], 0.007, 0x2b3038);
  // TX, RX and ground: leave the bridge's pins 1-3, arc over both boards, land on the target's.
  const wires = [0xe4572e, 0x2a7de1, 0x23272e].map((color, i) => {
    const dz = 0.012 * (i - 1);
    return tube([[-0.31 + i * 0.0127, 0.016, -0.226], [-0.3, 0.12, -0.25 + dz], [-0.12, 0.24, -0.27 + dz],
      [0.12, 0.24, -0.27 + dz], [0.3, 0.12, -0.29 + dz], [0.31 - i * 0.0127, 0.016, -0.314]], 0.0055, color, 96);
  });
  scene.add(bridge.group, target.group, usbA.mesh, usbB.mesh, ...wires.map((w) => w.mesh));

  const v = (x, y, z) => new THREE.Vector3(x, y, z);
  // Keys travel: keyboard -> laptop -> USB -> bridge -> wires -> target -> USB -> monitor.
  const out = new THREE.CatmullRomCurve3([
    v(-0.3, 0.05, 0.33), v(-0.48, 0.09, 0.16), ...usbA.curve.points.slice().reverse().map((p) => p.clone().setY(p.y + 0.012)),
    ...wires[1].curve.points.map((p) => p.clone().setY(p.y + 0.012)),
    ...usbB.curve.points.map((p) => p.clone().setY(p.y + 0.012)), v(0.76, 0.12, -0.33), v(0.76, 0.4, -0.27),
  ]);
  // Before the switch, keys only reach the laptop.
  const local = new THREE.CatmullRomCurve3([v(-0.3, 0.05, 0.33), v(-0.55, 0.16, 0.1), v(-0.85, 0.22, -0.18)]);
  return { out, local, leds: [bridge.led, target.led],
    anchors: { bridge: v(-0.2, 0.03, -0.27), target: v(0.2, 0.03, -0.27), wires: v(0, 0.26, -0.27) } };
}
