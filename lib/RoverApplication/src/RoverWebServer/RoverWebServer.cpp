#include "RoverWebServer.h"
#include "Log/RemoteLogger.h"
#include <Update.h>

RoverWebServer::RoverWebServer(const RoverApplicationConfig& config, CommandDispatcher& dispatcher)
  : config(config), dispatcher(dispatcher), server(config.webServerPort)
{}

bool RoverWebServer::authorized() {
  // Auth disabled when both fields empty.
  if (!config.adminUser || !config.adminPassword || config.adminUser[0] == '\0') {
    return true;
  }
  if (server.authenticate(config.adminUser, config.adminPassword)) {
    return true;
  }
  server.requestAuthentication();
  return false;
}

void RoverWebServer::begin() {
  if (running) return;
  // Unified JSON command surface — same shape as WS / BLE / MQTT.
  // Body: a JSON object with a "command" field. Reply: the dispatcher's
  // first respond() call (every command currently emits exactly one).
  server.on("/api/cmd", HTTP_POST, [this]() { handleApiCommand(); });
  // OTA upload utility (firmware page).
  server.on("/ota", HTTP_GET, [this]() { handleOtaPage(); });
  server.on("/ota/upload", HTTP_POST,
    [this]() { handleOtaUploadFinish(); },
    [this]() { handleOtaUpload(); }
  );
  // Log viewer utility.
  server.on("/logs", HTTP_GET, [this]() { handleLogsPage(); });
  server.on("/logs/data", HTTP_GET, [this]() { handleLogsData(); });
  server.begin(config.webServerPort);
  running = true;
  Log.println("[http] started");
}

void RoverWebServer::stop() {
  if (!running) return;
  server.close();
  running = false;
  Log.println("[http] stopped");
}

void RoverWebServer::handleClient() {
  if (running) server.handleClient();
}

void RoverWebServer::handleApiCommand() {
  if (!authorized()) return;
  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "Body not found");
    return;
  }
  std::string raw = server.arg("plain").c_str();
  std::string captured;
  bool got = false;
  dispatcher.dispatchRaw(raw, [&](const std::string& reply) {
    if (!got) { captured = reply; got = true; }
    // Subsequent respond() calls within the same dispatch are dropped — HTTP
    // can only emit one response. Today every handler emits exactly one.
  });
  if (got) {
    server.send(200, "application/json", captured.c_str());
  } else {
    server.send(400, "application/json", "{\"error\":\"unknown_command\"}");
  }
}

static const char OTA_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Rover OTA</title>
<style>
:root{--bg:#0b1020;--card:#1a2240;--border:#2a345a;--text:#e7ebf5;--dim:#8a93b3;--accent:#5b8cff;}
*{box-sizing:border-box}body{margin:0;font-family:-apple-system,Segoe UI,Roboto,sans-serif;color:var(--text);background:var(--bg);min-height:100vh;display:flex;align-items:center;justify-content:center;padding:20px}
.card{background:var(--card);border:1px solid var(--border);border-radius:14px;padding:22px;max-width:420px;width:100%;box-shadow:0 12px 40px rgba(0,0,0,.4)}
h1{margin:0 0 6px;font-size:20px}p{margin:0 0 18px;color:var(--dim);font-size:13px}
input[type=file]{width:100%;padding:10px;background:#131a30;border:1px dashed var(--border);border-radius:10px;color:var(--text);margin-bottom:12px;font-size:13px}
button{width:100%;padding:12px;border:none;border-radius:10px;background:linear-gradient(135deg,var(--accent),#8a5bff);color:#fff;font-weight:600;cursor:pointer;font-size:14px}
button:disabled{opacity:.5;cursor:not-allowed}
.bar{margin-top:14px;height:8px;background:#131a30;border-radius:999px;overflow:hidden;display:none}
.bar.show{display:block}.fill{height:100%;width:0;background:linear-gradient(90deg,var(--accent),#8a5bff);transition:width .15s}
.msg{margin-top:12px;font-size:13px;text-align:center}.ok{color:#3ddc84}.err{color:#ff6b6b}
</style></head><body>
<div class="card">
<h1>Firmware Update</h1>
<p>Upload a compiled <code>firmware.bin</code> to flash the rover over the air.</p>
<form id="f"><input type="file" id="file" accept=".bin" required>
<button id="b" type="submit">Upload &amp; Flash</button>
<div class="bar" id="bar"><div class="fill" id="fill"></div></div>
<div class="msg" id="msg"></div></form>
</div>
<script>
const f=document.getElementById('f'),file=document.getElementById('file'),b=document.getElementById('b'),
bar=document.getElementById('bar'),fill=document.getElementById('fill'),msg=document.getElementById('msg');
f.addEventListener('submit',e=>{e.preventDefault();if(!file.files[0])return;
const fd=new FormData();fd.append('update',file.files[0]);
const xhr=new XMLHttpRequest();xhr.open('POST','/ota/upload');
xhr.upload.onprogress=ev=>{if(ev.lengthComputable){bar.classList.add('show');
fill.style.width=(ev.loaded/ev.total*100)+'%';msg.textContent=Math.round(ev.loaded/ev.total*100)+'%';}};
xhr.onload=()=>{if(xhr.status===200){msg.className='msg ok';msg.textContent='Upload complete. Rebooting...';}
else{msg.className='msg err';msg.textContent='Failed: '+xhr.responseText;b.disabled=false;}};
xhr.onerror=()=>{msg.className='msg err';msg.textContent='Network error';b.disabled=false;};
b.disabled=true;msg.className='msg';msg.textContent='Uploading...';xhr.send(fd);});
</script></body></html>
)rawliteral";

void RoverWebServer::handleOtaPage() {
  if (!authorized()) return;
  server.send_P(200, "text/html", OTA_PAGE);
}

void RoverWebServer::handleOtaUpload() {
  if (!authorized()) return;
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    Log.printf("OTA: receiving %s\n", upload.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Log.printf("OTA: %u bytes written\n", upload.totalSize);
    } else {
      Update.printError(Serial);
    }
  }
}

void RoverWebServer::handleOtaUploadFinish() {
  if (Update.hasError()) {
    server.send(500, "text/plain", "Update failed");
  } else {
    server.send(200, "text/plain", "OK, rebooting");
    delay(500);
    ESP.restart();
  }
}

static const char LOGS_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Rover Logs</title>
<style>
:root{--bg:#0b1020;--card:#131a30;--border:#2a345a;--text:#e7ebf5;--dim:#8a93b3;--accent:#5b8cff;}
*{box-sizing:border-box}html,body{height:100%}
body{margin:0;font-family:-apple-system,Segoe UI,Roboto,sans-serif;color:var(--text);background:var(--bg);display:flex;flex-direction:column}
header{display:flex;align-items:center;gap:10px;padding:10px 14px;border-bottom:1px solid var(--border);background:var(--card)}
h1{margin:0;font-size:15px;font-weight:650;flex:1}
button,a.btn{background:var(--card);border:1px solid var(--border);color:var(--text);padding:6px 12px;border-radius:8px;font-size:12px;cursor:pointer;text-decoration:none}
button:hover,a.btn:hover{border-color:var(--accent)}
button.active{background:var(--accent);color:#fff;border-color:var(--accent)}
.dot{width:7px;height:7px;border-radius:50%;background:var(--dim);display:inline-block;margin-right:5px}
.dot.live{background:#3ddc84;box-shadow:0 0 6px #3ddc84;animation:pulse 1.5s ease-in-out infinite}
@keyframes pulse{50%{opacity:.4}}
pre{flex:1;margin:0;padding:14px;overflow:auto;font:12px/1.45 ui-monospace,Menlo,Consolas,monospace;color:var(--text);background:var(--bg);white-space:pre-wrap;word-break:break-word}
pre::-webkit-scrollbar{width:8px}pre::-webkit-scrollbar-thumb{background:var(--border);border-radius:4px}
.bad{color:#ff6b6b}.warn{color:#ffb454}.ok{color:#3ddc84}
</style></head><body>
<header>
<h1>Rover Logs</h1>
<span><span class="dot" id="dot"></span><span id="status">connecting</span></span>
<button id="pauseBtn">Pause</button>
<button id="clearBtn">Clear</button>
</header>
<pre id="log"></pre>
<script>
const log=document.getElementById('log'),dot=document.getElementById('dot'),
  st=document.getElementById('status'),pauseBtn=document.getElementById('pauseBtn'),
  clearBtn=document.getElementById('clearBtn');
let cursor=0,paused=false,timer=null,lastOk=0;
function setLive(on){dot.classList.toggle('live',on);st.textContent=on?'live':'offline';}
function colorize(line){if(/E \(|error|fail/i.test(line))return '<span class="bad">'+line+'</span>';
  if(/W \(|warn/i.test(line))return '<span class="warn">'+line+'</span>';
  if(/OTA: complete|connected|ready/i.test(line))return '<span class="ok">'+line+'</span>';
  return line;}
async function poll(){if(paused)return;
  try{const r=await fetch('/logs/data?since='+cursor);
    if(!r.ok)throw 0;
    const j=await r.json();cursor=j.cursor;setLive(true);lastOk=Date.now();
    if(j.data){const atBottom=log.scrollTop+log.clientHeight>=log.scrollHeight-30;
      const lines=j.data.split(/\r?\n/);
      let html='';for(const l of lines)if(l)html+=colorize(l.replace(/&/g,'&amp;').replace(/</g,'&lt;'))+'\n';
      log.insertAdjacentHTML('beforeend',html);
      if(log.textContent.length>200000)log.textContent=log.textContent.slice(-150000);
      if(atBottom)log.scrollTop=log.scrollHeight;}}
  catch(e){if(Date.now()-lastOk>3000)setLive(false);}}
pauseBtn.addEventListener('click',()=>{paused=!paused;pauseBtn.textContent=paused?'Resume':'Pause';pauseBtn.classList.toggle('active',paused);});
clearBtn.addEventListener('click',()=>log.textContent='');
timer=setInterval(poll,500);poll();
</script></body></html>
)rawliteral";

void RoverWebServer::handleLogsPage() {
  if (!authorized()) return;
  server.send_P(200, "text/html", LOGS_PAGE);
}

void RoverWebServer::handleLogsData() {
  if (!authorized()) return;
  size_t cursor = 0;
  if (server.hasArg("since")) cursor = strtoul(server.arg("since").c_str(), nullptr, 10);
  String data = Log.readSince(cursor);
  // Build a minimal JSON manually (no nlohmann here — tiny payload).
  String out;
  out.reserve(data.length() + 64);
  out += "{\"cursor\":";
  out += String((unsigned long)Log.totalWritten());
  out += ",\"data\":\"";
  // JSON-escape backslashes, quotes, control chars
  for (size_t i = 0; i < data.length(); ++i) {
    char c = data[i];
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"':  out += "\\\""; break;
      case '\n': out += "\\n";  break;
      case '\r': out += "\\r";  break;
      case '\t': out += "\\t";  break;
      default:
        if ((uint8_t)c < 0x20) {
          char buf[8]; snprintf(buf, sizeof(buf), "\\u%04x", c);
          out += buf;
        } else {
          out += c;
        }
    }
  }
  out += "\"}";
  server.send(200, "application/json", out);
}
