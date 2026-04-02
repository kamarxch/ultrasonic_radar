#include <Arduino.h>
#include <ESP32Servo.h>
#include <WebServer.h>

// =====================
// CHANGE THESE
#include "secrets.h"
const char* ssid     = WIFI_SSID;
const char* password = WIFI_PASSWORD;

#define SERVO_PIN    12
#define TRIG_PIN     13
#define ECHO_PIN     14
#define MAX_DISTANCE 200

Servo radar_servo;
WebServer server(80);

int  currentAngle    = 0;
int  currentDistance = 0;
bool sweepForward    = true;

long getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return MAX_DISTANCE;
  long cm = duration * 0.034 / 2;
  return cm > MAX_DISTANCE ? MAX_DISTANCE : cm;
}

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP-CAM Radar</title>
<style>
  * { margin:0; padding:0; box-sizing:border-box; }
  body {
    background:#0a0f0a; color:#00ff41;
    font-family:'Courier New',monospace;
    display:flex; flex-direction:column; align-items:center;
    min-height:100vh; padding:20px;
  }
  h1 {
    font-size:1.2rem; letter-spacing:4px; margin-bottom:4px;
    color:#00ff41; text-shadow:0 0 8px #00ff41;
  }
  .subtitle { font-size:0.7rem; color:#006614; letter-spacing:2px; margin-bottom:16px; }
  #radar { display:block; }
  .cards {
    display:flex; gap:16px; margin-top:16px;
    flex-wrap:wrap; justify-content:center;
  }
  .card {
    background:#050f05; border:1px solid #003a00;
    border-radius:8px; padding:12px 24px;
    text-align:center; min-width:110px;
  }
  .card .label { font-size:0.6rem; color:#006614; letter-spacing:2px; margin-bottom:4px; }
  .card .value { font-size:1.8rem; font-weight:bold; color:#00ff41; text-shadow:0 0 6px #00ff41; }
  .card .unit  { font-size:0.65rem; color:#006614; }
  #alert {
    margin-top:14px; padding:10px 32px; border-radius:6px;
    font-size:0.85rem; letter-spacing:3px; font-weight:bold;
    transition:all 0.2s;
    background:#050f05; border:1px solid #003a00; color:#006614;
  }
  #alert.danger {
    background:#1a0000; border-color:#ff2200;
    color:#ff2200; text-shadow:0 0 8px #ff2200;
  }
  #status { margin-top:8px; font-size:0.6rem; color:#004400; letter-spacing:1px; }
</style>
</head>
<body>

<h1>ESP-CAM RADAR</h1>
<p class="subtitle">ULTRASONIC SWEEP SYSTEM</p>

<canvas id="radar"></canvas>

<div class="cards">
  <div class="card">
    <div class="label">DISTANCE</div>
    <div class="value" id="dist">--</div>
    <div class="unit">cm</div>
  </div>
  <div class="card">
    <div class="label">ANGLE</div>
    <div class="value" id="angle">--</div>
    <div class="unit">deg</div>
  </div>
  <div class="card">
    <div class="label">MAX RANGE</div>
    <div class="value">200</div>
    <div class="unit">cm</div>
  </div>
</div>

<div id="alert">NO OBJECT DETECTED</div>
<div id="status">CONNECTING...</div>

<script>
const DETECT_THRESHOLD = 80;
const MAX_DIST = 200;

const canvas = document.getElementById('radar');
const ctx    = canvas.getContext('2d');

// Responsive size
const W = Math.min(window.innerWidth - 40, 500);
const H = W / 2 + 20; // half circle + small bottom margin
canvas.width  = W;
canvas.height = H;

const cx = W / 2;
const cy = H - 20;   // origin sits at bottom center
const R  = W / 2 - 10;

const points = [];

function toRad(deg) { return deg * Math.PI / 180; }

// angle 0 = right, 180 = left (matches physical servo)
// on canvas: 0deg -> right (positive x), 180deg -> left (negative x)
// canvas angle from positive x axis going counter-clockwise for upper half
function angleToCanvas(deg) {
  // servo 0=right, 90=center-top, 180=left
  // canvas: right = 0rad, top = -PI/2, left = PI
  return toRad(deg); // maps 0->PI (left), 90->PI/2 (top), 180->0 (right)
}

function drawRadar(angle, distance) {
  ctx.clearRect(0, 0, W, H);

  // Black semicircle background
  ctx.fillStyle = '#050f05';
  ctx.beginPath();
  ctx.arc(cx, cy, R, Math.PI, 0, false);
  ctx.lineTo(cx + R, cy);
  ctx.lineTo(cx - R, cy);
  ctx.closePath();
  ctx.fill();

  // Range rings (semicircles)
  [0.25, 0.5, 0.75, 1].forEach(f => {
    ctx.beginPath();
    ctx.arc(cx, cy, R * f, Math.PI, 0, false);
    ctx.strokeStyle = '#003a00';
    ctx.lineWidth = 0.8;
    ctx.stroke();
    // Label on right side of each ring
    ctx.fillStyle = '#004d00';
    ctx.font = '9px Courier New';
    ctx.fillText(Math.round(MAX_DIST * f) + 'cm', cx + R * f + 3, cy - 3);
  });

  // Angle lines every 30 degrees
  for (let a = 0; a <= 180; a += 30) {
    const rad = angleToCanvas(a);
    ctx.beginPath();
    ctx.moveTo(cx, cy);
    ctx.lineTo(cx + R * Math.cos(rad), cy - R * Math.sin(rad));
    ctx.strokeStyle = '#003a00';
    ctx.lineWidth = 0.8;
    ctx.stroke();
    // Angle label
    const lx = cx + (R + 14) * Math.cos(rad);
    const ly = cy - (R + 14) * Math.sin(rad);
    ctx.fillStyle = '#005500';
    ctx.font = '9px Courier New';
    ctx.textAlign = 'center';
    ctx.fillText(a + '°', lx, ly);
  }
  ctx.textAlign = 'left';

  // Sweep trail
  const trailLen = 30;
  for (let i = 1; i <= trailLen; i++) {
    const tAngle = sweepForward ? angle - i : angle + i;
    if (tAngle < 0 || tAngle > 180) continue;
    const tRad = angleToCanvas(tAngle);
    ctx.beginPath();
    ctx.moveTo(cx, cy);
    ctx.lineTo(cx + R * Math.cos(tRad), cy - R * Math.sin(tRad));
    ctx.strokeStyle = `rgba(0,255,65,${(trailLen - i) / trailLen * 0.2})`;
    ctx.lineWidth = 2;
    ctx.stroke();
  }

  // Sweep line
  const sweepRad = angleToCanvas(angle);
  ctx.beginPath();
  ctx.moveTo(cx, cy);
  ctx.lineTo(cx + R * Math.cos(sweepRad), cy - R * Math.sin(sweepRad));
  ctx.strokeStyle = '#00ff41';
  ctx.lineWidth = 2;
  ctx.stroke();

  // Detection dots (fade over 3s)
  const now = Date.now();
  for (let i = points.length - 1; i >= 0; i--) {
    const p = points[i];
    const age = (now - p.time) / 3000;
    if (age > 1) { points.splice(i, 1); continue; }
    const pr  = angleToCanvas(p.angle);
    const pr2 = (p.dist / MAX_DIST) * R;
    const px  = cx + pr2 * Math.cos(pr);
    const py  = cy - pr2 * Math.sin(pr);
    ctx.beginPath();
    ctx.arc(px, py, 4, 0, 2 * Math.PI);
    ctx.fillStyle = `rgba(0,255,65,${1 - age})`;
    ctx.fill();
  }

  // Flat bottom line
  ctx.beginPath();
  ctx.moveTo(cx - R, cy);
  ctx.lineTo(cx + R, cy);
  ctx.strokeStyle = '#00aa22';
  ctx.lineWidth = 1;
  ctx.stroke();

  // Semicircle border
  ctx.beginPath();
  ctx.arc(cx, cy, R, Math.PI, 0, false);
  ctx.strokeStyle = '#00aa22';
  ctx.lineWidth = 1.5;
  ctx.stroke();

  // Center dot
  ctx.beginPath();
  ctx.arc(cx, cy, 3, 0, 2 * Math.PI);
  ctx.fillStyle = '#00ff41';
  ctx.fill();
}

let sweepForward = true;
let lastAngle = 0;

function poll() {
  fetch('/data')
    .then(r => r.json())
    .then(d => {
      const angle = d.angle;
      const dist  = d.distance;

      // Detect sweep direction for trail
      sweepForward = angle >= lastAngle;
      lastAngle = angle;

      drawRadar(angle, dist);

      if (dist < MAX_DIST) {
        points.push({ angle, dist, time: Date.now() });
      }

      document.getElementById('dist').textContent  = dist;
      document.getElementById('angle').textContent = angle;

      const alertEl = document.getElementById('alert');
      if (dist < DETECT_THRESHOLD) {
        alertEl.textContent = 'OBJECT DETECTED — ' + dist + ' cm';
        alertEl.className = 'danger';
      } else {
        alertEl.textContent = 'NO OBJECT DETECTED';
        alertEl.className = '';
      }

      document.getElementById('status').textContent = 'LIVE';
    })
    .catch(() => {
      document.getElementById('status').textContent = 'CONNECTION LOST — retrying...';
    });
}

// Fast poll — every 50ms
setInterval(poll, 50);
poll();
</script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", INDEX_HTML);
}

void handleData() {
  String json = "{\"angle\":" + String(currentAngle) +
                ",\"distance\":" + String(currentDistance) + "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  radar_servo.attach(SERVO_PIN);
  radar_servo.write(0);
  delay(500);

  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected! Open this in your browser:");
  Serial.print("http://");
  Serial.println(WiFi.localIP());

  server.on("/",     handleRoot);
  server.on("/data", handleData);
  server.begin();
}

void loop() {
  server.handleClient();

  if (sweepForward) {
    currentAngle++;
    if (currentAngle >= 180) sweepForward = false;
  } else {
    currentAngle--;
    if (currentAngle <= 0) sweepForward = true;
  }

  radar_servo.write(currentAngle);
  delay(15);  // slightly faster sweep
  currentDistance = getDistance();
}