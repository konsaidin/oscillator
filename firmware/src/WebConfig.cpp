#include "WebConfig.h"
#include "PowerAnalyzer.h"

WebConfig webConfig;

// CSS styles for mobile-responsive design
const char CSS_STYLES[] PROGMEM = R"rawliteral(
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:#1a1a2e;color:#eee;min-height:100vh;padding:20px}
.container{max-width:500px;margin:0 auto}
h1{text-align:center;color:#0ff;margin-bottom:20px;font-size:1.5rem}
h2{color:#0ff;margin:20px 0 15px;font-size:1.2rem;border-bottom:1px solid #333;padding-bottom:10px}
.card{background:#16213e;border-radius:12px;padding:20px;margin-bottom:20px;box-shadow:0 4px 6px rgba(0,0,0,0.3)}
label{display:block;margin-bottom:5px;color:#aaa;font-size:0.9rem}
input,select{width:100%;padding:12px;border:1px solid #333;border-radius:8px;background:#0f0f23;color:#fff;font-size:16px;margin-bottom:15px}
input:focus,select:focus{outline:none;border-color:#0ff}
button{width:100%;padding:14px;border:none;border-radius:8px;font-size:16px;cursor:pointer;margin-bottom:10px;transition:all 0.3s}
.btn-primary{background:linear-gradient(135deg,#00d4ff,#0099cc);color:#000;font-weight:bold}
.btn-primary:hover{transform:translateY(-2px);box-shadow:0 4px 12px rgba(0,212,255,0.4)}
.btn-secondary{background:#333;color:#fff}
.btn-secondary:hover{background:#444}
.btn-danger{background:#e74c3c;color:#fff}
.btn-danger:hover{background:#c0392b}
.status{display:flex;justify-content:space-between;padding:10px 0;border-bottom:1px solid #333}
.status:last-child{border-bottom:none}
.status-label{color:#888}
.status-value{color:#0ff;font-weight:bold}
.status-ok{color:#2ecc71}
.status-warn{color:#f39c12}
.status-error{color:#e74c3c}
.wifi-list{max-height:200px;overflow-y:auto;margin-bottom:15px}
.wifi-item{display:flex;justify-content:space-between;align-items:center;padding:12px;background:#0f0f23;border-radius:8px;margin-bottom:8px;cursor:pointer;transition:all 0.2s}
.wifi-item:hover{background:#1a1a3e;transform:translateX(5px)}
.wifi-name{font-weight:500}
.wifi-signal{font-size:0.8rem;color:#888}
.signal-strong{color:#2ecc71}
.signal-medium{color:#f39c12}
.signal-weak{color:#e74c3c}
.nav{display:flex;gap:10px;margin-bottom:20px}
.nav a{flex:1;text-align:center;padding:12px;background:#333;color:#fff;text-decoration:none;border-radius:8px;transition:all 0.2s}
.nav a:hover,.nav a.active{background:#0ff;color:#000}
.loader{border:3px solid #333;border-top:3px solid #0ff;border-radius:50%;width:24px;height:24px;animation:spin 1s linear infinite;margin:20px auto}
@keyframes spin{0%{transform:rotate(0deg)}100%{transform:rotate(360deg)}}
.hidden{display:none}
.msg{padding:12px;border-radius:8px;margin-bottom:15px;text-align:center}
.msg-success{background:#1e4620;color:#2ecc71}
.msg-error{background:#4a1515;color:#e74c3c}
.phase-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:10px;margin-bottom:15px}
.phase-box{background:#0f0f23;padding:15px;border-radius:8px;text-align:center}
.phase-label{font-size:0.8rem;color:#888;margin-bottom:5px}
.phase-value{font-size:1.5rem;font-weight:bold;color:#0ff}
.phase-unit{font-size:0.7rem;color:#666}
@media(max-width:400px){.phase-grid{grid-template-columns:1fr}.phase-box{padding:10px}}
</style>
)rawliteral";

// JavaScript for dynamic functionality
const char JS_CODE[] PROGMEM = R"rawliteral(
<script>
function scanWifi(){
  document.getElementById('wifi-list').innerHTML='<div class="loader"></div>';
  fetch('/scan').then(r=>r.json()).then(d=>{
    let html='';
    d.networks.forEach(n=>{
      let sig=n.rssi>-50?'strong':n.rssi>-70?'medium':'weak';
      html+=`<div class="wifi-item" onclick="selectWifi('${n.ssid}')">
        <span class="wifi-name">${n.ssid}${n.secure?' 🔒':''}</span>
        <span class="wifi-signal signal-${sig}">${n.rssi}dBm</span>
      </div>`;
    });
    document.getElementById('wifi-list').innerHTML=html||'<p>No networks found</p>';
  }).catch(e=>{
    document.getElementById('wifi-list').innerHTML='<p class="status-error">Scan failed</p>';
  });
}
function selectWifi(ssid){
  document.getElementById('ssid').value=ssid;
  document.getElementById('password').focus();
}
function connectWifi(e){
  e.preventDefault();
  let ssid=document.getElementById('ssid').value;
  let pass=document.getElementById('password').value;
  if(!ssid){alert('Enter SSID');return;}
  document.getElementById('conn-btn').disabled=true;
  document.getElementById('conn-btn').textContent='Connecting...';
  fetch('/connect',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'ssid='+encodeURIComponent(ssid)+'&password='+encodeURIComponent(pass)
  }).then(r=>r.json()).then(d=>{
    if(d.success){
      showMsg('Connected! IP: '+d.ip,'success');
      setTimeout(()=>location.reload(),2000);
    }else{
      showMsg('Connection failed: '+d.error,'error');
    }
  }).catch(e=>showMsg('Error: '+e,'error'))
  .finally(()=>{
    document.getElementById('conn-btn').disabled=false;
    document.getElementById('conn-btn').textContent='Connect';
  });
}
function saveConfig(e){
  e.preventDefault();
  let form=document.getElementById('config-form');
  let data=new FormData(form);
  let params=new URLSearchParams(data);
  document.getElementById('save-btn').disabled=true;
  document.getElementById('save-btn').textContent='Saving...';
  fetch('/saveconfig',{method:'POST',body:params})
  .then(r=>r.json()).then(d=>{
    if(d.success){showMsg('Configuration saved!','success');}
    else{showMsg('Save failed','error');}
  }).catch(e=>showMsg('Error: '+e,'error'))
  .finally(()=>{
    document.getElementById('save-btn').disabled=false;
    document.getElementById('save-btn').textContent='Save Configuration';
  });
}
function showMsg(text,type){
  let el=document.getElementById('msg');
  el.className='msg msg-'+type;
  el.textContent=text;
  el.classList.remove('hidden');
  setTimeout(()=>el.classList.add('hidden'),5000);
}
function updateStatus(){
  fetch('/status').then(r=>r.json()).then(d=>{
    if(d.phases){
      ['a','b','c'].forEach((p,i)=>{
        let el=document.getElementById('v'+p);
        if(el)el.textContent=d.phases[i].voltage.toFixed(1);
      });
      let fel=document.getElementById('freq');
      if(fel)fel.textContent=d.frequency.toFixed(2);
    }
    let wst=document.getElementById('wifi-status');
    if(wst)wst.textContent=d.wifi_connected?'Connected':'Disconnected';
    let ip=document.getElementById('ip-addr');
    if(ip)ip.textContent=d.ip||'N/A';
  }).catch(e=>console.log(e));
}
function reboot(){
  if(confirm('Reboot device?')){
    fetch('/reboot',{method:'POST'}).then(()=>{
      showMsg('Rebooting...','success');
      setTimeout(()=>location.reload(),5000);
    });
  }
}
if(document.getElementById('wifi-list'))scanWifi();
setInterval(updateStatus,2000);
updateStatus();
</script>
)rawliteral";

WebConfig::WebConfig() : _server(80), _apMode(false), _lastScanTime(0), _scanInProgress(false) {
    _cachedScanResults = "{\"networks\":[]}";
    _calibA = CALIBRATION_COEFF_A;
    _calibB = CALIBRATION_COEFF_B;
    _calibC = CALIBRATION_COEFF_C;
    _influxURL = INFLUXDB_URL;
    _influxOrg = INFLUXDB_ORG;
    _influxBucket = INFLUXDB_BUCKET;
    _influxToken = INFLUXDB_TOKEN;
    _deviceID = DEVICE_ID;
}

void WebConfig::begin() {
    // Setup HTTP routes
    _server.on("/", HTTP_GET, [this]() { handleRoot(); });
    _server.on("/scan", HTTP_GET, [this]() { handleScan(); });
    _server.on("/connect", HTTP_POST, [this]() { handleConnect(); });
    _server.on("/config", HTTP_GET, [this]() { handleConfig(); });
    _server.on("/saveconfig", HTTP_POST, [this]() { handleSaveConfig(); });
    _server.on("/status", HTTP_GET, [this]() { handleStatus(); });
    _server.on("/reboot", HTTP_POST, [this]() { handleReboot(); });
    _server.onNotFound([this]() { handleNotFound(); });
    
    _server.begin();
    Serial.println("[WebConfig] HTTP server started on port 80");
}

void WebConfig::handle() {
    if (_apMode) {
        _dnsServer.processNextRequest();
    }
    _server.handleClient();
}

void WebConfig::startAPMode() {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    _apMode = true;
    
    IPAddress apIP = WiFi.softAPIP();
    
    // Start DNS server for Captive Portal - redirect all domains to our IP
    _dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    _dnsServer.start(DNS_PORT, "*", apIP);
    
    Serial.printf("[WebConfig] AP Mode started. SSID: %s, IP: %s\n", AP_SSID, apIP.toString().c_str());
    Serial.println("[WebConfig] Captive Portal DNS started - all domains redirect to panel");
}

bool WebConfig::connectToSavedWiFi() {
    if (_ssid.length() == 0) {
        Serial.println("[WebConfig] No saved WiFi credentials");
        return false;
    }
    
    Serial.printf("[WebConfig] Connecting to: %s\n", _ssid.c_str());
    
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);  // Включаем автопереподключение
    WiFi.persistent(true);        // Сохраняем настройки WiFi
    WiFi.begin(_ssid.c_str(), _password.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < WIFI_CONNECT_TIMEOUT_SEC * 2) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[WebConfig] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
        _apMode = false;
        return true;
    }
    
    Serial.println("[WebConfig] Connection failed");
    return false;
}

bool WebConfig::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

bool WebConfig::isAPMode() {
    return _apMode;
}

String WebConfig::getIPAddress() {
    if (_apMode) {
        return WiFi.softAPIP().toString();
    }
    return WiFi.localIP().toString();
}

String WebConfig::getSavedSSID() {
    return _ssid;
}

void WebConfig::loadConfig() {
    _prefs.begin("webconfig", true);  // Read-only
    
    _ssid = _prefs.getString("ssid", "");
    _password = _prefs.getString("password", "");
    _influxURL = _prefs.getString("influx_url", INFLUXDB_URL);
    _influxOrg = _prefs.getString("influx_org", INFLUXDB_ORG);
    _influxBucket = _prefs.getString("influx_bucket", INFLUXDB_BUCKET);
    _influxToken = _prefs.getString("influx_token", INFLUXDB_TOKEN);
    _deviceID = _prefs.getString("device_id", DEVICE_ID);
    _calibA = _prefs.getFloat("calib_a", CALIBRATION_COEFF_A);
    _calibB = _prefs.getFloat("calib_b", CALIBRATION_COEFF_B);
    _calibC = _prefs.getFloat("calib_c", CALIBRATION_COEFF_C);
    
    _prefs.end();
    
    Serial.println("[WebConfig] Configuration loaded");
}

void WebConfig::saveConfig() {
    _prefs.begin("webconfig", false);  // Read-write
    
    _prefs.putString("ssid", _ssid);
    _prefs.putString("password", _password);
    _prefs.putString("influx_url", _influxURL);
    _prefs.putString("influx_org", _influxOrg);
    _prefs.putString("influx_bucket", _influxBucket);
    _prefs.putString("influx_token", _influxToken);
    _prefs.putString("device_id", _deviceID);
    _prefs.putFloat("calib_a", _calibA);
    _prefs.putFloat("calib_b", _calibB);
    _prefs.putFloat("calib_c", _calibC);
    
    _prefs.end();
    
    Serial.println("[WebConfig] Configuration saved");
}

// Getters for configuration
float WebConfig::getCalibCoeffA() { return _calibA; }
float WebConfig::getCalibCoeffB() { return _calibB; }
float WebConfig::getCalibCoeffC() { return _calibC; }
String WebConfig::getInfluxURL() { return _influxURL; }
String WebConfig::getInfluxOrg() { return _influxOrg; }
String WebConfig::getInfluxBucket() { return _influxBucket; }
String WebConfig::getInfluxToken() { return _influxToken; }
String WebConfig::getDeviceID() { return _deviceID; }

String WebConfig::getHTMLHeader(const String& title) {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width,initial-scale=1,maximum-scale=1'>";
    html += "<title>" + title + "</title>";
    html += FPSTR(CSS_STYLES);
    html += "</head><body><div class='container'>";
    html += "<h1>⚡ Power Monitor</h1>";
    html += "<div class='nav'>";
    html += "<a href='/'>Home</a>";
    html += "<a href='/config'>Settings</a>";
    html += "</div>";
    return html;
}

String WebConfig::getHTMLFooter() {
    String html = FPSTR(JS_CODE);
    html += "</div></body></html>";
    return html;
}

String WebConfig::getMainPage() {
    String html = getHTMLHeader("Power Monitor");
    
    // Status card
    html += "<div class='card'><h2>📊 Live Data</h2>";
    html += "<div class='phase-grid'>";
    html += "<div class='phase-box'><div class='phase-label'>Phase A</div><div class='phase-value' id='va'>--</div><div class='phase-unit'>V</div></div>";
    html += "<div class='phase-box'><div class='phase-label'>Phase B</div><div class='phase-value' id='vb'>--</div><div class='phase-unit'>V</div></div>";
    html += "<div class='phase-box'><div class='phase-label'>Phase C</div><div class='phase-value' id='vc'>--</div><div class='phase-unit'>V</div></div>";
    html += "</div>";
    html += "<div class='status'><span class='status-label'>Frequency</span><span class='status-value'><span id='freq'>--</span> Hz</span></div>";
    html += "</div>";
    
    // WiFi card
    html += "<div class='card'><h2>📶 WiFi</h2>";
    html += "<div id='msg' class='msg hidden'></div>";
    html += "<div class='status'><span class='status-label'>Status</span><span class='status-value' id='wifi-status'>";
    html += isConnected() ? "Connected" : "Disconnected";
    html += "</span></div>";
    html += "<div class='status'><span class='status-label'>IP Address</span><span class='status-value' id='ip-addr'>";
    html += getIPAddress();
    html += "</span></div>";
    
    if (_apMode || !isConnected()) {
        html += "<h2>🔍 Available Networks</h2>";
        html += "<div id='wifi-list'><div class='loader'></div></div>";
        html += "<button class='btn-secondary' onclick='scanWifi()'>🔄 Rescan</button>";
        html += "<form onsubmit='connectWifi(event)'>";
        html += "<label>Network Name (SSID)</label>";
        html += "<input type='text' id='ssid' name='ssid' placeholder='Select or enter SSID' value='" + _ssid + "'>";
        html += "<label>Password</label>";
        html += "<input type='password' id='password' name='password' placeholder='WiFi password'>";
        html += "<button type='submit' id='conn-btn' class='btn-primary'>Connect</button>";
        html += "</form>";
    }
    html += "</div>";
    
    // Device info
    html += "<div class='card'><h2>ℹ️ Device</h2>";
    html += "<div class='status'><span class='status-label'>Device ID</span><span class='status-value'>" + _deviceID + "</span></div>";
    html += "<div class='status'><span class='status-label'>Firmware</span><span class='status-value'>v1.0.0</span></div>";
    html += "<div class='status'><span class='status-label'>Uptime</span><span class='status-value'>" + String(millis() / 1000) + "s</span></div>";
    html += "<button class='btn-danger' onclick='reboot()'>🔄 Reboot</button>";
    html += "</div>";
    
    html += getHTMLFooter();
    return html;
}

String WebConfig::getConfigPage() {
    String html = getHTMLHeader("Settings");
    
    html += "<div id='msg' class='msg hidden'></div>";
    
    // Calibration card
    html += "<div class='card'><h2>🔧 Calibration</h2>";
    html += "<form id='config-form' onsubmit='saveConfig(event)'>";
    html += "<label>Phase A Coefficient</label>";
    html += "<input type='number' step='0.001' name='calib_a' value='" + String(_calibA, 4) + "'>";
    html += "<label>Phase B Coefficient</label>";
    html += "<input type='number' step='0.001' name='calib_b' value='" + String(_calibB, 4) + "'>";
    html += "<label>Phase C Coefficient</label>";
    html += "<input type='number' step='0.001' name='calib_c' value='" + String(_calibC, 4) + "'>";
    
    // InfluxDB settings
    html += "<h2>📡 InfluxDB</h2>";
    html += "<label>Server URL</label>";
    html += "<input type='text' name='influx_url' value='" + _influxURL + "'>";
    html += "<label>Organization</label>";
    html += "<input type='text' name='influx_org' value='" + _influxOrg + "'>";
    html += "<label>Bucket</label>";
    html += "<input type='text' name='influx_bucket' value='" + _influxBucket + "'>";
    html += "<label>Token</label>";
    html += "<input type='password' name='influx_token' value='" + _influxToken + "'>";
    
    // Device settings
    html += "<h2>🏷️ Device</h2>";
    html += "<label>Device ID</label>";
    html += "<input type='text' name='device_id' value='" + _deviceID + "'>";
    
    html += "<button type='submit' id='save-btn' class='btn-primary'>💾 Save Configuration</button>";
    html += "</form></div>";
    
    html += getHTMLFooter();
    return html;
}

void WebConfig::handleRoot() {
    _server.send(200, "text/html", getMainPage());
}

void WebConfig::handleScan() {
    // Check if scan results are ready
    int n = WiFi.scanComplete();
    
    if (n == WIFI_SCAN_FAILED) {
        // No scan in progress, start async scan
        if (!_scanInProgress) {
            Serial.println("[WebConfig] Starting async WiFi scan...");
            WiFi.scanNetworks(true, false, false, 300);  // async=true, show_hidden=false, passive=false, max_ms=300
            _scanInProgress = true;
        }
        // Return cached results while scanning
        _server.send(200, "application/json", _cachedScanResults);
        return;
    }
    
    if (n == WIFI_SCAN_RUNNING) {
        // Scan still running, return cached
        _server.send(200, "application/json", _cachedScanResults);
        return;
    }
    
    // Scan complete, build results
    _scanInProgress = false;
    String json = "{\"networks\":[";
    
    for (int i = 0; i < n; i++) {
        if (i > 0) json += ",";
        // Escape SSID for JSON
        String ssid = WiFi.SSID(i);
        ssid.replace("\\", "\\\\");
        ssid.replace("\"", "\\\"");
        json += "{\"ssid\":\"" + ssid + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
        json += "\"secure\":" + String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN) + "}";
    }
    
    json += "]}";
    
    // Cache results and clean up
    _cachedScanResults = json;
    WiFi.scanDelete();
    
    Serial.printf("[WebConfig] Scan complete, found %d networks\n", n);
    _server.send(200, "application/json", json);
}

void WebConfig::handleConnect() {
    String ssid = _server.arg("ssid");
    String password = _server.arg("password");
    
    Serial.printf("[WebConfig] Attempting to connect to: %s\n", ssid.c_str());
    
    // Save credentials
    _ssid = ssid;
    _password = password;
    saveConfig();
    
    // Try to connect
    WiFi.begin(ssid.c_str(), password.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        attempts++;
    }
    
    String json;
    if (WiFi.status() == WL_CONNECTED) {
        _apMode = false;
        json = "{\"success\":true,\"ip\":\"" + WiFi.localIP().toString() + "\"}";
        Serial.printf("[WebConfig] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        json = "{\"success\":false,\"error\":\"Connection timeout\"}";
        Serial.println("[WebConfig] Connection failed");
    }
    
    _server.send(200, "application/json", json);
}

void WebConfig::handleConfig() {
    _server.send(200, "text/html", getConfigPage());
}

void WebConfig::handleSaveConfig() {
    // Get form values
    if (_server.hasArg("calib_a")) _calibA = _server.arg("calib_a").toFloat();
    if (_server.hasArg("calib_b")) _calibB = _server.arg("calib_b").toFloat();
    if (_server.hasArg("calib_c")) _calibC = _server.arg("calib_c").toFloat();
    if (_server.hasArg("influx_url")) _influxURL = _server.arg("influx_url");
    if (_server.hasArg("influx_org")) _influxOrg = _server.arg("influx_org");
    if (_server.hasArg("influx_bucket")) _influxBucket = _server.arg("influx_bucket");
    if (_server.hasArg("influx_token")) _influxToken = _server.arg("influx_token");
    if (_server.hasArg("device_id")) _deviceID = _server.arg("device_id");
    
    saveConfig();
    
    _server.send(200, "application/json", "{\"success\":true}");
}

// External reference to lastPowerData from main.cpp
extern PowerData lastPowerData;

void WebConfig::handleStatus() {
    // Return actual measurement data from PowerAnalyzer
    String json = "{";
    json += "\"wifi_connected\":" + String(isConnected() ? "true" : "false") + ",";
    json += "\"ip\":\"" + getIPAddress() + "\",";
    json += "\"uptime\":" + String(millis() / 1000) + ",";
    json += "\"phases\":[";
    json += "{\"voltage\":" + String(lastPowerData.voltageA, 1) + ",\"status\":\"ok\"},";
    json += "{\"voltage\":" + String(lastPowerData.voltageB, 1) + ",\"status\":\"ok\"},";
    json += "{\"voltage\":" + String(lastPowerData.voltageC, 1) + ",\"status\":\"ok\"}";
    json += "],";
    json += "\"frequency\":" + String(lastPowerData.frequencyAvg, 2) + ",";
    json += "\"unbalance\":" + String(lastPowerData.unbalance, 2);
    json += "}";
    
    _server.send(200, "application/json", json);
}

void WebConfig::handleReboot() {
    _server.send(200, "application/json", "{\"success\":true}");
    delay(500);
    ESP.restart();
}

void WebConfig::handleNotFound() {
    // Captive Portal: redirect all requests to the main page
    if (_apMode) {
        // Check for captive portal detection URLs and redirect
        String host = _server.hostHeader();
        
        // Android captive portal detection
        if (_server.uri() == "/generate_204" || 
            _server.uri() == "/gen_204" ||
            _server.uri() == "/connecttest.txt") {
            _server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
            _server.send(302, "text/plain", "");
            return;
        }
        
        // iOS/macOS captive portal detection
        if (_server.uri() == "/hotspot-detect.html" ||
            _server.uri() == "/library/test/success.html") {
            _server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
            _server.send(302, "text/plain", "");
            return;
        }
        
        // Windows captive portal detection
        if (_server.uri() == "/ncsi.txt" ||
            _server.uri() == "/connecttest.txt") {
            _server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
            _server.send(302, "text/plain", "");
            return;
        }
        
        // Redirect any other request to main page
        _server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
        _server.send(302, "text/plain", "");
        return;
    }
    
    _server.send(404, "text/plain", "Not found");
}
