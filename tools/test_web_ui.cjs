// Copyright (C) 2026 Steve Madden
// SPDX-License-Identifier: GPL-3.0-or-later
'use strict';
// Exercises the actual embedded-page script with a small DOM and HTTP harness.
// Run: node tools/test_web_ui.cjs -- no packages or firmware build required.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const root = path.resolve(__dirname, '..');
const html = fs.readFileSync(path.join(root, 'arduino/CH899_Clock/web/index.html'), 'utf8');
const script = html.match(/<script>([\s\S]*?)<\/script>/)[1];
const baseConfig = {ssid: 'Home', ntpHost: 'pool.ntp.org', timezone: 'GMT0BST,M3.5.0/1,M10.5.0/2', syncHour: 10, syncMinute: 0, hasPassword: true, requestToken: 'a'.repeat(32)};
const response = (data, status = 200) => ({ok: status >= 200 && status < 300, status, json: async () => data});
async function settle() { for (let n = 0; n < 40; n++) await Promise.resolve(); }

function harness(overrides = {}) {
  let nextTimer = 1, now = 0;
  const intervals = new Map(), timeouts = new Map(), events = {}, calls = [], beacons = [];
  const elements = {};
  for (const [, id] of html.matchAll(/id="([^"]+)"/g)) {
    let value = '';
    elements[id] = {
      get value() { return value; }, set value(v) { value = String(v); },
      options: [], disabled: false, textContent: '', className: '', placeholder: '',
      get length() { return this.options.length; },
      set length(v) { this.options.length = v; },
      add(option) { this.options.push(option); },
      focus() { this.focused = true; }
    };
  }
  const routes = overrides.routes || {};
  const fetch = async (url, options = {}) => {
    calls.push({url, method: options.method || 'GET', body: options.body?.toString(), options});
    if (routes[url]) return routes[url](options);
    if (url === '/api/v1/config' && options.method !== 'POST') return response({...baseConfig, ...overrides.config});
    if (url === '/api/v1/config') return response({saved: true, reboot: 'scheduled'});
    if (url === '/api/v1/status') return response({mode: 'station', ip: '192.0.2.1', deviceName: 'WiFi-Clock Setup-321a86', mdnsHost: 'wifi-clock-321a86.local', firmwareVersion: 'v1.0.0'});
    if (url.startsWith('/api/v1/scan')) return response({networks: overrides.networks || []});
    if (url.startsWith('/api/v1/keepalive')) return response({}, 204);
    if (url === '/api/v1/session/close') return response({}, 204);
    if (url === '/api/v1/factory-reset') return response({reset: 'scheduled'}, 202);
    throw Error('Unexpected route: ' + url);
  };
  const context = vm.createContext({
    document: {getElementById: id => elements[id]},
    Option: function (text, value) { this.text = text; this.value = value; },
    AbortController, URLSearchParams, Blob, fetch,
    Date: {now: () => now},
    navigator: {sendBeacon: (...args) => { beacons.push(args); return overrides.beaconResult !== false; }},
    confirm: () => overrides.confirm !== false,
    location: {reload() { context.reloaded = true; }},
    addEventListener: (name, handler) => { events[name] = handler; },
    setTimeout: (fn, ms) => { const id = nextTimer++; timeouts.set(id, {fn, ms}); return id; },
    clearTimeout: id => timeouts.delete(id),
    setInterval: (fn, ms) => { const id = nextTimer++; intervals.set(id, {fn, ms}); return id; },
    clearInterval: id => intervals.delete(id)
  });
  vm.runInContext(script, context, {filename: 'web/index.html'});
  return {context, elements, calls, events, intervals, timeouts, beacons,
    async poll() { for (const {fn} of [...intervals.values()]) fn(); await settle(); },
    async tickTimeout(ms) {
      now += ms;
      for (const [id, timer] of [...timeouts]) {
        if (timer.ms === ms) { timeouts.delete(id); timer.fn(); }
      }
      await settle();
    }
  };
}
const tests = [];
const test = (name, run) => tests.push({name, run});

test('page and generated header match exactly', async () => {
  const header = fs.readFileSync(path.join(root, 'arduino/CH899_Clock/web_ui.h'), 'utf8');
  assert.equal(header.split('R"CH899_WEB(\n')[1].split('\n)CH899_WEB";')[0], html);
});
test('page uses the fixed WiFi-Clock Setup title', async () => {
  const h = harness(); await settle();
  assert.match(html, /<title>WiFi-Clock Setup<\/title>/);
  assert.match(html, /<h1 id="mainTitle">WiFi-Clock Setup<\/h1>/);
  assert.doesNotMatch(script, /document\.title\s*=|mainTitle'\)\.textContent\s*=/);
  assert.match(html, /rel="icon"[^>]+%F0%9F%95%92/);
});
test('status shows the unique local address only on the connected LAN', async () => {
  const station = harness(); await settle();
  assert.equal(station.elements.localAddress.textContent, 'Local address: http://wifi-clock-321a86.local/');
  assert.equal(station.elements.localAddress.className, 'small');
  assert.equal(station.elements.firmwareVersion.textContent, 'Firmware v1.0.0');
  const portal = harness({routes: {'/api/v1/status': () => response({mode: 'portal', ip: '192.168.4.1', mdnsHost: 'wifi-clock-321a86.local'})}});
  await settle();
  assert.equal(portal.elements.localAddress.textContent, '');
  assert.equal(portal.elements.localAddress.className, 'small hidden');
});
test('NTP server and daily update time share a responsive row', async () => {
  assert.match(html, /<div class="row">\s*<label for="ntpHost">NTP server[\s\S]*?<label for="syncTime">Daily update time[\s\S]*?<\/div>/);
  assert.match(html, /<option value="10:00" selected>10:00<\/option>/);
  assert.match(html, /Network access to the selected time server is needed during each update/);
  assert.match(html, /internet access is required when that server is online rather than local/);
  assert.match(html, /cannot automatically follow future changes to local time laws/);
});
test('default fields use placeholders and the London automatic rule', async () => {
  const h = harness(); await settle();
  assert.equal(h.elements.ntpHost.value, '');
  assert.equal(h.elements.timezonePreset.value, 'london');
  assert.equal(h.elements.dstMode.value, 'automatic');
  assert.equal(h.elements.customTimezone.value, '');
  assert.equal(h.elements.customTimezone.disabled, true);
  assert.equal(h.elements.syncTime.value, '10:00');
  assert.match(h.elements.password.placeholder, /keep this network/);
});
test('known and custom timezone rules survive load', async () => {
  const known = harness({config: {ntpHost: 'time.example.test', timezone: 'UTC0'}});
  await settle();
  assert.equal(known.elements.ntpHost.value, 'time.example.test');
  assert.equal(known.elements.timezonePreset.value, 'utc_reykjavik');
  assert.equal(known.elements.dstMode.value, 'automatic');
  const custom = harness({config: {timezone: 'ABC-4'}});
  await settle();
  assert.equal(custom.elements.timezonePreset.value, 'london');
  assert.equal(custom.elements.dstMode.value, 'custom');
  assert.equal(custom.elements.customTimezone.value, 'ABC-4');
  assert.equal(custom.elements.customTimezone.disabled, false);
});
test('all 40 verified timezone presets populate and resolve both modes', async () => {
  const h = harness(); await settle();
  const presets = vm.runInContext('TIMEZONE_PRESETS', h.context);
  assert.equal(presets.length, 40);
  assert.equal(h.elements.timezonePreset.options.length, 40);
  assert.equal(new Set(presets.map(preset => preset[0])).size, 40);
  for (const preset of presets) {
    assert(preset[2].length > 0 && preset[2].length <= 96);
    assert(preset[3].length > 0 && preset[3].length <= 96);
    h.elements.timezonePreset.value = preset[0];
    h.elements.dstMode.value = 'disabled';
    assert.equal(vm.runInContext('selectedTimezoneRule()', h.context), preset[2]);
    h.elements.dstMode.value = 'automatic';
    assert.equal(vm.runInContext('selectedTimezoneRule()', h.context), preset[3]);
  }
});
test('only MM32-confirmed daily update times are offered', async () => {
  const select = html.match(/<select id="syncTime">([\s\S]*?)<\/select>/);
  assert.ok(select);
  assert.deepEqual([...select[1].matchAll(/<option value="([^"]+)"[^>]*>/g)].map(match => match[1]),
    ['09:00', '10:00', '21:00', '22:00']);
});
test('settings stay disabled until configuration arrives', async () => {
  let finish;
  const h = harness({routes: {'/api/v1/config': () => new Promise(resolve => { finish = resolve; })}});
  assert.equal(h.elements.sendTime.disabled, true);
  assert.equal(h.elements.ssid.disabled, true);
  finish(response(baseConfig)); await settle();
  assert.equal(h.elements.sendTime.disabled, false);
});
test('scan polls asynchronously and merges strongest SSID; manual stays last', async () => {
  const h = harness({routes: {
    '/api/v1/scan?start=1': () => response({scanning: true}, 202),
    '/api/v1/scan': () => response({networks: [
      {ssid: 'Home', rssi: -80}, {ssid: 'Home', rssi: -30},
      {ssid: '', rssi: -20}, {ssid: 'manual', rssi: -40},
      {ssid: '__manual__', rssi: -50}
    ]})
  }});
  await settle();
  assert.equal(h.elements.scan.disabled, true);
  await h.tickTimeout(750);
  const options = h.elements.networks.options;
  assert.deepEqual(options.map(o => o.value), ['', 'ssid:Home', 'ssid:manual', 'ssid:__manual__', 'manual']);
  assert.match(options[1].text, /-30 dBm/);
  assert.equal(h.elements.scan.disabled, false);
});
test('status refresh never overwrites edited form fields', async () => {
  const h = harness(); await settle();
  h.elements.ssid.value = 'Edited network';
  h.elements.dstMode.value = 'custom';
  h.elements.customTimezone.value = 'ABC-4';
  await h.poll();
  assert.equal(h.elements.ssid.value, 'Edited network');
  assert.equal(h.elements.customTimezone.value, 'ABC-4');
});
test('changing SSID updates password instructions', async () => {
  const h = harness(); await settle();
  h.elements.ssid.value = 'Other'; h.elements.ssid.oninput();
  assert.match(h.elements.password.placeholder, /open Wi-Fi/);
  h.elements.ssid.value = 'Home'; h.elements.ssid.oninput();
  assert.match(h.elements.password.placeholder, /saved password/);
});
test('successful save uses one POST and stops polling without separate reboot', async () => {
  const h = harness(); await settle();
  h.elements.password.value = 'test-password';
  await h.elements.sendTime.onclick(); await settle();
  const posts = h.calls.filter(c => c.method === 'POST' && c.url !== '/api/v1/keepalive');
  assert.deepEqual(posts.map(c => c.url), ['/api/v1/config']);
  assert.equal(new URLSearchParams(posts[0].body).get('syncHour'), '10');
  assert.equal(new URLSearchParams(posts[0].body).get('syncMinute'), '00');
  assert.equal(new URLSearchParams(posts[0].body).get('timezone'), baseConfig.timezone);
  assert.equal(h.intervals.size, 0);
  assert.equal(h.elements.password.value, '');
  assert.equal(h.elements.sendTime.disabled, true);
  assert.match(h.elements.notice.textContent, /Settings saved/);
  assert.match(h.elements.state.textContent, /restarting/);
});
test('failed save displays server error and permits retry', async () => {
  const h = harness({routes: {'/api/v1/config': options => options.method === 'POST'
    ? response({error: 'Could not save configuration'}, 500) : response(baseConfig)}});
  await settle();
  await h.elements.sendTime.onclick(); await settle();
  assert.match(h.elements.notice.textContent, /Could not save configuration/);
  assert.equal(h.elements.sendTime.disabled, false);
  assert.equal(h.intervals.size, 2);
});
test('empty SSID makes no save request', async () => {
  const h = harness(); await settle();
  h.elements.ssid.value = '';
  await h.elements.sendTime.onclick();
  assert.equal(h.calls.filter(c => c.url === '/api/v1/config' && c.method === 'POST').length, 0);
  assert.equal(h.elements.ssid.focused, true);
});
test('blank custom timezone makes no save request', async () => {
  const h = harness(); await settle();
  h.elements.dstMode.value = 'custom';
  h.elements.customTimezone.value = '';
  await h.elements.sendTime.onclick();
  assert.equal(h.calls.filter(c => c.url === '/api/v1/config' && c.method === 'POST').length, 0);
  assert.equal(h.elements.customTimezone.focused, true);
});
test('disabled and custom modes submit their resolved POSIX rules', async () => {
  const disabled = harness(); await settle();
  disabled.elements.timezonePreset.value = 'denver';
  disabled.elements.dstMode.value = 'disabled';
  await disabled.elements.sendTime.onclick();
  let post = disabled.calls.find(c => c.url === '/api/v1/config' && c.method === 'POST');
  assert.equal(new URLSearchParams(post.body).get('timezone'), 'DEN7');
  const custom = harness(); await settle();
  custom.elements.dstMode.value = 'custom';
  custom.elements.customTimezone.value = 'ABC-4';
  await custom.elements.sendTime.onclick();
  post = custom.calls.find(c => c.url === '/api/v1/config' && c.method === 'POST');
  assert.equal(new URLSearchParams(post.body).get('timezone'), 'ABC-4');
});
test('repeated ticks do not overlap stalled status or heartbeat requests', async () => {
  const never = () => new Promise(() => {});
  const h = harness({routes: {
    '/api/v1/status': never, '/api/v1/keepalive': never
  }});
  await settle(); await h.poll(); await h.poll();
  assert.equal(h.calls.filter(c => c.url === '/api/v1/status').length, 1);
  assert.equal(h.calls.filter(c => c.url.startsWith('/api/v1/keepalive')).length, 1);
});
test('status timeout reports lost connection and allows another request', async () => {
  const stalled = options => new Promise((resolve, reject) => {
    options.signal.addEventListener('abort', () => reject(Error('Timed out')));
  });
  const h = harness({routes: {'/api/v1/status': stalled}});
  await settle(); await h.tickTimeout(6000);
  assert.equal(h.elements.state.textContent, 'Clock connection lost');
  await h.poll();
  assert.equal(h.calls.filter(c => c.url === '/api/v1/status').length, 2);
});
test('page close stops timers and uses fetch when beacon cannot be queued', async () => {
  const h = harness({beaconResult: false}); await settle();
  h.events.pagehide(); await settle();
  assert.equal(h.intervals.size, 0);
  assert.equal(h.beacons.length, 1);
  assert(h.calls.some(c => c.url === '/api/v1/session/close' && c.options.keepalive));
  const count = h.calls.length; await h.poll();
  assert.equal(h.calls.length, count);
});
test('factory reset requires confirmation then stops polling', async () => {
  const cancelled = harness({confirm: false}); await settle();
  await cancelled.elements.factoryReset.onclick();
  assert.equal(cancelled.calls.filter(c => c.url === '/api/v1/factory-reset').length, 0);
  const accepted = harness(); await settle();
  await accepted.elements.factoryReset.onclick();
  assert(accepted.calls.some(c => c.url === '/api/v1/factory-reset' && c.method === 'POST'));
  assert.equal(accepted.intervals.size, 0);
  assert.match(accepted.elements.notice.textContent, /settings erased/);
});
test('back-forward restored page reloads to establish a new session', async () => {
  const h = harness(); await settle();
  h.events.pageshow({persisted: true});
  assert.equal(h.context.reloaded, true);
});

test('clock token protects save and scan requests without appearing in URLs', async () => {
  const h = harness(); await settle();
  await h.elements.sendTime.onclick();
  for (const call of h.calls.filter(c => c.url.startsWith('/api/v1/scan') || (c.url === '/api/v1/config' && c.method === 'POST'))) {
    assert.equal(call.options.headers['X-Clock-Token'], baseConfig.requestToken);
    assert(!call.url.includes(baseConfig.requestToken));
  }
});
test('heartbeat retries retain page identity and send start only until acknowledged', async () => {
  let attempts = 0;
  const h = harness({routes: {'/api/v1/keepalive': () => {
    if (++attempts === 1) throw Error('Reply lost');
    return response({}, 204);
  }}});
  await settle(); await h.poll(); await h.poll();
  const heartbeats = h.calls.filter(c => c.url === '/api/v1/keepalive');
  const forms = heartbeats.map(c => new URLSearchParams(c.body));
  assert.deepEqual(forms.map(f => f.get('start')), ['1', '1', '0']);
  assert.match(forms[0].get('page'), /^[0-9a-f]{32}$/);
  assert.equal(new Set(forms.map(f => f.get('page'))).size, 1);
  assert(heartbeats.every(c => c.options.headers['X-Clock-Token'] === baseConfig.requestToken));
});
test('close beacon carries the page identity and per-boot token', async () => {
  const h = harness(); await settle(); h.events.pagehide();
  const heartbeat = h.calls.find(c => c.url === '/api/v1/keepalive');
  const form = new URLSearchParams(h.beacons[0][1]);
  assert.equal(form.get('page'), new URLSearchParams(heartbeat.body).get('page'));
  assert.equal(form.get('token'), baseConfig.requestToken);
});
test('ambiguous save stops polling and asks the owner to reconnect', async () => {
  const h = harness({routes: {'/api/v1/config': options => {
    if (options.method === 'POST') throw Error('Network disappeared');
    return response(baseConfig);
  }}});
  await settle(); await h.elements.sendTime.onclick();
  assert.equal(h.intervals.size, 0);
  assert.equal(h.elements.sendTime.disabled, true);
  assert.match(h.elements.notice.textContent, /Save was not confirmed/);
});
test('expired boot token stops stale page activity', async () => {
  const h = harness({routes: {'/api/v1/keepalive': () => response({error: 'Page expired'}, 403)}});
  await settle();
  assert.equal(h.intervals.size, 0);
  assert.match(h.elements.notice.textContent, /page has expired/);
  assert.equal(h.calls.filter(c => c.url.startsWith('/api/v1/scan')).length, 0);
});
test('closing before bootstrap completes never starts a heartbeat', async () => {
  let finish;
  const h = harness({routes: {'/api/v1/config': () => new Promise(resolve => { finish = resolve; })}});
  h.events.pagehide(); finish(response(baseConfig)); await settle();
  assert.equal(h.calls.filter(c => c.url === '/api/v1/keepalive').length, 0);
  assert.equal(h.beacons.length, 0);
});

(async () => {
  let passed = 0;
  for (const {name, run} of tests) {
    try { await run(); passed++; console.log('PASS ' + name); }
    catch (error) { console.error('FAIL ' + name + '\n' + error.stack); process.exitCode = 1; }
  }
  console.log(passed + '/' + tests.length + ' page checks passed.');
})();
