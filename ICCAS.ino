
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WebServer.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char* ssid = "SOMAIYA-WIFI";
const char* password = "d3F2f46J";
WebServer server(80);

const float SENSOR_DISTANCE_CM = 30.0; 

class Road {
  public:
    String name;
    uint8_t pinEntry;
    uint8_t pinExit;
    
    volatile unsigned long timeEntry = 0;
    volatile unsigned long timeExit = 0;
    volatile bool entryTriggered = false;
    volatile bool exitTriggered = false;

    int vehicleCount = 0;
    float lastSpeed = 0.0; // cm/s
    float avgSpeed = 0.0;
    float totalSpeed = 0.0;

    Road(String n, uint8_t pEn, uint8_t pEx) : name(n), pinEntry(pEn), pinExit(pEx) {}

    void processDetection() {
      if (entryTriggered && exitTriggered) {
        if (timeExit > timeEntry) {
          unsigned long timeDiff = timeExit - timeEntry; // in milliseconds
          float timeSeconds = timeDiff / 1000.0;
          
          lastSpeed = SENSOR_DISTANCE_CM / timeSeconds; // cm/s
          vehicleCount++;
          totalSpeed += lastSpeed;
          avgSpeed = totalSpeed / vehicleCount;

          Serial.printf("[%s] Vehicle Detected! Speed: %.2f cm/s | Count: %d\n", name.c_str(), lastSpeed, vehicleCount);
        }
        entryTriggered = false;
        exitTriggered = false;
      }
    }
};

Road roadN("North", 13, 12);
Road roadE("East", 14, 27);
Road roadS("South", 26, 25);
Road roadW("West", 33, 32);

void IRAM_ATTR isrN_Entry() { roadN.timeEntry = millis(); roadN.entryTriggered = true; }
void IRAM_ATTR isrN_Exit()  { roadN.timeExit = millis(); roadN.exitTriggered = true; }
void IRAM_ATTR isrE_Entry() { roadE.timeEntry = millis(); roadE.entryTriggered = true; }
void IRAM_ATTR isrE_Exit()  { roadE.timeExit = millis(); roadE.exitTriggered = true; }
void IRAM_ATTR isrS_Entry() { roadS.timeEntry = millis(); roadS.entryTriggered = true; }
void IRAM_ATTR isrS_Exit()  { roadS.timeExit = millis(); roadS.exitTriggered = true; }
void IRAM_ATTR isrW_Entry() { roadW.timeEntry = millis(); roadW.entryTriggered = true; }
void IRAM_ATTR isrW_Exit()  { roadW.timeExit = millis(); roadW.exitTriggered = true; }

void handleRoot() {
  String html = "<html><head><title>ICCAS Dashboard</title>";
  html += "<meta http-equiv='refresh' content='2'>"; // Auto-refresh every 2 seconds
  html += "<style>body{font-family: Arial; text-align: center;} table{margin: 0 auto; border-collapse: collapse; width: 80%;} th, td{border: 1px solid #ddd; padding: 8px;}</style></head><body>";
  html += "<h2>Intelligent Crossroad Collision Awareness System</h2>";
  html += "<table><tr><th>Road</th><th>Vehicle Count</th><th>Last Speed (cm/s)</th><th>Avg Speed (cm/s)</th></tr>";
  
  html += "<tr><td>North</td><td>" + String(roadN.vehicleCount) + "</td><td>" + String(roadN.lastSpeed) + "</td><td>" + String(roadN.avgSpeed) + "</td></tr>";
  html += "<tr><td>East</td><td>" + String(roadE.vehicleCount) + "</td><td>" + String(roadE.lastSpeed) + "</td><td>" + String(roadE.avgSpeed) + "</td></tr>";
  html += "<tr><td>South</td><td>" + String(roadS.vehicleCount) + "</td><td>" + String(roadS.lastSpeed) + "</td><td>" + String(roadS.avgSpeed) + "</td></tr>";
  html += "<tr><td>West</td><td>" + String(roadW.vehicleCount) + "</td><td>" + String(roadW.lastSpeed) + "</td><td>" + String(roadW.avgSpeed) + "</td></tr>";
  
  html += "</table></body></html>";
  server.send(200, "text/html", html);
}

unsigned long lastDisplayUpdate = 0;
void updateDisplay() {
  if (millis() - lastDisplayUpdate < 1000) return; // Update OLED every 1 second
  lastDisplayUpdate = millis();

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  display.setCursor(0, 0);
  display.println("--- ICCAS SYSTEM ---");
  
  display.printf("N: %d | %.1f cm/s\n", roadN.vehicleCount, roadN.lastSpeed);
  display.printf("E: %d | %.1f cm/s\n", roadE.vehicleCount, roadE.lastSpeed);
  display.printf("S: %d | %.1f cm/s\n", roadS.vehicleCount, roadS.lastSpeed);
  display.printf("W: %d | %.1f cm/s\n", roadW.vehicleCount, roadW.lastSpeed);
  
  int total = roadN.vehicleCount + roadE.vehicleCount + roadS.vehicleCount + roadW.vehicleCount;
  display.printf("Total Vol: %d\n", total);
  
  if (WiFi.status() == WL_CONNECTED) {
    display.println("WiFi: OK");
  } else {
    display.println("WiFi: OFF");
  }

  display.display();
}

void setup() {
  Serial.begin(115200);
  
  pinMode(roadN.pinEntry, INPUT_PULLUP); pinMode(roadN.pinExit, INPUT_PULLUP);
  pinMode(roadE.pinEntry, INPUT_PULLUP); pinMode(roadE.pinExit, INPUT_PULLUP);
  pinMode(roadS.pinEntry, INPUT_PULLUP); pinMode(roadS.pinExit, INPUT_PULLUP);
  pinMode(roadW.pinEntry, INPUT_PULLUP); pinMode(roadW.pinExit, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(roadN.pinEntry), isrN_Entry, FALLING);
  attachInterrupt(digitalPinToInterrupt(roadN.pinExit), isrN_Exit, FALLING);
  attachInterrupt(digitalPinToInterrupt(roadE.pinEntry), isrE_Entry, FALLING);
  attachInterrupt(digitalPinToInterrupt(roadE.pinExit), isrE_Exit, FALLING);
  attachInterrupt(digitalPinToInterrupt(roadS.pinEntry), isrS_Entry, FALLING);
  attachInterrupt(digitalPinToInterrupt(roadS.pinExit), isrS_Exit, FALLING);
  attachInterrupt(digitalPinToInterrupt(roadW.pinEntry), isrW_Entry, FALLING);
  attachInterrupt(digitalPinToInterrupt(roadW.pinExit), isrW_Exit, FALLING);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setCursor(0,10);
  display.setTextColor(SSD1306_WHITE);
  display.println("Booting ICCAS...");
  display.display();

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 10) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected.");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  }

  server.on("/", handleRoot);
  server.begin();
}

void loop() {
  roadN.processDetection();
  roadE.processDetection();
  roadS.processDetection();
  roadW.processDetection();

  server.handleClient();

  updateDisplay();
}