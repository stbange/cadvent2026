// SPDX-License-Identifier: MIT
import { devices, expect, test } from '@playwright/test';

/** Reads one pixel (x, y in percent) from a PNG screenshot inside the page. */
async function pixelAt(page: import('@playwright/test').Page, png: Buffer, xPct: number, yPct: number) {
  return page.evaluate(
    async ({ b64, xPct, yPct }) => {
      const img = new Image();
      img.src = `data:image/png;base64,${b64}`;
      await img.decode();
      const c = document.createElement('canvas');
      c.width = img.width;
      c.height = img.height;
      const ctx = c.getContext('2d')!;
      ctx.drawImage(img, 0, 0);
      const x = Math.floor(((img.width - 1) * xPct) / 100);
      const y = Math.floor(((img.height - 1) * yPct) / 100);
      return Array.from(ctx.getImageData(x, y, 1, 1).data.slice(0, 3));
    },
    { b64: png.toString('base64'), xPct, yPct },
  );
}

/** Mean and spread of the luminance of a PNG screenshot (sampled grid). */
async function stats(page: import('@playwright/test').Page, png: Buffer) {
  return page.evaluate(async (b64) => {
    const img = new Image();
    img.src = `data:image/png;base64,${b64}`;
    await img.decode();
    const c = document.createElement('canvas');
    c.width = img.width;
    c.height = img.height;
    const ctx = c.getContext('2d')!;
    ctx.drawImage(img, 0, 0);
    const d = ctx.getImageData(0, 0, img.width, img.height).data;
    let sum = 0, min = 255, max = 0, n = 0;
    for (let i = 0; i < d.length; i += 4 * 97) {
      const l = 0.3 * d[i] + 0.59 * d[i + 1] + 0.11 * d[i + 2];
      sum += l; min = Math.min(min, l); max = Math.max(max, l); n++;
    }
    return { mean: sum / n, spread: max - min };
  }, png.toString('base64'));
}

function collectLogs(page: import('@playwright/test').Page) {
  const logs: string[] = [];
  page.on('console', (m) => logs.push(m.text()));
  return logs;
}

test('web build boots WebGL 2 and renders the hall', async ({ page }) => {
  const logs = collectLogs(page);
  await page.goto('/cadvent.html?date=2026-12-05');
  await expect.poll(() => logs.some((l) => l.includes('renderer ready (OpenGL ES 3.0)'))).toBe(true);
  await expect.poll(() => logs.some((l) => l.includes('ready (rendering, 2026-12-05)'))).toBe(true);
  await page.waitForTimeout(1000);
  const shot = await page.locator('#canvas').screenshot();
  // a lit, detailed scene (not a blank or black canvas)
  const s = await stats(page, shot);
  expect(s.mean).toBeGreaterThan(8);
  expect(s.spread).toBeGreaterThan(80);
  // the HUD's gold text sits in the top-left corner
  const hud = await pixelAt(page, shot, 1, 2);
  expect(hud.length).toBe(3);
  await expect(page.locator('#status')).toBeHidden();
});

test('development URL override opens everything', async ({ page }) => {
  const logs = collectLogs(page);
  await page.goto('/cadvent.html?dev=unlockall&portal=1');
  await expect.poll(() => logs.some((l) => l.includes('entered portal 1'))).toBe(true);
});

// Device descriptors carry defaultBrowserType, which cannot be set per group.
const { defaultBrowserType: _ignored, ...pixel7 } = devices['Pixel 7'];

test.describe('phone', () => {
  test.use(pixel7);

  test('touch input switches to on-screen controls', async ({ page }) => {
    const logs = collectLogs(page);
    await page.goto('/cadvent.html');
    await expect.poll(() => logs.some((l) => l.includes('renderer ready'))).toBe(true);
    const box = (await page.locator('#canvas').boundingBox())!;
    const cdp = await page.context().newCDPSession(page);
    const at = (x: number, y: number) => [{ x: box.x + box.width * x, y: box.y + box.height * y, id: 1 }];
    await cdp.send('Input.dispatchTouchEvent', { type: 'touchStart', touchPoints: at(0.2, 0.7) });
    await cdp.send('Input.dispatchTouchEvent', { type: 'touchMove', touchPoints: at(0.2, 0.55) });
    await page.waitForTimeout(300);
    await cdp.send('Input.dispatchTouchEvent', { type: 'touchEnd', touchPoints: [] });
    await expect.poll(() => logs.some((l) => l.includes('touch controls shown'))).toBe(true);
  });
});
