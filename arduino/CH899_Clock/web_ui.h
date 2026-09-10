// Copyright (C) 2026 Steve Madden
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Generated from web/index.html by tools/embed_web_ui.ps1. Do not edit here.
// Edit the HTML source, run this script, then compile the Arduino sketch.
static const char CLOCK_WEB_UI[] PROGMEM = R"CH899_WEB(
<!-- Copyright (C) 2026 Steve Madden | SPDX-License-Identifier: GPL-3.0-or-later -->
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>WiFi-Clock Setup</title>
  <link rel="icon" href="data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 100 100'%3E%3Ctext y='.9em' font-size='90'%3E%F0%9F%95%92%3C/text%3E%3C/svg%3E">
  <style>
    :root{--ink:#142238;--muted:#68758a;--line:#dbe2ec;--blue:#2167d8;--blue-dark:#174da6;--red:#b42318;--red-dark:#8e1b13;--pale:#f3f7fc;--card:#fff;--ok:#16764a;--warn:#a45b00}
    *{box-sizing:border-box}body{max-width:680px;margin:auto;padding:20px;background:var(--pale);color:var(--ink);font:16px/1.45 system-ui,-apple-system,"Segoe UI",sans-serif}
    header{padding:8px 2px 18px}h1{margin:0;font-size:1.9rem;letter-spacing:-.03em}h2{margin:0;font-size:1.08rem}p{margin:8px 0;color:var(--muted)}
    .card{margin:14px 0;padding:20px;border:1px solid var(--line);border-radius:16px;background:var(--card);box-shadow:0 8px 22px #152d4c0a}
    .status{display:flex;align-items:center;gap:9px;margin-top:10px;font-weight:650}.dot{width:10px;height:10px;border-radius:50%;background:#9da9ba}.dot.online{background:#19a563;box-shadow:0 0 0 4px #19a5631d}.dot.portal{background:#e7a126;box-shadow:0 0 0 4px #e7a1261d}.hidden{display:none}.small{font-size:.88rem}.firmwareVersion{margin-top:16px;text-align:center;color:#8792a4}a{color:var(--blue);font-weight:650}
    label{display:block;margin-top:15px;font-size:.9rem;font-weight:700}input,select,button{width:100%;min-height:44px;margin-top:6px;border:1px solid #bfcada;border-radius:10px;padding:9px 11px;background:#fff;color:inherit;font:inherit}input:focus,select:focus{outline:3px solid #2167d82e;border-color:var(--blue)}
    .row{display:grid;grid-template-columns:1fr 1fr;gap:12px}.actions{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-top:20px}button{cursor:pointer;border-color:var(--blue);background:var(--blue);color:#fff;font-weight:750}button:hover{background:var(--blue-dark)}button:disabled{opacity:.6;cursor:wait}button.secondary{border-color:#bfcada;background:#fff;color:var(--blue)}button.secondary:hover{background:#f6f9fe}button.danger{margin-top:10px;border-color:var(--red);background:var(--red)}button.danger:hover{background:var(--red-dark)}.notice{min-height:1.4em;margin-top:14px;font-size:.92rem}.notice.ok{color:var(--ok)}.notice.bad{color:#b12424}
    @media(max-width:480px){body{padding:14px}.row,.actions{grid-template-columns:1fr}.card{padding:17px}}
  </style>
</head>
<body>
  <header>
    <h1 id="mainTitle">WiFi-Clock Setup</h1>
    <div class="status"><span id="dot" class="dot"></span><span id="state">Checking clock status…</span></div>
    <p id="localAddress" class="small hidden"></p>
  </header>

  <section class="card">
    <h2>Set up the clock</h2>
    <p>Set your 2.4 GHz Wi-Fi, time server and time zone.</p>
    <label for="networks">Available Wi-Fi networks</label>
    <select id="networks"><option value="">Scanning networks…</option><option value="manual">Enter manually / hidden SSID</option></select>
    <div id="ssidRow"><label for="ssid">Network name (SSID)</label><input id="ssid" maxlength="32" autocomplete="off" placeholder="Enter or choose a Wi-Fi network"></div>
    <label for="password">Wi-Fi password</label>
    <input id="password" type="password" maxlength="64" autocomplete="current-password" placeholder="Enter password; leave blank for open Wi-Fi">
    <div class="row">
      <label for="ntpHost">NTP server<input id="ntpHost" maxlength="253" autocomplete="off" placeholder="pool.ntp.org"></label>
      <label for="syncTime">Daily update time<select id="syncTime"><option value="09:00">09:00</option><option value="10:00" selected>10:00</option><option value="21:00">21:00</option><option value="22:00">22:00</option></select></label>
    </div>
    <div class="row">
      <label for="timezonePreset">Timezone<select id="timezonePreset"></select></label>
      <label for="dstMode">Daylight saving<select id="dstMode"><option value="automatic">Automatic</option><option value="disabled">Disabled</option><option value="custom">Custom rule</option></select></label>
    </div>
    <label for="customTimezone">Custom POSIX rule (used in Custom rule mode)</label>
    <input id="customTimezone" maxlength="96" autocomplete="off" placeholder="GMT0BST,M3.5.0/1,M10.5.0/2">
    <p class="small">Automatic applies the verified daylight-saving rule for the selected location. Disabled keeps standard time all year. Custom uses the POSIX rule entered above. Time-zone and daylight-saving changes are calculated locally after the clock obtains UTC time. Network access to the selected time server is needed during each update; internet access is required when that server is online rather than local. The built-in rules cannot automatically follow future changes to local time laws.</p>
    <div class="actions">
      <button class="secondary" id="scan">Re-scan networks</button>
      <button id="sendTime">Save &amp; update clock</button>
    </div>
    <button class="danger" id="factoryReset">Factory reset</button>
    <p id="notice" class="notice" role="status" aria-live="polite"></p>
    <p id="firmwareVersion" class="small firmwareVersion">Firmware</p>
  </section>

  <script>
    const $ = id => document.getElementById(id);
    const DEFAULT_NTP = 'pool.ntp.org';
    const DEFAULT_TZ = 'GMT0BST,M3.5.0/1,M10.5.0/2';
    // [id, label, fixed-standard-time rule, automatic rule]. Automatic rules
    // were checked against current IANA data from 2026-09-10 through 2035.
    const TIMEZONE_PRESETS = [
      ['baker','(UTC-12:00) Baker Island','<-12>12','<-12>12'],
      ['american_samoa','(UTC-11:00) American Samoa','SST11','SST11'],
      ['honolulu','(UTC-10:00) Honolulu','HST10','HST10'],
      ['anchorage','(UTC-09:00) Anchorage','AKST9','AKST9AKDT,M3.2.0,M11.1.0'],
      ['los_angeles','(UTC-08:00) Los Angeles','PST8','PST8PDT,M3.2.0,M11.1.0'],
      ['phoenix_vancouver','(UTC-07:00) Phoenix, Vancouver','MST7','MST7'],
      ['denver','(UTC-07:00) Denver','DEN7','MST7MDT,M3.2.0,M11.1.0'],
      ['chicago','(UTC-06:00) Chicago','CHI6','CST6CDT,M3.2.0,M11.1.0'],
      ['mexico_city','(UTC-06:00) Mexico City','CST6','CST6'],
      ['new_york','(UTC-05:00) New York','EST5','EST5EDT,M3.2.0,M11.1.0'],
      ['lima','(UTC-05:00) Lima','<-05>5','<-05>5'],
      ['halifax','(UTC-04:00) Halifax','AST4','AST4ADT,M3.2.0,M11.1.0'],
      ['santiago','(UTC-04:00) Santiago','<-04>4','<-04>4<-03>,M9.1.6/24,M4.1.6/24'],
      ['newfoundland','(UTC-03:30) Newfoundland','NST3:30','NST3:30NDT,M3.2.0,M11.1.0'],
      ['buenos_aires','(UTC-03:00) Buenos Aires','<-03>3','<-03>3'],
      ['south_georgia','(UTC-02:00) South Georgia','<-02>2','<-02>2'],
      ['azores','(UTC-01:00) Azores','<-01>1','<-01>1<+00>,M3.5.0/0,M10.5.0/1'],
      ['utc_reykjavik','(UTC+00:00) UTC / Reykjavik','UTC0','UTC0'],
      ['london','(UTC+00:00) London (GMT/BST)','GMT0','GMT0BST,M3.5.0/1,M10.5.0/2'],
      ['paris_berlin','(UTC+01:00) Paris, Berlin','CET-1','CET-1CEST,M3.5.0,M10.5.0/3'],
      ['athens','(UTC+02:00) Athens','EET-2','EET-2EEST,M3.5.0/3,M10.5.0/4'],
      ['johannesburg','(UTC+02:00) Johannesburg','SAST-2','SAST-2'],
      ['moscow_nairobi','(UTC+03:00) Moscow, Nairobi','MSK-3','MSK-3'],
      ['tehran','(UTC+03:30) Tehran','<+0330>-3:30','<+0330>-3:30'],
      ['dubai','(UTC+04:00) Dubai, Abu Dhabi','<+04>-4','<+04>-4'],
      ['karachi','(UTC+05:00) Karachi, Islamabad','PKT-5','PKT-5'],
      ['almaty','(UTC+05:00) Almaty','<+05>-5','<+05>-5'],
      ['india_sri_lanka','(UTC+05:30) India, Sri Lanka','IST-5:30','IST-5:30'],
      ['dhaka','(UTC+06:00) Dhaka','<+06>-6','<+06>-6'],
      ['yangon','(UTC+06:30) Yangon','<+0630>-6:30','<+0630>-6:30'],
      ['bangkok_jakarta','(UTC+07:00) Bangkok, Jakarta','<+07>-7','<+07>-7'],
      ['singapore_beijing','(UTC+08:00) Singapore, Beijing','<+08>-8','<+08>-8'],
      ['tokyo_seoul','(UTC+09:00) Tokyo, Seoul','JST-9','JST-9'],
      ['darwin','(UTC+09:30) Darwin','ACST-9:30','ACST-9:30'],
      ['sydney_melbourne','(UTC+10:00) Sydney, Melbourne','AEST-10','AEST-10AEDT,M10.1.0,M4.1.0/3'],
      ['solomon','(UTC+11:00) Solomon Islands','<+11>-11','<+11>-11'],
      ['auckland','(UTC+12:00) Auckland','NZST-12','NZST-12NZDT,M9.5.0,M4.1.0/3'],
      ['fiji','(UTC+12:00) Fiji','<+12>-12','<+12>-12'],
      ['samoa_tonga','(UTC+13:00) Samoa, Tonga','<+13>-13','<+13>-13'],
      ['kiritimati','(UTC+14:00) Kiritimati','<+14>-14','<+14>-14']
    ];
    let savedSsid = '', hasSavedPassword = false;
    let requestToken = '';
    // A page identity, not an authentication secret. The ESP supplies the
    // independent per-boot anti-forgery token when configuration is loaded.
    const pageId = Array.from({length: 4}, () => Math.floor(Math.random() * 0x100000000).toString(16).padStart(8, '0')).join('');
    let leaving = false, scanBusy = false, actionBusy = false, configLoaded = false;
    let webSessionStarted = false, heartbeatBusy = false, statusBusy = false;
    let heartbeatTimer, statusTimer;
    const notice = (text, kind = '') => {
      $('notice').textContent = text;
      $('notice').className = 'notice ' + kind;
    };
    // Bound every request so a sleeping clock cannot accumulate pending polls.
    async function request(url, options = {}) {
      const controller = new AbortController();
      const timer = setTimeout(() => controller.abort(), 6000);
      try {
        const headers = {...options.headers};
        if (requestToken) headers['X-Clock-Token'] = requestToken;
        const response = await fetch(url, {...options, headers, cache: 'no-store', signal: controller.signal});
        const data = response.status === 204 ? {} : await response.json();
        if (!response.ok) {
          const error = Error(data.error || 'The clock could not complete the request.');
          error.status = response.status;
          throw error;
        }
        return data;
      } finally {
        clearTimeout(timer);
      }
    }
    function setStatus(status) {
      if (typeof status.firmwareVersion === 'string' && status.firmwareVersion) {
        $('firmwareVersion').textContent = 'Firmware ' + status.firmwareVersion;
      }
      const localAddress = $('localAddress');
      if (status.mode === 'station' && typeof status.mdnsHost === 'string' && status.mdnsHost) {
        localAddress.textContent = 'Local address: http://' + status.mdnsHost + '/';
        localAddress.className = 'small';
      } else {
        localAddress.textContent = '';
        localAddress.className = 'small hidden';
      }
      if (status.mode === 'portal') {
        $('dot').className = 'dot portal';
        $('state').textContent = 'Setup active · ' + status.ip;
      } else if (status.mode === 'station') {
        $('dot').className = 'dot online';
        $('state').textContent = 'Connected · ' + status.ip;
      } else {
        $('dot').className = 'dot';
        $('state').textContent = 'Clock connection unavailable';
      }
    }
    async function refreshStatus() {
      if (leaving || statusBusy) return;
      statusBusy = true;
      try {
        const status = await request('/api/v1/status');
        if (!leaving) setStatus(status);
      } catch (error) {
        if (!leaving) {
          $('dot').className = 'dot';
          $('state').textContent = 'Clock connection lost';
          $('localAddress').textContent = '';
          $('localAddress').className = 'small hidden';
        }
      } finally {
        statusBusy = false;
      }
    }
    function updatePasswordHint() {
      $('password').placeholder = hasSavedPassword && $('ssid').value === savedSsid
        ? 'Leave blank to keep this network’s saved password'
        : 'Enter password; leave blank for open Wi-Fi';
    }
    function populateTimezonePresets() {
      const menu = $('timezonePreset');
      for (const preset of TIMEZONE_PRESETS) menu.add(new Option(preset[1], preset[0]));
      menu.value = 'london';
    }
    function selectedTimezonePreset() {
      return TIMEZONE_PRESETS.find(preset => preset[0] === $('timezonePreset').value) ||
        TIMEZONE_PRESETS.find(preset => preset[0] === 'london');
    }
    function selectedTimezoneRule() {
      if ($('dstMode').value === 'custom') return $('customTimezone').value.trim();
      const preset = selectedTimezonePreset();
      return preset[$('dstMode').value === 'disabled' ? 2 : 3];
    }
    function loadTimezoneRule(rule) {
      let preset = TIMEZONE_PRESETS.find(candidate => candidate[3] === rule);
      if (preset) {
        $('timezonePreset').value = preset[0];
        $('dstMode').value = 'automatic';
        $('customTimezone').value = '';
        return;
      }
      preset = TIMEZONE_PRESETS.find(candidate => candidate[2] === rule);
      if (preset) {
        $('timezonePreset').value = preset[0];
        $('dstMode').value = 'disabled';
        $('customTimezone').value = '';
        return;
      }
      $('timezonePreset').value = 'london';
      $('dstMode').value = 'custom';
      $('customTimezone').value = rule;
    }
    function updateControls() {
      const locked = actionBusy || !configLoaded || leaving;
      for (const id of ['networks', 'ssid', 'password', 'syncTime', 'timezonePreset', 'dstMode', 'ntpHost', 'sendTime']) {
        $(id).disabled = locked;
      }
      $('customTimezone').disabled = locked || $('dstMode').value !== 'custom';
      $('scan').disabled = locked || scanBusy;
      $('factoryReset').disabled = locked;
    }
    async function load() {
      updateControls();
      try {
        const config = await request('/api/v1/config');
        if (leaving) return;
        if (!/^[0-9a-f]{32}$/.test(config.requestToken || '')) throw Error('Reload the clock setup page.');
        requestToken = config.requestToken;
        savedSsid = config.ssid || '';
        hasSavedPassword = config.hasPassword === true;
        $('ssid').value = savedSsid;
        $('ntpHost').value = config.ntpHost === DEFAULT_NTP ? '' : (config.ntpHost || '');
        loadTimezoneRule(config.timezone || DEFAULT_TZ);
        const syncTime = String(config.syncHour ?? 10).padStart(2, '0') + ':' +
          String(config.syncMinute ?? 0).padStart(2, '0');
        $('syncTime').value = ['09:00', '10:00', '21:00', '22:00'].includes(syncTime)
          ? syncTime : '10:00';
        configLoaded = true;
        updatePasswordHint();
        updateControls();
        await keepAlive();
        await scan();
      } catch (error) {
        if (!leaving) notice('Could not load settings. Reconnect to the clock and refresh this page.', 'bad');
      }
    }
    async function scan() {
      if (scanBusy || actionBusy || leaving) return;
      scanBusy = true;
      updateControls();
      notice('Scanning nearby Wi-Fi networks…');
      try {
        let data = await request('/api/v1/scan?start=1');
        const started = Date.now();
        while (data.scanning) {
          if (leaving || actionBusy) return;
          if (Date.now() - started > 35000) throw Error('Scan timed out. Enter the network name manually or try again.');
          await new Promise(resolve => setTimeout(resolve, 750));
          if (leaving || actionBusy) return;
          data = await request('/api/v1/scan');
        }
        if (leaving || actionBusy) return;
        const strongest = new Map();
        for (const network of data.networks) {
          if (!network.ssid) continue;
          if (!strongest.has(network.ssid) || strongest.get(network.ssid).rssi < network.rssi) {
            strongest.set(network.ssid, network);
          }
        }
        const networks = [...strongest.values()].sort((a, b) => b.rssi - a.rssi);
        const menu = $('networks');
        menu.length = 0;
        menu.add(new Option('Select a network', ''));
        for (const network of networks) {
          // Prefix values so an actual SSID cannot collide with our manual option.
          menu.add(new Option(network.ssid + ' (' + network.rssi + ' dBm)', 'ssid:' + network.ssid));
        }
        menu.add(new Option('Enter manually / hidden SSID', 'manual'));
        menu.value = strongest.has($('ssid').value) ? 'ssid:' + $('ssid').value : 'manual';
        notice(networks.length + ' network(s) found.', 'ok');
      } catch (error) {
        if (!leaving && !actionBusy) notice(error.message || 'Network scan failed. Enter the network name manually.', 'bad');
      } finally {
        scanBusy = false;
        updateControls();
      }
    }
    $('networks').onchange = event => {
      if (event.target.value === 'manual') $('ssid').focus();
      else if (event.target.value.startsWith('ssid:')) $('ssid').value = event.target.value.slice(5);
      updatePasswordHint();
    };
    $('ssid').oninput = updatePasswordHint;
    $('dstMode').onchange = () => {
      if ($('dstMode').value === 'custom' && !$('customTimezone').value.trim()) {
        $('customTimezone').value = selectedTimezonePreset()[3];
      }
      updateControls();
      if ($('dstMode').value === 'custom') $('customTimezone').focus();
    };
    $('scan').onclick = scan;
    const settings = () => {
      const [syncHour, syncMinute] = $('syncTime').value.split(':');
      return new URLSearchParams({
        ssid: $('ssid').value, password: $('password').value,
        ntpHost: $('ntpHost').value.trim(), timezone: selectedTimezoneRule(),
        syncHour, syncMinute
      });
    };
    function stopPolling() {
      leaving = true;
      clearInterval(heartbeatTimer);
      clearInterval(statusTimer);
    }
    function showRestart(message) {
      stopPolling();
      $('password').value = '';
      $('dot').className = 'dot';
      $('state').textContent = 'Clock restarting…';
      notice(message, 'ok');
      updateControls();
    }
    $('sendTime').onclick = async () => {
      if (actionBusy || leaving || !configLoaded) return;
      if (!$('ssid').value) {
        notice('Choose a Wi-Fi network or enter its name.', 'bad');
        $('ssid').focus();
        return;
      }
      if ($('dstMode').value === 'custom' && !$('customTimezone').value.trim()) {
        notice('Enter a custom POSIX time-zone rule.', 'bad');
        $('customTimezone').focus();
        return;
      }
      actionBusy = true;
      updateControls();
      notice('Saving settings…');
      try {
        await request('/api/v1/config', {
          method: 'POST',
          headers: {'Content-Type': 'application/x-www-form-urlencoded'},
          body: settings()
        });
        showRestart('Settings saved. The clock will restart and try to update its time. This page may become unreachable.');
      } catch (error) {
        if (!error.status || error.status === 403) stopPolling();
        notice('Save was not confirmed. Reconnect and check the saved settings before trying again. ' + error.message, 'bad');
      } finally {
        actionBusy = false;
        updateControls();
      }
    };
    $('factoryReset').onclick = async () => {
      if (actionBusy || leaving) return;
      if (!confirm('Erase all saved Wi-Fi, time-server, time-zone and update-time settings? The clock will restart.')) return;
      actionBusy = true;
      updateControls();
      try {
        await request('/api/v1/factory-reset', {method: 'POST'});
        showRestart('Saved settings erased. Reconnect to the clock setup network after it restarts.');
      } catch (error) {
        if (!error.status || error.status === 403) stopPolling();
        notice('Reset was not confirmed. Reconnect to the clock and check. ' + error.message, 'bad');
      } finally {
        actionBusy = false;
        updateControls();
      }
    };
    async function keepAlive() {
      if (leaving || heartbeatBusy || !requestToken) return;
      heartbeatBusy = true;
      try {
        await request('/api/v1/keepalive', {
          method: 'POST',
          headers: {'Content-Type': 'application/x-www-form-urlencoded'},
          body: new URLSearchParams({start: webSessionStarted ? '0' : '1', page: pageId})
        });
        webSessionStarted = true;
      } catch (error) {
        // The firmware's session expiry covers lost requests.
        if (error.status === 403 && !leaving) {
          stopPolling();
          notice('This setup page has expired. Reconnect to the clock and refresh it.', 'bad');
          updateControls();
        }
      } finally {
        heartbeatBusy = false;
      }
    }
    function closeSession() {
      stopPolling();
      if (!requestToken) return;
      const url = '/api/v1/session/close';
      const body = new URLSearchParams({page: pageId, token: requestToken});
      if (navigator.sendBeacon && navigator.sendBeacon(url, body)) return;
      fetch(url, {method: 'POST', body, keepalive: true}).catch(() => {});
    }
    addEventListener('pagehide', closeSession);
    // A page restored from the back/forward cache needs a new browser session.
    addEventListener('pageshow', event => { if (event.persisted) location.reload(); });
    populateTimezonePresets();
    heartbeatTimer = setInterval(keepAlive, 2000);
    statusTimer = setInterval(refreshStatus, 2000);
    refreshStatus();
    load();
  </script>
</body>
</html>

)CH899_WEB";
