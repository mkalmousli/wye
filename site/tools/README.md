Not part of the website. `shoot.cjs` screenshots the 3D story (each chapter,
desktop and phone) and reports errors and frame rate, using Playwright's
headless Chromium:

    npm i playwright@1.49.1 && npx playwright install --only-shell chromium
    python3 -m http.server 8768 &   # from the site directory
    node tools/shoot.cjs http://localhost:8768/ /tmp/shots
