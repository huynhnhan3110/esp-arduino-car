#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>
#include <Servo.h>
// ============= SET WIFI STATION CREDENTIALS ============= //
#define WIFI_SSID "Viettel_Thay Tam"
#define WIFI_PASS "31102000"

// ============= SET WIFI ACCESS POINT CREDENTIALS ============= //
#define AP_SSID "ESP8266"
#define AP_PASS "magicword"

// ============= SET FIXED IP STATION MODE ========= //
IPAddress ip(192,168,1,200); 
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

// ============= CREATE WEBSOCKET ========= //
WebSocketsServer webSocket = WebSocketsServer(81);

// ============= DEFINE PINOUT ========= //
uint8_t IN1 = 5; // D1
uint8_t IN2 = 4; // D2
uint8_t IN3 = 14; // D5
uint8_t IN4 = 12; // D6
uint8_t enA = 16; // D0
uint8_t enB = 13;// D7
uint8_t BUZZER = 3; // RX PIN

// ============= DEFINE SERVO ========= //
Servo Reload;    // Servo1 
Servo Triger;    // Servo2 
Servo Charge;    // Servo3 

// ============= DEFINE GLOBAL VARIABLES ========= //
const int onDuration=1000;
const int periodDuration=4000;
bool sound_repeat = true;
unsigned long lastPeriodStart;

uint8_t ter;
int motorSpeedA = 0;
int motorSpeedB = 0;
int yAxis;
int xAxis;
uint8_t maxSpeed = 0;

void setup() {
  ter = 0;
// ============= SETUP SERIAL PORT ========= //
  Serial.begin(115200);
  Serial.println();

// ============= SETUP PIN ========= //
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(enA, OUTPUT);
  pinMode(enB, OUTPUT);
  pinMode(BUZZER,OUTPUT);
  
// ============= SETUP SERVO ========= //
  Reload.attach(15);   // Set Servo_Reload signal pin to D8
  Triger.attach(0);   // Set Servo_Triger signal pin to d3
  Charge.attach(2);   // Set Servo_Charge signal pin to D4
  
// ============= BEGIN ACCESS POINT ========= //
  WiFi.softAP(AP_SSID, AP_PASS);
  
// ============= BEGIN STATION ========= //
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  WiFi.config(ip, gateway, subnet);

// ============= AT STATION -> CONNECTING TO WIFI ========= //
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }
  
// ============= CONNECTED TO NETWORK ========= //
  Serial.println();
  Serial.println("Connected!");
  Serial.print("IP address for network ");
  Serial.print(WIFI_SSID);
  Serial.print(" : ");
  Serial.println(WiFi.localIP());
  Serial.print("IP address for network ");
  Serial.print(AP_SSID);
  Serial.print(" : ");
  Serial.print(WiFi.softAPIP());
  
// ============= SETUP WEBSOCKET ========= //
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}
  
void loop() {
// ============= BUZZER TONE WHEN NOT CONNECT ========= //
   if(sound_repeat) {
      if (millis()-lastPeriodStart>=periodDuration)
    {
      lastPeriodStart+=periodDuration;
      tone(BUZZER,550, onDuration); // play 550 Hz tone in background for 'onDuration'
    }
  }

// ============= WEBSOCKET LOOP ========= //
  webSocket.loop();

}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {

  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      sound_repeat = true;
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %s url: %s\n", num, ip.toString().c_str(), payload);
        
        // send message to client
//        String maggase = "Connected to Serial on " + WiFi.localIP().toString() + "\n";
//        webSocket.sendTXT(num, maggase);
      webSocket.sendTXT(num, "connected");
      sound_repeat = false;
      }
      break;
    case WStype_TEXT:
    {
      String cmd = (const char *)payload;
      
  	  // Speed control
      if(cmd[0] == 's') {
        maxSpeed = map(cmd.substring(1).toInt(),10,100,170,255);
        Serial.println(maxSpeed);
      }
	  
  	  // AUTO FIRE EVENT
  	  if(cmd == "AutoFire") {
    		Serial.println("Auto Fire");
    		while(ter < 3) {
    			Fire();
    			ter++;
    		}
    		ter=0;
  	  }
  	  
  	  // FIRE EVENT 
  	  if(cmd == "Fire") {
    		Serial.println("Fire");
    	  Fire();
  	  }
  	  // BUZZER EVENT
      if (cmd == "buzzerOn") {
        Serial.println("buzzeron");
        digitalWrite(BUZZER, HIGH); //Turn buzzer ON
      }
      if (cmd == "buzzerOff") {
        Serial.println("buzzeroff");
        digitalWrite(BUZZER, LOW); //Turn buzzer ON
      }
      // Motor Control
      if(cmd[0] =='x'){
        Serial.println(cmd.substring(1).toInt());
        xAxis = cmd.substring(1).toInt();
      }
      if(cmd[0] =='y'){
        Serial.println(cmd.substring(1).toInt());
        yAxis = cmd.substring(1).toInt();
      }
      
      if(yAxis < -35) {
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);
        digitalWrite(IN3, HIGH);
        digitalWrite(IN4, LOW);
        motorSpeedA = map(yAxis, -35, -126, 0, maxSpeed);
        motorSpeedB = map(yAxis, -35, -126, 0, maxSpeed);
      }else if (yAxis > 35) {
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, HIGH);
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, HIGH);
        motorSpeedA = map(yAxis, 35, 126, 0, maxSpeed);
        motorSpeedB = map(yAxis, 35, 126, 0, maxSpeed);
      }else {
        motorSpeedA = 0;
        motorSpeedB = 0;
      }
       if (xAxis > 35) {
        int xMapped = map(xAxis, 35, 126, 0, maxSpeed);
        motorSpeedA = motorSpeedA + xMapped;
        motorSpeedB = motorSpeedB - xMapped;
        if (motorSpeedB < 0) {
          motorSpeedB = 0;
        }
        if (motorSpeedA > 255) {
          motorSpeedA = 255;
        }
      }
      if (xAxis < -35) {
        int xMapped = map(xAxis, -35, -126, 0, maxSpeed);
        motorSpeedA = motorSpeedA - xMapped;
        motorSpeedB = motorSpeedB + xMapped;
        if (motorSpeedB > 255) {
          motorSpeedB = 255;
        }
        if (motorSpeedA < 0) {
          motorSpeedA = 0;
        }
      }
       if (motorSpeedA < 40) {
        motorSpeedA = 0;
      }
      if (motorSpeedB < 40) {
        motorSpeedB = 0;
      }

      analogWrite(enA, motorSpeedA); 
      analogWrite(enB, motorSpeedB);
      break;
  }
    case WStype_BIN:
      Serial.printf("[%u] get binary lenght: %u\n", num, lenght);
      hexdump(payload, lenght);
      // send message to client
      // webSocket.sendBIN(num, payload, lenght);
      break;
  }
}

void Fire(){
  Triger.write(0);
  delay(500);   
  Reload.write(0); // 20
  delay(500);
  Triger.write(0);  
  delay(500);
  Charge.write(0); 
  delay(1500);
  Triger.write(90); 
  delay(500);
  Charge.write(85); // Set Stop angle Watch picture!! 
  delay(500);
  Reload.write(160); // 90
  delay(500); 
  Reload.write(0); // 20
  delay(500); 
}

void sound_noti(int tone1, int tone2, int tone3) {
  tone(BUZZER, tone1); 
  delay (200); 
  tone(BUZZER, tone2);
  delay (200); 
  tone(BUZZER, tone3); 
  delay (200); 
  noTone(BUZZER);
}
