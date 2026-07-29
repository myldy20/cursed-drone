import { chromium } from 'playwright';
import crypto from 'node:crypto';
import fs from 'node:fs/promises';

const url = process.env.CURSED_DRONE_WEB_URL || 'http://127.0.0.1:4173';
const logicalWidth = 1496;
const logicalHeight = 672;
const outputDir = process.env.CURSED_DRONE_SMOKE_OUTPUT || 'dist/web-smoke';

await fs.mkdir(outputDir, { recursive: true });

function logicalPoint(box, x, y) {
  const scale = Math.min(box.width / logicalWidth, box.height / logicalHeight);
  return {
    x: box.x + (box.width - logicalWidth * scale) * 0.5 + x * scale,
    y: box.y + (box.height - logicalHeight * scale) * 0.5 + y * scale,
  };
}

function digest(buffer) {
  return crypto.createHash('sha256').update(buffer).digest('hex');
}

async function canvasDigest(page) {
  return digest(await page.locator('#canvas').screenshot());
}

async function requireCanvasChange(page, before, label) {
  await page.waitForTimeout(220);
  const after = await canvasDigest(page);
  if (after === before) throw new Error(`${label}: canvas did not change`);
}

async function clickLogical(page, x, y, touch) {
  const box = await page.locator('#canvas').boundingBox();
  if (!box) throw new Error('canvas has no bounding box');
  const point = logicalPoint(box, x, y);
  if (touch) await page.touchscreen.tap(point.x, point.y);
  else await page.mouse.click(point.x, point.y);
}

async function dragLogical(page, fromX, fromY, toX, toY) {
  const box = await page.locator('#canvas').boundingBox();
  if (!box) throw new Error('canvas has no bounding box');
  const from = logicalPoint(box, fromX, fromY);
  const to = logicalPoint(box, toX, toY);
  await page.mouse.move(from.x, from.y);
  await page.mouse.down();
  await page.mouse.move(to.x, to.y, { steps: 12 });
  await page.mouse.up();
}

async function runCase(browser, testCase) {
  const context = await browser.newContext({
    viewport: testCase.viewport,
    deviceScaleFactor: testCase.dpr,
    hasTouch: testCase.touch,
    isMobile: testCase.mobile,
  });
  const page = await context.newPage();
  const errors = [];
  page.on('pageerror', (error) => errors.push(`pageerror: ${error.message}`));
  page.on('console', (message) => {
    if (message.type() === 'error') errors.push(`console: ${message.text()}`);
  });

  await page.goto(url, { waitUntil: 'networkidle', timeout: 60000 });
  await page.locator('#start').waitFor({ state: 'visible', timeout: 60000 });
  await page.waitForFunction(() => !document.querySelector('#start')?.disabled, null, { timeout: 60000 });
  await page.locator('#start').click();
  await page.waitForFunction(() => document.querySelector('#gate')?.classList.contains('hidden'), null, { timeout: 15000 });
  await page.waitForTimeout(1200);

  const fatalVisible = await page.locator('#fatal').evaluate((node) => getComputedStyle(node).display !== 'none');
  if (fatalVisible) throw new Error(`startup fatal: ${await page.locator('#fatal').textContent()}`);

  // Top navigation centres in the fixed 1496x672 logical viewport. Requiring
  // a canvas change after every click catches the Retina/letterbox regression
  // where the rendered tab and its effective hit location diverged.
  const tabs = [
    ['actor', 457, 87],
    ['fx', 752, 87],
    ['master', 1047, 87],
    ['memory', 1342, 87],
    ['place', 162, 87],
  ];
  for (const [label, x, y] of tabs) {
    const before = await canvasDigest(page);
    await clickLogical(page, x, y, testCase.touch);
    await requireCanvasChange(page, before, `${testCase.name}/${label}`);
  }

  if (!testCase.touch) {
    // First Place macro row. Moving across it verifies SDL mouse-to-touch drag
    // routing as well as tab clicks.
    const before = await canvasDigest(page);
    await dragLogical(page, 360, 265, 790, 265);
    await requireCanvasChange(page, before, `${testCase.name}/drag`);
  }

  await page.screenshot({ path: `${outputDir}/${testCase.name}.png`, fullPage: true });
  if (errors.length) throw new Error(`${testCase.name}: ${errors.join('\n')}`);
  await context.close();
}

const browser = await chromium.launch({
  headless: true,
  args: [
    '--autoplay-policy=no-user-gesture-required',
    '--disable-features=AudioServiceOutOfProcess',
  ],
});

try {
  for (const testCase of [
    { name: 'desktop-dpr1', viewport: { width: 1496, height: 672 }, dpr: 1, touch: false, mobile: false },
    { name: 'retina-letterbox', viewport: { width: 1440, height: 900 }, dpr: 2, touch: false, mobile: false },
    { name: 'mobile-touch', viewport: { width: 844, height: 390 }, dpr: 2, touch: true, mobile: true },
  ]) {
    await runCase(browser, testCase);
    console.log(`PASS ${testCase.name}`);
  }
} finally {
  await browser.close();
}
