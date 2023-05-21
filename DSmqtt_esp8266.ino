/*
  Basic ESP8266 MQTT example
  This sketch demonstrates the capabilities of the pubsub library in combination
  with the ESP8266 board/library.
  It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,
    else switch it off
  It will reconnect to the server if the connection is lost using a blocking
  reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
  achieve the same result without blocking the main loop.
  To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <string.h>

// Update these with values suitable for your network.

const char* ssid = "WIFI_NAME";
const char* password = "WIFI_PSW";
const char* mqtt_server = "192.168.1.16";

#define mqtt_user "mqtt-user"
#define mqtt_password "kittens"
#define generalUpdatePeriod 1000
#define soilMoistureUpdatePeriodSlow 60*30*1000
#define soilMoistureUpdatePeriodFast 60*1000
#define fastLoggingDuration (120*60*1000)
unsigned long soilMoistureUpdatePeriod = soilMoistureUpdatePeriodSlow;

#define FLOAT1 14
#define FLOAT2 12
#define FLOAT3 13
#define FLOAT4 10
const int analogInPin = A0;
#define SOILMIN 0
#define SOILMAX 1023
#define SOILMUX 16
#define VALVE1 4
#define VALVE2 0
#define PUMP 5

#define FLOATTOPIC "floatPct"
#define SOILTOPIC1 "soilPct1"
#define SOILTOPIC2 "soilPct2"
#define VALVECMDTOPIC1 "valveCmdTopic1"
#define VALVESTATETOPIC1 "valveStateTopic1"
#define VALVECMDTOPIC2 "valveCmdTopic2"
#define VALVESTATETOPIC2 "valveStateTopic2"
#define PUMPCMDTOPIC "pumpCmdTopic"
#define PUMPSTATETOPIC "pumpStateTopic"
#define SOILUPDATECMDTOPIC "soilUpdateCmdTopic"
#define SOILUPDATESTATETOPIC "soilUpdateStateTopic"
#define FORCESOILUPDATETOPIC "forceSoilUpdateTopic"


WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsgGeneral = 0;
unsigned long lastMsgSoilMoisture = 0;
unsigned long lastSoilUpdatePeriod =0;

float floatBuffer[10];
float lastValidFloat = 0;
int floatBufferPosition = 0;

#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void actuateValve(int valve, int state) {
  if (valve == 1) {
    if (state == 1) {
      digitalWrite(VALVE1, LOW);
      client.publish(VALVESTATETOPIC1, "1");
    } else if (state == 0) {
      digitalWrite(VALVE1, HIGH);
      client.publish(VALVESTATETOPIC1, "0");
    }
  } else if (valve == 2) {
    if (state == 1) {
      digitalWrite(VALVE2, LOW);
      client.publish(VALVESTATETOPIC2, "1");
    } else if (state == 0) {
      digitalWrite(VALVE2, HIGH);
      client.publish(VALVESTATETOPIC2, "0");
    }
  }
}

void actuatePump(int state) {
  if (state == 1) {
    digitalWrite(PUMP, LOW);
    client.publish(PUMPSTATETOPIC, "1");
  } else if (state == 0) {
    digitalWrite(PUMP, HIGH);
    client.publish(PUMPSTATETOPIC, "0");
  }
}

void actuateSoilMoistureUpdate(int state) {
  if (state == 1) {
    soilMoistureUpdatePeriod = soilMoistureUpdatePeriodFast;
    client.publish(SOILUPDATESTATETOPIC, "1");
  } else if (state == 0) {
    soilMoistureUpdatePeriod = soilMoistureUpdatePeriodSlow;
    client.publish(SOILUPDATESTATETOPIC, "0");
  }
  lastSoilUpdatePeriod = millis();
}



  void callback(char* topic, byte * payload, unsigned int length) {

    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
      Serial.print((char)payload[i]);
    }
    Serial.println();

    if ( strcmp(topic, VALVECMDTOPIC1) == 0) {
      if ((char)payload[0] == '1') {
        actuateValve(1, 1);
      } else if ((char)payload[0] == '0') {
        actuateValve(1, 0);
      }
    } else if ( strcmp(topic, VALVECMDTOPIC2) == 0) {
      if ((char)payload[0] == '1') {
        actuateValve(2, 1);
      } else if ((char)payload[0] == '0') {
        actuateValve(2, 0);
      }
    } else if ( strcmp(topic, PUMPCMDTOPIC) == 0) {
      if ((char)payload[0] == '1') {
        actuatePump(1);
      } else if ((char)payload[0] == '0') {
        actuatePump(0);
      }
    } else if ( strcmp(topic, SOILUPDATECMDTOPIC) == 0) {
      if ((char)payload[0] == '1') {
        actuateSoilMoistureUpdate(1);
      } else if ((char)payload[0] == '0') {
        actuateSoilMoistureUpdate(0);
      }
    } else if ( strcmp(topic, FORCESOILUPDATETOPIC) == 0) {
      updateSoilMoisture();
    } 

  }

  void reconnect() {
    // Loop until we're reconnected
    while (!client.connected()) {
      Serial.print("Attempting MQTT connection...");
      // Create a random client ID
      String clientId = "ESP8266Client-";
      clientId += String(random(0xffff), HEX);
      // Attempt to connect
      if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
        Serial.println("connected");
        // Once connected, publish an announcement...
        client.publish("outTopic", "hello world");
        // ... and resubscribe
        client.subscribe("inTopic");
        client.subscribe(VALVECMDTOPIC1);
        client.subscribe(VALVECMDTOPIC2);
        client.subscribe(PUMPCMDTOPIC);
        client.subscribe(SOILUPDATECMDTOPIC);
        client.subscribe(FORCESOILUPDATETOPIC);
      } else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        // Wait 5 seconds before retrying
        delay(5000);
      }
    }
  }

  void setup() {
    // pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
    pinMode(FLOAT1, INPUT_PULLUP);
    pinMode(FLOAT2, INPUT_PULLUP);
    pinMode(FLOAT3, INPUT_PULLUP);
    pinMode(FLOAT4, INPUT_PULLUP);
    pinMode(SOILMUX, OUTPUT);
    pinMode(VALVE1, OUTPUT);
    pinMode(VALVE2, OUTPUT);
    pinMode(PUMP, OUTPUT);
    actuatePump(0);
    actuateValve(1,0);
    actuateValve(2,0);
    

    Serial.begin(115200);
    setup_wifi();
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
  }

float readFloatArray(void){
    char floatSwitchArray[] = "0000";
    float floatPercent = -1;
    floatSwitchArray[0] = digitalRead(FLOAT1) + '0';
    floatSwitchArray[1] = digitalRead(FLOAT2) + '0';
    floatSwitchArray[2] = digitalRead(FLOAT3) + '0';
    floatSwitchArray[3] = digitalRead(FLOAT4) + '0';
    Serial.println("float switch state: ");
    Serial.println(floatSwitchArray);
    //write switch case and use str comp
    if (strcmp(floatSwitchArray, "0000") == 0) {
      Serial.println("float array 0");
      floatPercent = 0;
    } else if (strcmp(floatSwitchArray, "0001") == 0) {
      Serial.println("float array 5");
      floatPercent = .05;
    } else if (strcmp(floatSwitchArray, "0011") == 0) {
      Serial.println("float array 25");
      floatPercent = .25;
    } else if (strcmp(floatSwitchArray, "0111") == 0) {
      Serial.println("float array 60");
      floatPercent = .6;
    } else if (strcmp(floatSwitchArray, "1111") == 0) {
      Serial.println("float array 100");
      floatPercent = 1;
    } else {
      Serial.println("float array error");
      floatPercent = -1;
    }
  return floatPercent;
}

  void updateGeneral(void) {
    float trialFloat;
    int allSameFlag = 1;
    
    trialFloat = readFloatArray();
    floatBuffer[floatBufferPosition] = trialFloat;
    
    for( int i= 0; i<9; i++){
      if(floatBuffer[i] != floatBuffer[i+1]){
        allSameFlag = 0;
      }  
    }
    if(allSameFlag){
          client.publish(FLOATTOPIC, String(trialFloat).c_str());
          lastValidFloat = trialFloat;
    }else{
        client.publish(FLOATTOPIC, String(lastValidFloat).c_str());
    }
    floatBufferPosition++;
    
    if(floatBufferPosition >=10){
      floatBufferPosition = 0;
    }

  }

int readAverageAnalog(int analogPin){
  unsigned int sval = 0;
  for (int i = 0; i < 10; i++){
    sval += analogRead(analogPin);
    delay (10);
  }
  return sval / 10;
}

void updateSoilMoisture() {
    int sensorValue = -1;
    float soilPct = -1;

    if (digitalRead(SOILMUX) == LOW) {
      sensorValue = readAverageAnalog(analogInPin);
      Serial.print("soil sensor 1 = ");
      Serial.println(sensorValue);
      soilPct = (float(sensorValue) - SOILMIN) / (SOILMAX - SOILMIN);

      Serial.print("soil 1 pct= ");
      Serial.println(soilPct);
      client.publish(SOILTOPIC1, String(soilPct).c_str());

      //read and report sensor 2
      digitalWrite(SOILMUX, HIGH);
      delay(100);
      sensorValue = readAverageAnalog(analogInPin);
      Serial.print("soil sensor 2 = ");
      Serial.println(sensorValue);
      soilPct = (float(sensorValue) - SOILMIN) / (SOILMAX - SOILMIN);

      Serial.print("soil 2 pct= ");
      Serial.println(soilPct);
      client.publish(SOILTOPIC2, String(soilPct).c_str());
    } else {
      sensorValue = readAverageAnalog(analogInPin);
      Serial.print("soil sensor 2 = ");
      Serial.println(sensorValue);
      soilPct = (float(sensorValue) - SOILMIN) / (SOILMAX - SOILMIN);

      Serial.print("soil 2 pct= ");
      Serial.println(soilPct);
      client.publish(SOILTOPIC2, String(soilPct).c_str());

      //read and report sensor 1
      digitalWrite(SOILMUX, LOW);
      delay(100);
      sensorValue = readAverageAnalog(analogInPin);
      Serial.print("soil sensor 1 = ");
      Serial.println(sensorValue);
      soilPct = (float(sensorValue) - SOILMIN) / (SOILMAX - SOILMIN);

      Serial.print("soil 1 pct= ");
      Serial.println(soilPct);
      client.publish(SOILTOPIC1, String(soilPct).c_str());

    }


  }

  void loop() {

    if (!client.connected()) {
      reconnect();
      updateGeneral();
      updateSoilMoisture();
      actuateSoilMoistureUpdate(0);
    }
    client.loop();

    unsigned long now = millis();
    if (now - lastMsgGeneral > generalUpdatePeriod) {
      lastMsgGeneral = now;
      updateGeneral();
    }

    if (now - lastMsgSoilMoisture > soilMoistureUpdatePeriod) {
      lastMsgSoilMoisture = now;
      updateSoilMoisture();
    }

    if (now - lastSoilUpdatePeriod > fastLoggingDuration) {
      lastSoilUpdatePeriod = now;
      actuateSoilMoistureUpdate(0);
    }
    

  }
