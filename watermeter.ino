// (c) Edwin Zuidema
// Proximity detector to water flow
// v1.0

#include <ESP8266WiFi.h>
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>

#define PROX_PIN 2           // D4 is connected to GPIO2 on NodeMCU
#define PULSE_FACTOR 1000    // Number of blinks per m3 of the meter (One rotation/liter)
#define SEND_FREQUENCY 30000 // Minimum time between updates (in milliseconds)

//#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
  #define IRQ_HANDLER_ATTR ICACHE_RAM_ATTR
//#else
//  #define IRQ_HANDLER_ATTR
//#endif

// Wifi: SSID and password
char* WIFI_SSID = "SSID";
char* WIFI_PASSWORD = "PASSWORD";

// MySQL: server IP, port, username and password
IPAddress MYSQL_SERVER_IP (192,168,10,10);
char* MYSQL_USER = "user";
char* MYSQL_PASSWORD = "password";
char* MYSQL_DATABASE = "water";
char* MYSQL_INSERT_DATA = "INSERT INTO water.water (datetime, flow, pulseCount, volume) VALUES (UTC_TIMESTAMP(), %f, %d, %f)";
char query[128];

WiFiClient client;
MySQL_Connection conn((Client *)&client);

double ppl = ((double)PULSE_FACTOR)/1000;        // Pulses per liter

volatile uint32_t pulseCount = 0;
volatile uint32_t lastBlink = 0;
volatile double flow = 0;
//bool pcReceived = false;
uint32_t oldPulseCount = 0;
double oldflow = 0;
double oldvolume =0;
uint32_t lastSend =0;
uint32_t lastPulse =0;


void IRQ_HANDLER_ATTR onPulse()
{
  uint32_t newBlink = micros();
  uint32_t interval = newBlink-lastBlink;

  Serial.println("Prox IRQ Handler:");
  Serial.print("Interval: ");
  Serial.print(interval);
  Serial.println(" us");

  if (interval != 0) {
    lastPulse = millis();
    if (interval<500000L) {
      // Sometimes we get interrupt on RISING, 500000 = 0.5 second debounce (~120 l/m)
      Serial.println("Bounce, no action");
      return;
    }
    flow = (60000000.0 / interval) / ppl;
    Serial.print("Flow ( (1 min / interval) / pulses per liter): ");
    Serial.print(flow);
    Serial.println(" l/min");
  }
  lastBlink = newBlink;
  pulseCount++;
  Serial.print("pulsecount: ");
  Serial.println(pulseCount);
}

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(115200);
  delay(500);
  Serial.println("Setup: Serial opened");

  // init the WiFi connection
  Serial.print("INFO: Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);   
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("INFO: WiFi connected");
  Serial.println("INFO: IP address: ");
  Serial.println(WiFi.localIP());

  Serial.print("Connecting to MySQL...  ");
  if (conn.connect(MYSQL_SERVER_IP, 3306, MYSQL_USER, MYSQL_PASSWORD)) {
    Serial.println("OK.");
    delay(1000);
  } else {
    Serial.println("FAILED.");
  }
  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
  row_values *row = NULL;

  sprintf(query, "SELECT pulseCount FROM water.water ORDER BY datetime DESC LIMIT 1;");
  Serial.print("MySQL query: ");
  Serial.println(query);
  // Execute the query
  cur_mem->execute(query);

  // Getch the columns (required?!)
  column_names *columns = cur_mem->get_columns();

  // Read the row (we are only expecting the one)
  row = cur_mem->get_next_row();
  if (row != NULL) {
    pulseCount = oldPulseCount = atoi(row->values[0]);
    Serial.print("pulsecount: ");
    Serial.println(pulseCount);
  } else {
    pulseCount = oldPulseCount = 0;  
  }
  Serial.println("Done with MySQL.");
  
  lastSend = millis();
  attachInterrupt(digitalPinToInterrupt(PROX_PIN), onPulse, FALLING);
}

void loop() {
//  Serial.println("Loop...");
  row_values *row = NULL;
  uint32_t currentTime = millis();

  // Only update values at a maximum frequency
  if (currentTime - lastSend > SEND_FREQUENCY) {
    lastSend=currentTime;
    Serial.println("Updating:");

    if (flow != oldflow) {
      oldflow = flow;
    }
    // No Pulse count received in 2min then no flow
    if(currentTime - lastPulse > 120000) {
      Serial.println("(no pulse received since last update)");
      flow = 0;
    }
    Serial.print("flow: ");
    Serial.print(flow);
    Serial.println(" l/min:");

    // Pulse count has changed, update all data and insert into DB
    if (pulseCount != oldPulseCount) {
      oldPulseCount = pulseCount;
      Serial.print("pulsecount: ");
      Serial.print(pulseCount);
      Serial.print(" (oldPulsecount: ");
      Serial.print(oldPulseCount);
      Serial.println(")");

      double volume = ((double)pulseCount/((double)PULSE_FACTOR));
      if (volume != oldvolume) {
        oldvolume = volume;
        Serial.print("volume (pulsecount / factor): ");
        Serial.print(volume, 3);
        Serial.println(" m3");
        //send(volumeMsg.set(volume, 3));               // Send volume value to gw
        Serial.println("Check if MySQL is still connected");
        if (conn.connected()) {
          MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
          sprintf(query, MYSQL_INSERT_DATA, flow, pulseCount, volume);
          Serial.print("MySQL query: ");
          Serial.println(query);

          // Execute the query
          cur_mem->execute(query);
          Serial.println("Done with MySQL.");

          // Deleting the cursor also frees up memory used
          delete cur_mem;
        } else {
          Serial.println("MySQL NOT connected (anymore)");
        }
      }
    }
  }
}
