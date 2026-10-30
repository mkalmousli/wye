// node shoot.cjs <url> <outdir>: screenshots of each story chapter + errors + fps.
const { chromium } = require("playwright");
const [url, out] = process.argv.slice(2);
(async () => {
  const browser = await chromium.launch({ args: ["--use-angle=swiftshader", "--enable-unsafe-swiftshader", "--ignore-gpu-blocklist"] });
  for (const [name, vp] of [["desk", { width: 1440, height: 900 }], ["phone", { width: 390, height: 844 }]]) {
    const page = await browser.newPage({ viewport: vp, deviceScaleFactor: 1 });
    const errors = [];
    page.on("console", (m) => (m.type() === "error" || m.type() === "warning") && errors.push(m.text()));
    page.on("pageerror", (e) => errors.push(String(e)));
    await page.goto(url, { waitUntil: "load" });
    const ready = await page.waitForSelector("#scene.ready", { timeout: 20000 }).then(() => true, () => false);
    const fps = await page.evaluate(() => new Promise((r) => { let n = 0; const t0 = performance.now();
      const f = () => (++n, performance.now() - t0 < 2000 ? requestAnimationFrame(f) : r(Math.round(n / 2))); requestAnimationFrame(f); }));
    console.log(name, "3D ready:", ready, "fps:", fps, "no3d:", await page.evaluate(() => document.documentElement.classList.contains("no3d")));
    const story = await page.evaluate(() => { const s = document.getElementById("story"); return { top: s.offsetTop, h: s.offsetHeight }; });
    for (const ch of [0, 1, 2, 3]) {
      const y = story.top + (story.h - vp.height) * (ch / 3.999) + (ch ? 2 : 0);
      await page.evaluate((y) => window.scrollTo(0, y), y);
      await page.waitForTimeout(1600);
      await page.screenshot({ path: `${out}/${name}-ch${ch}.png` });
    }
    await page.evaluate(() => window.scrollTo(0, document.body.scrollHeight));
    await page.waitForTimeout(400);
    await page.screenshot({ path: `${out}/${name}-full.png`, fullPage: true });
    console.log(name, "errors:", errors.length ? errors : "none");
    await page.close();
  }
  await browser.close();
})();
