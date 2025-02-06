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
        <div class='form-floating'><label>SSID</label><input type='text' value='%s' class='form-control' maxlength = '29' name='ssid'></div>
        <div class='form-floating'><label>Password</label><input type='password' class='form-control' maxlength = '29' name='password'></div>
        <div class='form-floating'><label>Device Name</label><input type='text' value='%s' class='form-control' maxlength = '10' name='name'>
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
        </select>  stored: %u <br>       
        </div><br><label for='telegramDebug'>publish M-Bus telegram bytwise:</label><br><select name='telegramDebug' id='telegramDebug'>
          <option value='0'>(0) no</option>
          <option value='1'>(1) yes</option>
          <option value='' selected>choose option</option>
        </select>  stored: %u <br><br>             
        <div class='form-floating'><label>Sensor publish interval sec.</label><input type='text' value='%u' class='form-control' name='sensorInterval'></div>
        <div class='form-floating'><label>M-Bus publish interval sec.</label><input type='text' value='%u' class='form-control' name='mbusInterval'></div>
        <div class='form-floating'><label>MQTT Broker</label><input type='text' value='%s' class='form-control' maxlength = '19' name='broker'></div>
        <div class='form-floating'><label>MQTT Port</label><input type='text' value='%u' class='form-control' name='mqttPort'></div>
        <div class='form-floating'><label>MQTT User (optional)</label><input type='text' value='%s' class='form-control' maxlength = '29' name='mqttUser'></div>
        <div class='form-floating'><label>MQTT Password (optional)</label><input type='password' class='form-control' maxlength = '29' name='mqttPswrd'></div>
        <br>
        <button type='submit'>Save</button>
        <p style='text-align:right'><a href='/update' style='color:#3F4CFB'>update</a></p>
        <p style='text-align:right'><a href='https://www.github.com/zeppelin500/mbusino' style='color:#3F4CFB'>MBusino</a></p>
      </form>
    </main>
  </body>
</html>)rawliteral";

char html_buffer[sizeof(index_html)+200] = {0};

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
    </style>
  </head>
  <body>
    <main class='form-signin'>
       	<h1 class=''><i>MBusino</i> update</h1><br>
          <p style='text-align:center'> select a xxx.bin file </p><br>
			    <form style='text-align:center' method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'>
         	</form><br/>
          <p style='text-align:center'> MBusino will restart after update </p><br>
          <p style='text-align:center'><a href='/' style='color:#3F4CFB'>home</a></p>
    </main>
  </body>
</html>)rawliteral";