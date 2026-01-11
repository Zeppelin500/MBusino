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
        <p style='text-align:right'><a href='/update' style='color:#3F4CFB'>update</a></p>
        <p style='text-align:right'><a href='https://www.github.com/zeppelin500/mbusino' style='color:#3F4CFB'>MBusino</a></p>
      </form>
    </main>
  </body>
</html>)rawliteral";

char html_buffer[sizeof(index_html)+200] = {0};


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
        </div><br><label for='newAddress'>New M-Bus Address:</label><br><select name='newAddress' id='newAddress'>
          <option value='1'>1</option>
          <option value='2'>2</option>
          <option value='3'>3</option>
          <option value='4'>4</option>
          <option value='5'>5</option>           
          <option value='0' selected>0</option>
        </select><br><br>
        <br>
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

const size_t update_htmlLength = strlen_P(update_html);