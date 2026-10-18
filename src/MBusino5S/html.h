const char index_html[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang='en'>
  <head>
    <meta charset='utf-8'>
    <meta name='viewport' content='width=device-width,initial-scale=1'>
    <title>MBusino Setup</title>
    <style>
      *,
      ::after,
      ::before {
        box-sizing: border-box
      }

      body {
        margin: 0;
        font-family: 'Segoe UI', Roboto, 'Helvetica Neue', Arial, 'Noto Sans', 'Liberation Sans';
        font-size: 1rem;
        font-weight: 400;
        line-height: 1.5;
        color: #fff;
        background-color: #438287
      }

      .form-control {
        display: block;
        width: 100%%;
        height: calc(1.5em + .75rem + 2px);
        border: 1px solid #ced4da
      }

      button {
        cursor: pointer;
        border: 1px solid transparent;
        color: #fff;
        background-color: #304648;
        border-color: #304648;
        padding: .5rem 1rem;
        font-size: 1.25rem;
        line-height: 1.5;
        border-radius: .3rem;
        width: 100%%
      }

      .form-signin {
        width: 100%%;
        max-width: 400px;
        padding: 15px;
        margin: auto
      }

      h1 {
        text-align: center
      }
    </style>
  </head>
  <body>
    <main class='form-signin'>
      <form action='/get'>
        <h1 class=''><i>MBusino</i> Setup</h1><br>
        <div class='form-floating'><label>SSID</label><input type='text' value='%s' class='form-control' maxlength = '64' name='ssid'></div>
        <div class='form-floating'><label>Password</label><input type='password' class='form-control' maxlength = '64' name='password'></div>
        <div class='form-floating'><label>Device Name</label><input type='text' value='%s' class='form-control' maxlength = '30' name='name'>
        </div><br><label for='extension'>Stage of Extension:</label><br><select name='extension' id='extension'>
          <option value='5'>(5) 5x DS18B20 + BME</option>
          <option value='7'>(7) 7x DS18B20 no BME</option>
          <option value='0'>(0) only M-Bus</option>
          <option value='' selected>choose option</option>
        </select>  stored: %u <br>
        </div><br><label for='haAd'>generate Home-Assitant autodiscovery messages:</label><br><select name='haAd' id='haAd'>
          <option value='0'>(0) no</option>
          <option value='1'>(1) yes</option>
          <option value='' selected>choose option</option>
        </select>  stored: %u <br><br> 
        </div><br><label for='telegramDebug'>publish M-Bus telegram bytwise:</label><br><select name='telegramDebug' id='telegramDebug'>
          <option value='0'>(0) no</option>
          <option value='1'>(1) yes</option>
          <option value='' selected>choose option</option>
        </select>  stored: %u <br><br>         
        <div class='form-floating'><label>Sensor publish interval sec.</label><input type='text' value='%u' class='form-control' name='sensorInterval'></div>
        <div class='form-floating'><label>M-Bus publish interval sec.</label><input type='text' value='%u' class='form-control' name='mbusInterval'></div>
        <div class='form-floating'><label>MQTT Broker</label><input type='text' value='%s' class='form-control' maxlength = '64' name='broker'></div>
        <div class='form-floating'><label>MQTT Port</label><input type='text' value='%u' class='form-control' name='mqttPort'></div>
        <div class='form-floating'><label>MQTT User (optional)</label><input type='text' value='%s' class='form-control' maxlength = '64' name='mqttUser'></div>
        <div class='form-floating'><label>MQTT Password (optional)</label><input type='password' class='form-control' maxlength = '64' name='mqttPswrd'></div>
        <div class='form-floating'><label>number of M-Bus Slaves</label><input type='text' value='%u' class='form-control' name='mbusSlaves'></div>
        <div class='form-floating'><label>M-Bus address 1</label><input type='text' value='%u' class='form-control' name='mbusAddress1'></div>
        <div class='form-floating'><label>M-Bus address 2</label><input type='text' value='%u' class='form-control' name='mbusAddress2'></div>
        <div class='form-floating'><label>M-Bus address 3</label><input type='text' value='%u' class='form-control' name='mbusAddress3'></div>
        <div class='form-floating'><label>M-Bus address 4</label><input type='text' value='%u' class='form-control' name='mbusAddress4'></div>
        <div class='form-floating'><label>M-Bus address 5</label><input type='text' value='%u' class='form-control' name='mbusAddress5'></div>
        <br>
        <button type='submit'>Save</button>
        <p style='text-align:right'><a href='/setaddress' style='color:#3F4CFB'>set M-Bus Address</a></p>
        <p style='text-align:right'><a href='/mbus' style='color:#3F4CFB'>M-Bus telegrams</a></p>
        <p style='text-align:right'><a href='/update' style='color:#3F4CFB'>update</a></p>
        <p style='text-align:right'><a href='https://www.github.com/zeppelin500/mbusino' style='color:#3F4CFB'>MBusino</a></p>
      </form>
    </main>
  </body>
</html>)rawliteral";

char html_buffer[sizeof(index_html)+512] = {0};


const char setAddress_html[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang='en'>
  <head>
    <meta charset='utf-8'>
    <meta name='viewport' content='width=device-width,initial-scale=1'>
    <title>MBusino Setup</title>
    <style>
      *,
      ::after,
      ::before {
        box-sizing: border-box
      }

      body {
        margin: 0;
        font-family: 'Segoe UI', Roboto, 'Helvetica Neue', Arial, 'Noto Sans', 'Liberation Sans';
        font-size: 1rem;
        font-weight: 400;
        line-height: 1.5;
        color: #fff;
        background-color: #438287
      }

      .form-control {
        display: block;
        width: 100%%;
        height: calc(1.5em + .75rem + 2px);
        border: 1px solid #ced4da
      }

      button {
        cursor: pointer;
        border: 1px solid transparent;
        color: #fff;
        background-color: #304648;
        border-color: #304648;
        padding: .5rem 1rem;
        font-size: 1.25rem;
        line-height: 1.5;
        border-radius: .3rem;
        width: 100%%
      }

      .form-signin {
        width: 100%%;
        max-width: 400px;
        padding: 15px;
        margin: auto
      }

      h1 {
        text-align: center
      }
    </style>
  </head>
  <body>
    <main class='form-signin'>
      <form action='/get'>
        <h1 class=''><i>MBusino</i> set M-Bus address</h1><br>
        <p style='text-align:center'> ! only one M-Bus Slave should be connected ! </p><br>
        <label for='newAddress'>New M-Bus Address (0-250):</label><br>
        <input id='newAddress' min='0' max='250' name='newAddress' type='number' value='0'> 
        <br><br><br>
        <button type='submit'>Save</button>
        <p style='text-align:right'><a href='/' style='color:#3F4CFB'>home</a></p>
      </form>
    </main>
  </body>
</html>)rawliteral";

const char update_html[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang='en'>
  <head>
    <meta charset='utf-8'>
    <meta name='viewport' content='width=device-width,initial-scale=1'>
    <title>MBusino update</title>
    <style>
      *,
      ::after,
      ::before {
        box-sizing: border-box
      }

      body {
        margin: 0;
        font-family: 'Segoe UI', Roboto, 'Helvetica Neue', Arial, 'Noto Sans', 'Liberation Sans';
        font-size: 1rem;
        font-weight: 400;
        line-height: 1.5;
        color: #fff;
        background-color: #438287
      }

      h1 {
        text-align: center
      }

      #ustate {
        text-align: center;
        min-height: 1.5em
      }

      #ustate.err {
        color: #ffd2d2
      }

      #ustate.ok {
        color: #d7ffe0
      }

      #ubar {
        display: none;
        width: 80%;
        max-width: 420px;
        height: 18px;
        margin: 10px auto;
        border: 1px solid #fff;
        border-radius: 9px;
        overflow: hidden;
        background: rgba(255, 255, 255, .18)
      }

      #ubarfill {
        height: 100%;
        width: 0;
        background: #fff;
        transition: width .2s
      }
    </style>
  </head>
  <body>
    <main class='form-signin'>
       	<h1 class=''><i>MBusino</i> update</h1><br>
          <p style='text-align:center'> select a xxx.bin file </p><br>
			    <form id='uform' style='text-align:center' method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update' id='ufile' accept='.bin'><input type='submit' id='ubtn' value='Update'>
         	</form><br/>
          <div id='ubar'><div id='ubarfill'></div></div>
          <p id='ustate'></p>
          <p style='text-align:center'> MBusino will restart after update </p><br>
          <p style='text-align:center'><a href='/' style='color:#3F4CFB'>home</a></p>
    </main>
    <script>
      (function () {
        var form = document.getElementById('uform'),
            fileIn = document.getElementById('ufile'),
            btn = document.getElementById('ubtn'),
            bar = document.getElementById('ubar'),
            fill = document.getElementById('ubarfill'),
            state = document.getElementById('ustate'),
            xhr = null,
            deadline = 0,
            waiting = false;

        function say(text, cls) {
          state.textContent = text;
          state.className = cls || '';
        }

        function enable() {
          btn.disabled = false;
          fileIn.disabled = false;
        }

        function pollBack() {
          if (Date.now() > deadline) {
            waiting = false;
            say('The device did not come back within 60 s. Check power and network, then reload this page.', 'err');
            enable();
            return;
          }
          waiting = true;
          fetch('/?alive=' + Date.now(), { cache: 'no-store' }).then(function () {
            waiting = false;
            location.href = '/';
          }).catch(function () {
            waiting = false;
            setTimeout(pollBack, 400);
          });
        }

        form.addEventListener('submit', function (e) {
          e.preventDefault();
          if (xhr) { return; }
          if (!fileIn.files.length) { say('Please select a .bin file first.', 'err'); return; }
          if (!/\.bin$/i.test(fileIn.files[0].name)) { say('Only .bin files can be flashed.', 'err'); return; }

          var fd = new FormData(form);
          xhr = new XMLHttpRequest();
          btn.disabled = true;
          fileIn.disabled = true;
          bar.style.display = 'block';
          fill.style.width = '0%';
          say('Uploading and flashing... keep the device powered and do not close this page.');

          xhr.upload.addEventListener('progress', function (ev) {
            if (ev.lengthComputable) {
              var pct = Math.round(ev.loaded / ev.total * 100);
              fill.style.width = pct + '%';
              if (pct >= 100) {
                say('Uploaded. Verifying and rebooting...');
              } else {
                say('Uploading and flashing ' + pct + '% - keep the device powered.');
              }
            }
          });

          xhr.addEventListener('load', function () {
            var txt = (xhr.responseText || '').replace(/^\s+|\s+$/g, '');
            if (txt.indexOf('success') === 0) {
              fill.style.width = '100%';
              say('Update successful - waiting for the device to come back...', 'ok');
              deadline = Date.now() + 60000;
              xhr = null;
              pollBack();
            } else {
              say('Update failed: ' + (txt || 'no answer from the device'), 'err');
              xhr = null;
              enable();
            }
          });

          xhr.addEventListener('error', function () {
            say('Connection lost during the update. Check the device, then reload this page.', 'err');
            xhr = null;
            enable();
          });

          xhr.addEventListener('timeout', function () {
            say('The update took longer than 3 minutes - aborted. Check the connection and try again.', 'err');
            xhr = null;
            enable();
          });

          xhr.addEventListener('abort', function () {
            xhr = null;
            enable();
            say('Upload aborted.');
          });

          xhr.open('POST', '/update');
          xhr.timeout = 180000;
          xhr.send(fd);
        });
      })();
    </script>
  </body>
</html>)rawliteral";

const size_t update_htmlLength = strlen_P(update_html);

const char mbus_html[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang='en'>
  <head>
    <meta charset='utf-8'>
    <meta name='viewport' content='width=device-width,initial-scale=1'>
    <title>MBusino M-Bus</title>
    <style>
      body {
        margin: 0;
        padding: 15px;
        font-family: 'Segoe UI', Roboto, 'Helvetica Neue', Arial, 'Noto Sans', 'Liberation Sans';
        font-size: 1rem;
        line-height: 1.5;
        color: #fff;
        background-color: #438287
      }

      main {
        max-width: 960px;
        margin: auto
      }

      h1 {
        text-align: center
      }

      h2 {
        font-size: 1.2rem;
        margin: 1.5rem 0 .5rem
      }

      table {
        border-collapse: collapse;
        width: 100%
      }

      th,
      td {
        text-align: left;
        vertical-align: top;
        padding: .2rem .5rem;
        border-bottom: 1px solid #6aa5aa
      }

      .wrap {
        overflow-x: auto
      }

      .hex {
        font-family: monospace;
        word-break: break-all
      }

      .ok {
        color: #c8f7c8
      }

      .err {
        color: #ffc8c8
      }

      button {
        cursor: pointer;
        color: #fff;
        background-color: #304648;
        border: 1px solid #304648;
        padding: .3rem 1rem;
        font-size: 1rem;
        border-radius: .3rem
      }
    </style>
  </head>
  <body>
    <main>
      <h1><i>MBusino</i> M-Bus</h1>
      <p><span id='info'>loading...</span> <button id='poll' onclick='poll()'>Poll now</button></p>
      <h2>Last decoded telegram</h2>
      <div class='wrap'><table id='header'></table></div><br>
      <div class='wrap'><table id='records'></table></div>
      <h2>Received telegrams</h2>
      <div class='wrap'><table id='log'></table></div>
      <p style='text-align:right'><a href='/' style='color:#3F4CFB'>home</a></p>
    </main>
    <script>
      const STATUS = ['OK', 'no data received', 'bad frame'];
      function esc(v) { return String(v ?? '').replace(/[&<>"']/g, c => '&#' + c.charCodeAt(0) + ';') }
      function age(ms) { const s = Math.round(ms / 1000); return s < 120 ? s + ' s ago' : Math.round(s / 60) + ' min ago' }
      function row(cells, th) { const t = th ? 'th' : 'td'; return '<tr>' + cells.map(c => '<' + t + '>' + c + '</' + t + '>').join('') + '</tr>' }
      function render(d) {
        document.getElementById('info').textContent = 'Polls since start: ' + d.count + ', ' + (d.next < 0 ? 'polling...' : 'next poll in ' + d.next + ' s');
        let h = row(['no telegram decoded yet']), r = '';
        if (d.last) {
          const hd = d.last.data.header || {};
          h = row(['Header, address ' + d.last.a + ', ' + age(d.now - d.last.t), ''], 1);
          for (const k in hd) {
            if (hd[k] && typeof hd[k] === 'object') { for (const s in hd[k]) h += row([esc(s), esc(hd[k][s])]) }
            else h += row([esc(k), esc(hd[k])]);
          }
          r = row(['#', 'name', 'value', 'unit', 'storage', 'tariff', 'subunit'], 1);
          (d.last.data.records || []).forEach((x, i) => {
            r += row([i + 1, esc(x.name), esc(x.value_string || x.value_scaled), esc(x.units), esc(x.storage), esc(x.tariff), esc(x.subUnit)]);
          });
        }
        document.getElementById('header').innerHTML = h;
        document.getElementById('records').innerHTML = r;
        let l = row(['received', 'address', 'status', 'bytes', 'telegram (hex)'], 1);
        d.log.forEach(e => {
          l += row([age(d.now - e.t), e.a, '<span class=' + (e.s ? 'err' : 'ok') + '>' + STATUS[e.s] + '</span>', e.hex.length / 2,
            '<span class=hex>' + e.hex.replace(/(..)(?!$)/g, '$1 ') + '</span>']);
        });
        document.getElementById('log').innerHTML = l;
      }
      async function poll() {
        document.getElementById('poll').disabled = true;
        try { await fetch('/mbus/poll', { method: 'POST' }) } catch (e) {}
        setTimeout(() => document.getElementById('poll').disabled = false, 3000);
      }
      async function update() {
        try { const res = await fetch('/mbus.json'); render(await res.json()) }
        catch (e) { document.getElementById('info').textContent = 'no connection to MBusino' }
        setTimeout(update, 2000);
      }
      update();
    </script>
  </body>
</html>)rawliteral";
