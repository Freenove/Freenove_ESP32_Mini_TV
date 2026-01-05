/*
* @ File:   Sketch_06.3_TFT_Clock.ino
* @ Author: [Vegetable-SYC]
* @ Date:   [2025-12-24]
*/

#include <TFT_eSPI.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include <TimeLib.h>
#include <DNSServer.h>

/* -------------------------------------------------------------------------- */
/*                               Configuration                                */
/* -------------------------------------------------------------------------- */

// Pin Definitions
#define TFT_VCC       21         
#define TOUCH_PIN     32         
#define EEPROM_SIZE   1024       
#define AP_SSID       "ESP32-Clock"
#define AP_IP         IPAddress(192, 168, 4, 1)

// UI Geometry
#define CENTER_X      120
#define CENTER_Y      120
#define RADIUS        100

// Color Palette (16-bit RGB565)
#define COLOR_BG      0x0000 
#define COLOR_SCALE   0xFFFF 
#define COLOR_TEXT    0xFFFF 
#define COLOR_HOUR    0xF800 
#define COLOR_MIN     0x07E0 
#define COLOR_SEC     0xFFE0 
#define COLOR_CENTER  0xFFFF 
#define COLOR_RING    0xAD55 

// Global Constants
const char* ntpServer     = "pool.ntp.org";
const int timeZone        = 8; 
const char* AUTHOR_NAME   = "Freenove";
const int TOUCH_THRESHOLD = 80;
const int PRESS_TIME      = 3000;

// Object Instances
TFT_eSPI tft = TFT_eSPI();
WiFiServer server(80);
DNSServer dnsServer;
WiFiUDP ntpUDP;

// State Variables
bool timeSynced     = false;     
bool isAPMode       = false;     
int currentUI       = 1;         
int lastUI          = -1;        
bool isFirstDraw    = true;      
int wifiScanCount   = 0;         
float lastHAngle=0, lastMAngle=0, lastSAngle=0; 

// Touch Interaction Variables
uint32_t Touch_Time = 0;
int Touch_data = 0, Touch_old_data = 0; 
bool LONG_PRESS_FLAG = false;

// HTML/CSS for Web Configuration Portal
const char* CSS_BODY = R"raw(
<style>
    body {
        background: #202020;
        color: #fff;
        font-family: sans-serif;
        margin: 0;
        padding: 0;
    }
    #header {
        background: #333;
        padding: 15px;
        text-align: center;
    }
    h1 {
        margin: 0;
        font-size: 20px;
    }
    .list {
        max-height: 400px;
        overflow-y: auto;
    }
    .item {
        padding: 15px;
        border-bottom: 1px solid #444;
        display: flex;
        justify-content: space-between;
        cursor: pointer;
    }
    .item:active {
        background: #444;
    }
    .ssid {
        font-size: 16px;
        font-weight: bold;
    }
    .meta {
        font-size: 12px;
        color: #aaa;
    }
    .form {
        padding: 20px;
        background: #303030;
        text-align: center;
    }
    input {
        width: 100%;
        padding: 12px;
        margin: 10px 0;
        border: none;
        border-radius: 4px;
        font-size: 16px !important;
        box-sizing: border-box;
    }
    button {
        width: 100%;
        padding: 15px;
        background: #007BFF;
        color: #fff;
        border: none;
        border-radius: 4px;
        font-size: 18px;
        font-weight: bold;
        cursor: pointer;
    }
    a {
        color: #007BFF;
        text-decoration: none;
        display: block;
        margin-top: 10px;
        text-align: center;
    }
</style>

<script>
    function c(s) {
        document.getElementById('s').value = s;
        document.getElementById('p').focus();
    }
</script>
)raw";

/* -------------------------------------------------------------------------- */
/*                                Main Program                                */
/* -------------------------------------------------------------------------- */

void setup() {
  Serial.begin(115200);

  // Initialize NVS storage
  if (!EEPROM.begin(EEPROM_SIZE)) Serial.println("EEPROM Init Failed");

  // Initialize Display
  pinMode(TFT_VCC, OUTPUT);
  digitalWrite(TFT_VCC, LOW); 
  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(COLOR_BG);

  // Initialize Touch and calibrate baseline
  touchSetCycles(0xf000, 0x1000);
  Touch_old_data = touchRead(TOUCH_PIN);

  // Connection management
  connectToWiFi();
  ntpUDP.begin(123);
}

void loop() {
  if (isAPMode) {
    dnsServer.processNextRequest(); // Handle DNS queries for Captive Portal
    handleWebServer();             // Process config portal requests
  } else if (WiFi.isConnected()) {
    static unsigned long lastNtpUpdate = 0;
    // Periodic NTP synchronization
    if (millis() - lastNtpUpdate > 3600000 || !timeSynced) {
      syncNtpTime(timeZone);
      lastNtpUpdate = millis();
    }
  }

  checkTouchInput();

  // Rendering logic only active after time synchronization
  if (timeSynced) renderUI();

  delay(5);
}

// Logic for switching and refreshing different UI modes
void renderUI() {
  if (currentUI != lastUI) {
    tft.fillScreen(COLOR_BG);
    isFirstDraw = true;
    lastUI = currentUI;
  }

  switch (currentUI) {
    case 0: // UI Mode: Analog Clock
    {
      static int lastS = -1;
      if (second() != lastS) { 
        drawAnalogClock(); 
        lastS = second(); 
      }
      break;
    }
    case 1: // UI Mode: Digital Clock
    {
      static int lastM = -1;
      if (minute() != lastM || isFirstDraw) { 
        drawDigitalClock(); 
        lastM = minute(); 
        isFirstDraw = false;
      }
      break;
    }
    case 2: // UI Mode: System Info
    {
      static unsigned long lastUpdate = 0;
      if (millis() - lastUpdate > 2000 || isFirstDraw) { 
        drawInfoScreen((char*)AUTHOR_NAME);
        lastUpdate = millis();
        isFirstDraw = false;
      }
      break;
    }
  }
}

// Logic to draw an Analog Clock with pointer rotation
void drawAnalogClock() {
  if (isFirstDraw) {
    drawStaticFace();
    isFirstDraw = false;
  } else {
    eraseOldHands(); 
  }

  // Calculate hand angles in radians
  float ha = (hour()%12*30 + minute()*0.5) * DEG_TO_RAD;
  float ma = minute() * 6 * DEG_TO_RAD;
  float sa = second() * 6 * DEG_TO_RAD;

  // Render Hour hand
  int hx = CENTER_X + 60*sin(ha), hy = CENTER_Y - 60*cos(ha);
  tft.drawLine(CENTER_X, CENTER_Y, hx, hy, COLOR_HOUR);
  tft.drawLine(CENTER_X+1, CENTER_Y, hx+1, hy, COLOR_HOUR);

  // Render Minute hand
  int mx = CENTER_X + 80*sin(ma), my = CENTER_Y - 80*cos(ma);
  tft.drawLine(CENTER_X, CENTER_Y, mx, my, COLOR_MIN);

  // Render Second hand with vector arrow tip
  int sx = CENTER_X + 90*sin(sa), sy = CENTER_Y - 90*cos(sa);
  tft.drawLine(CENTER_X, CENTER_Y, sx, sy, COLOR_SEC);
  drawVectorArrow(sx, sy, sa, 5, COLOR_SEC);

  // Central cap
  tft.fillCircle(CENTER_X, CENTER_Y, 6, COLOR_CENTER);

  // Render digital time string at bottom of analog face
  tft.fillRect(CENTER_X - 60, 150, 120, 30, COLOR_BG);
  char ts[9]; 
  sprintf(ts, "%02d:%02d:%02d", hour(), minute(), second());
  tft.setTextColor(COLOR_TEXT, COLOR_BG);
  tft.setTextDatum(TC_DATUM); 
  tft.drawString(ts, CENTER_X, 150, 4);

  lastHAngle = ha; lastMAngle = ma; lastSAngle = sa;
}

// Logic to draw large Digital Clock
void drawDigitalClock() {
  tft.setTextDatum(MC_DATUM);
  tft.fillRect(0, 30, 240, 200, COLOR_BG);

  // Render Date
  char dateStr[20];
  sprintf(dateStr, "%04d-%02d-%02d", year(), month(), day());
  tft.setTextColor(TFT_SILVER, COLOR_BG);
  tft.drawString(dateStr, 120, 45, 4);

  // Render HH:MM
  char timeStr[10];
  sprintf(timeStr, "%02d:%02d", hour(), minute());
  tft.setTextColor(TFT_WHITE, COLOR_BG);
  tft.drawString(timeStr, 120, 120, 7); 

  // Render Weekday
  const char* wds[] = {"", "SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY", "THURSDAY", "FRIDAY", "SATURDAY"};
  tft.setTextColor(TFT_YELLOW, COLOR_BG);
  tft.drawString(wds[weekday()], 120, 200, 4);
}

// Logic to display System and Network statistics
void drawInfoScreen(char* AUTHOR) {
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_CYAN, COLOR_BG);
  tft.drawString("SYSTEM INFO", 120, 30, 4);
  tft.drawLine(20, 50, 220, 50, TFT_CYAN);

  // Display SSID
  tft.setTextColor(TFT_SILVER, COLOR_BG);
  tft.drawString("Network", 120, 70, 2);
  tft.setTextColor(TFT_WHITE, COLOR_BG);
  String ssid = WiFi.SSID();
  if (ssid.length() > 14) ssid = ssid.substring(0, 14) + "..";
  tft.drawString(ssid, 120, 95, 4);

  // RSSI Data
  tft.setTextDatum(MR_DATUM);
  tft.setTextColor(TFT_SILVER, COLOR_BG); tft.drawString("Signal ", 110, 130, 2);
  tft.setTextColor(TFT_GREEN, COLOR_BG);  tft.drawString(String(WiFi.RSSI()) + "dBm ", 110, 150, 2);

  // Local IP Data
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(TFT_SILVER, COLOR_BG); tft.drawString(" Local IP", 130, 130, 2);
  tft.setTextColor(TFT_YELLOW, COLOR_BG); tft.drawString(WiFi.localIP().toString(), 130, 150, 2);

  // Attribution
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_ORANGE, COLOR_BG); tft.drawString("Author", 120, 190, 2);
  tft.setTextColor(TFT_MAGENTA, COLOR_BG); tft.drawString(AUTHOR, 120, 215, 4);
}

// Draw outer dial and scale marks
void drawStaticFace() {
  tft.drawCircle(CENTER_X, CENTER_Y, RADIUS, COLOR_RING);
  for(int i=0; i<60; i++) {
    float a = i * 6 * DEG_TO_RAD;
    int len = (i%5==0) ? 15 : 8;
    tft.drawLine(
      CENTER_X + (RADIUS-len) * sin(a), CENTER_Y - (RADIUS-len) * cos(a),
      CENTER_X + RADIUS * sin(a),       CENTER_Y - RADIUS * cos(a), 
      COLOR_SCALE
    );
  }
}

// Clean previous hand positions to avoid artifacts
void eraseOldHands() {
  int hx = CENTER_X + 60*sin(lastHAngle), hy = CENTER_Y - 60*cos(lastHAngle);
  tft.drawLine(CENTER_X, CENTER_Y, hx, hy, COLOR_BG);
  tft.drawLine(CENTER_X+1, CENTER_Y, hx+1, hy, COLOR_BG);

  int mx = CENTER_X + 80*sin(lastMAngle), my = CENTER_Y - 80*cos(lastMAngle);
  tft.drawLine(CENTER_X, CENTER_Y, mx, my, COLOR_BG);

  int sx = CENTER_X + 90*sin(lastSAngle), sy = CENTER_Y - 90*cos(lastSAngle);
  tft.drawLine(CENTER_X, CENTER_Y, sx, sy, COLOR_BG);
  eraseVectorArrow(sx, sy, lastSAngle, 5);
}

// Vector math for the second hand pointer
void drawVectorArrow(int x, int y, float a, int s, uint16_t c) {
  tft.drawLine(x, y, x + s*sin(a + 2.6), y - s*cos(a + 2.6), c);
  tft.drawLine(x, y, x + s*sin(a - 2.6), y - s*cos(a - 2.6), c);
}

void eraseVectorArrow(int x, int y, float a, int s) {
  tft.drawLine(x, y, x + s*sin(a + 2.6), y - s*cos(a + 2.6), COLOR_BG);
  tft.drawLine(x, y, x + s*sin(a - 2.6), y - s*cos(a - 2.6), COLOR_BG);
}

// Handles Touch interactions (UI toggle or System Reset)
void checkTouchInput() {
  Touch_data = touchRead(TOUCH_PIN);
  if(Touch_old_data < Touch_data) Touch_old_data = Touch_data;

  if((Touch_old_data - Touch_data) > TOUCH_THRESHOLD) {
    if(Touch_Time == 0) Touch_Time = millis();

    // Check for Long Press (Reset WiFi)
    if(millis() - Touch_Time > PRESS_TIME) {
      Touch_Time = 0;
      LONG_PRESS_FLAG = true;
      clearWiFiCredentials();
      delay(200);
      ESP.restart();
    }
  } else {
    // Process Short Press (Change UI Mode)
    if(Touch_Time != 0) {
      if(!LONG_PRESS_FLAG) {
        currentUI++; 
        if (currentUI > 2) currentUI = 0;
      }
      Touch_Time = 0;
      LONG_PRESS_FLAG = false;
    }
  }
}

// UDP sync with NTP server to fetch UTC epoch time
void syncNtpTime(int tz) {
  IPAddress ip;
  if (!WiFi.hostByName(ntpServer, ip)) return;
  
  byte packet[48] = {0xE3, 0}; 
  ntpUDP.beginPacket(ip, 123);
  ntpUDP.write(packet, 48);
  ntpUDP.endPacket();
  
  delay(500);
  if (ntpUDP.parsePacket()) {
    ntpUDP.read(packet, 48);
    unsigned long high = word(packet[40], packet[41]);
    unsigned long low  = word(packet[42], packet[43]);
    unsigned long secs = high << 16 | low;
    setTime(secs - 2208988800UL + tz * 3600); // 70-year epoch conversion
    timeSynced = true;
    tft.fillScreen(COLOR_BG);
  }
}

// Logic to load saved credentials and connect to AP
void connectToWiFi() {
  char s[128] = {0}, p[128] = {0}; 
  for(int i=0; i<128; i++) s[i] = EEPROM.read(i);
  for(int i=0; i<128; i++) p[i] = EEPROM.read(256+i);

  if (strlen(s) > 0) {
    tft.setTextColor(TFT_WHITE, COLOR_BG); tft.setTextDatum(MC_DATUM);
    tft.drawString("Connecting WiFi...", 120, 100, 2);
    tft.drawString(s, 120, 130, 2);
    
    WiFi.begin(s, p);
    unsigned long st = millis();
    while (WiFi.status() != WL_CONNECTED) {
      if (millis() - st > 15000) { startAccessPoint(); return; }
      delay(500);
    }
    isAPMode = false;
  } else {
    startAccessPoint();
  }
}

// Create configuration Access Point with DNS redirection
void startAccessPoint() {
  isAPMode = true; 
  WiFi.mode(WIFI_AP_STA); 
  WiFi.softAPConfig(AP_IP, AP_IP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(AP_SSID);

  tft.fillScreen(0); 
  tft.setTextColor(TFT_ORANGE); tft.setTextDatum(MC_DATUM);
  tft.drawString("WiFi Config Mode", 120, 50, 4);
  tft.drawString("Scanning Networks...", 120, 100, 2);
  
  performWiFiScan();

  tft.fillScreen(0); 
  tft.setTextColor(TFT_GREEN);   tft.drawString("Server Ready", 120, 60, 4);
  tft.setTextColor(TFT_YELLOW);  tft.drawString("Connect to: " + String(AP_SSID), 120, 120, 2);
  tft.drawString("IP: 192.168.4.1", 120, 150, 4);

  dnsServer.start(53, "*", AP_IP);
  server.begin();
}

// Scan local environment for SSIDs
void performWiFiScan() {
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks(); 
  wifiScanCount = (n < 0) ? 0 : n;
}

// HTTP request handler for the configuration interface
void handleWebServer() {
  WiFiClient client = server.available();
  if (!client) return;

  String req = client.readStringUntil('\r');
  
  // Parse credentials sent via URL
  if (req.indexOf("GET /set") != -1) {
    int si = req.indexOf("s="), pi = req.indexOf("&p=");
    int ei = req.indexOf(" ", pi);
    if (si > 0 && pi > 0) {
      String s = urlDecode(req.substring(si+2, pi));
      String p = urlDecode(req.substring(pi+3, ei));
      saveCredentials(s.c_str(), p.c_str());
      client.println("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>Setting Saved! Restarting...</h1>");
      delay(1000); ESP.restart();
    }
  }
  
  // Manual trigger for WiFi rescan
  if (req.indexOf("GET /scan") != -1) {
    performWiFiScan();
    client.println("HTTP/1.1 302 Found\r\nLocation: /\r\n\r\n");
    return;
  }

  // Response with configuration HTML page
  client.println("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n");
  client.print("<html><head>");
  client.print(CSS_BODY);
  client.print("</head><body><div id='header'><h1>ESP32 WIFI SETUP</h1></div><div class='list'>");

  // Dynamic SSID list generation
  for (int i = 0; i < (wifiScanCount > 20 ? 20 : wifiScanCount); ++i) {
    String ssid = WiFi.SSID(i);
    int q = getSignalQuality(WiFi.RSSI(i));
    client.printf("<div class='item' onclick=\"c('%s')\"><div class='ssid'>%s</div><div class='meta'>%d%%</div></div>", ssid.c_str(), ssid.c_str(), q);
  }
  
  client.print("</div><div class='form'><form action='/set'><input id='s' name='s' placeholder='SSID'><input id='p' name='p' placeholder='PASS'><button type='submit'>SAVE</button></form></div></body></html>");
  client.stop();
}

// Save binary strings to EEPROM flash
void saveCredentials(const char* s, const char* p) {
  for(int i=0; i<512; i++) EEPROM.write(i, 0);
  for(int i=0; i<strlen(s); i++) EEPROM.write(i, s[i]);
  for(int i=0; i<strlen(p); i++) EEPROM.write(256+i, p[i]);
  EEPROM.commit();
}

// Clear EEPROM flash
void clearWiFiCredentials() {
  for (int i = 0; i < EEPROM_SIZE; i++) EEPROM.write(i, 0);
  EEPROM.commit();
}

// Parse HTTP percent-encoded strings
String urlDecode(String str) {
  String decoded = "";
  for (int i = 0; i < str.length(); i++) {
    if (str[i] == '+') decoded += ' ';
    else if (str[i] == '%') {
      int v = 0;
      sscanf(str.substring(i+1, i+3).c_str(), "%x", &v);
      decoded += char(v); i += 2;
    } else decoded += str[i];
  }
  return decoded;
}

// Map RSSI (dBm) to percentage value
int getSignalQuality(int rssi) {
  return (rssi <= -100) ? 0 : (rssi >= -50) ? 100 : 2 * (rssi + 100);
}