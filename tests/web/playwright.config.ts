// SPDX-License-Identifier: MIT
// Serves the Emscripten build (build/web by default, override with CADV_WEB_DIR)
// and runs the browser tests against it.
import { defineConfig, devices } from '@playwright/test';
import * as path from 'path';

const webDir = process.env.CADV_WEB_DIR ?? path.resolve(__dirname, '../../build/web');
const port = 8771;

export default defineConfig({
  testDir: '.',
  timeout: 30_000,
  reporter: [['list']],
  use: {
    baseURL: `http://127.0.0.1:${port}`,
    trace: 'off',
    launchOptions: {
      // Software WebGL 2 for headless CI machines without a GPU.
      args: ['--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader'],
    },
  },
  projects: [{ name: 'desktop-chromium', use: { ...devices['Desktop Chrome'] } }],
  webServer: {
    command: `python3 -m http.server ${port} --bind 127.0.0.1 --directory "${webDir}"`,
    url: `http://127.0.0.1:${port}/cadvent.html`,
    reuseExistingServer: !process.env.CI,
  },
});
