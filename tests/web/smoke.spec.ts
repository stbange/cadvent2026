// SPDX-License-Identifier: MIT
import { expect, test } from '@playwright/test';

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

function near(actual: number[], expected: number[], tol = 3) {
  return actual.every((v, i) => Math.abs(v - expected[i]) <= tol);
}

test('web build boots WebGL 2 and draws the M0 scene', async ({ page }) => {
  const logs: string[] = [];
  page.on('console', (m) => logs.push(m.text()));
  await page.goto('/cadvent.html');
  await expect.poll(() => logs.some((l) => l.includes('renderer ready (OpenGL ES 3.0)'))).toBe(true);
  await page.waitForTimeout(500);

  const shot = await page.locator('#canvas').screenshot();
  expect(near(await pixelAt(page, shot, 50, 50), [26, 140, 60])).toBe(true); // fir-green triangle
  expect(near(await pixelAt(page, shot, 2, 2), [16, 20, 48])).toBe(true); // night-blue clear colour
  await expect(page.locator('#status')).toBeHidden();
});
