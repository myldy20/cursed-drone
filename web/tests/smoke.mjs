import { chromium } from 'playwright';
import crypto from 'node:crypto';
import fs from 'node:fs/promises';
import { PNG } from 'pngjs';

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

function logicalLocalPoint(box, x, y) {
  const scale = Math.min(box.width / logicalWidth, box.height / logicalHeight);
  return {
    x: (box.width - logicalWidth * scale) * 0.5 + x * scale,
    y: (box.height - logicalHeight * scale) * 0.5 + y * scale,
  };
}

function digest(buffer) {
  return crypto.createHash('sha256').update(buffer).digest('hex');
}

async function canvasImage(page) {
  const locator = page.locator('#canvas');
  const box = await locator.boundingBox();
  if (!box) throw new Error('canvas has no bounding box');
  const png = PNG.sync.read(await locator.screenshot());
  return { box, png };
}

function pixelAtLogical(image, logicalX, logicalY) {
  const local = logicalLocalPoint(image.box, logicalX, logicalY);
  const x = Math.max(0, Math.min(image.png.width - 1,
    Math.round(local.x * image.png.width / image.box.width)));
  const y = Math.max(0, Math.min(image.png.height - 1,
    Math.round(local.y * image.png.height / image.box.height)));
  const offset = (y * image.png.width + x) * 4;
  return Array.from(image.png.data.subarray(offset, offset + 4));
}

function regionDigest(image, logicalRect) {
  const first = logicalLocalPoint(image.box, logicalRect.x, logicalRect.y);
  const last = logicalLocalPoint(image.box,
    logicalRect.x + logicalRect.w, logicalRect.y + logicalRect.h);
  const x0 = Math.max(0, Math.floor(first.x * image.png.width / image.box.width));
  const y0 = Math.max(0, Math.floor(first.y * image.png.height / image.box.height));
  const x1 = Math.min(image.png.width, Math.ceil(last.x * image.png.width / image.box.width));
  const y1 = Math.min(image.png.height, Math.ceil(last.y * image.png.height / image.box.height));
  const hash = crypto.createHash('sha256');
  for (let y = y0; y < y1; y += 1) {
    const start = (y * image.png.width + x0) * 4;
    const end = (y * image.png.width + x1) * 4;
    hash.update(image.png.data.subarray(start, end));
  }
  return hash.digest('hex');
}

function requireActiveTab(image, tabIndex, label) {
  // The shared Android/Web surface reserves a 48 px safe area on each side.
  // Header layout: safe 48 + margin 27, five 263 px tabs and 7 px gaps.
  const tabWidth = 263;
  const tabStart = 75 + tabIndex * (tabWidth + 7);
  // Sample the clear upper-left interior, away from centred text and borders.
  const [red, green, blue] = pixelAtLogical(image, tabStart + 13, 78);
  const looksActive = red > 85 && blue > 115 && red > green + 20;
  if (!looksActive) {
    throw new Error(`${label}: active tab pixel was rgb(${red},${green},${blue})`);
  }
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

  // Centres and rendered active-state samples in the fixed 1496x672 logical
  // viewport catch the exact Retina/letterbox bug: a click must activate the
  // tab drawn at that coordinate, not merely cause an animated canvas update.
  const tabs = [
    ['actor', 1, 477, 87],
    ['fx', 2, 747, 87],
    ['master', 3, 1017, 87],
    ['memory', 4, 1287, 87],
    ['place', 0, 207, 87],
  ];
  for (const [label, index, x, y] of tabs) {
    await clickLogical(page, x, y, testCase.touch);
    await page.waitForTimeout(120);
    requireActiveTab(await canvasImage(page), index, `${testCase.name}/${label}`);
  }

  if (!testCase.touch) {
    // The first Place macro row is isolated from animated telemetry, so a
    // regional digest verifies that mouse drag changed the actual control.
    const region = { x: 210, y: 225, w: 650, h: 80 };
    const before = regionDigest(await canvasImage(page), region);
    await dragLogical(page, 360, 265, 790, 265);
    await page.waitForTimeout(180);
    const after = regionDigest(await canvasImage(page), region);
    if (after === before) throw new Error(`${testCase.name}/drag: macro row did not change`);
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
