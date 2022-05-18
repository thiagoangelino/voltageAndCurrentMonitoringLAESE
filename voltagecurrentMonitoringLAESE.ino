
/*
    Thiago Angelino dos Santos
    https://thiagoangelino.com
*/
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <AutoConnect.h>
#include <ESP32_Servo.h>
#include "time.h"

WebServer Server;
AutoConnect Portal(Server);
Servo myservo; // atach the servo

//NTP config
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = -3600*3;

//Constants to connect with ThingSpeak
const char* host = "api.thingspeak.com";
String api_key = "C6BWS2TW4S746CCX"; // Your API Key provied by thingspeak

// Variables to meansurement
const int voltagePin = 35;
const int currentPin = 34;  
int voltageValue;  
int currentValue;
double voltageValue_conv;
double voltageValue_conv2;  
double currentValue_conv; 
double currentValue_conv2;
double voltage_sensTen_mv;
double voltage_sensCur_mv;
double ACSoffset = 2500; 
double potFV;
double mVperAmp = 100;                  // use 100 for 20A Module and 66 for 30A Module
double RawValue= 0;
char timeStr[20]= "0000000000000000000";

int current = 0; // setup a int to simulate a current
int i = 0; //incremento
float vetCorrente[300];
float vetTensao[300];

void rootPage() {
  char content[] = "Hello, world";
  Server.send(200, "text/plain", content);
}

//Your Domain name with URL path or IP address with path
String serverName = "http://sanusb.org/ftpmonitor";

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
//unsigned long timerDelay = 600000;
// Set timer to 5 seconds (5000)
unsigned long timerDelay = 600000;

void setup() {
  
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  myservo.attach(26);
  myservo.write(0);

  Server.on("/", rootPage);
  
  if (Portal.begin()) {
    Serial.println("WiFi connected: " + WiFi.localIP().toString());
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Serial.println(getDateTime());
  }
  
 
}

void loop() {
  String temp = getDateTime();
  temp.toCharArray(timeStr, 20);
  
  Portal.handleClient();
  

  //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      measurement();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();

  //Send an HTTP POST request every 10 minutes
  if ((millis() - lastTime) > timerDelay) {
    
  }
  //*/

}

void measurement(){
      Serial.println("measurement function");
      double highestCurrent = 0;
      double highestVoltage = 0;
      double valor_Corrente = 0;  
      
      voltageValue = analogRead(voltagePin);   
      voltageValue = voltageValue;
      currentValue = analogRead(currentPin); 
      currentValue = currentValue;    
    
      
     /// Filter
      for(i = 1; i <= 100; i++){
        for(int i = 0; i < 300; i++){
          vetCorrente[i] = analogRead(currentPin);
          vetTensao[i] = analogRead(voltagePin);
          delayMicroseconds(1000);
        }  
         
        for(int i = 0; i < 300; i++){
          if(highestCurrent < vetCorrente[i]){ highestCurrent = vetCorrente[i]; }
          if(highestVoltage < vetTensao[i]){ highestVoltage = vetTensao[i]; }
        } 
    
        voltage_sensCur_mv = (highestCurrent / 4096.0) * 3600; // Gets you mV
        //currentValue_conv = ((voltage_sensCur_mv - ACSoffset) / mVperAmp)-0.5;
        currentValue_conv = (((voltage_sensCur_mv - ACSoffset) / mVperAmp)*0.89)-0.6;
  
        voltage_sensTen_mv = (highestVoltage/4096.0) * 3.6; // Gets you mV
        //voltageValue_conv = voltage_sensTen_mv * 99;
        voltageValue_conv = voltage_sensTen_mv * 93;
        
        // Média Corrente
        currentValue_conv = (currentValue_conv2 + currentValue_conv)/2;
        currentValue_conv2 = currentValue_conv;
  
        // Média Tensão
        voltageValue_conv = (voltageValue_conv2 + voltageValue_conv)/2;
        voltageValue_conv2 = voltageValue_conv;
      }
      
      if (voltageValue_conv < 0){currentValue_conv = 0;};
      potFV = voltageValue_conv * currentValue_conv;

      SendToThingSpeak();
      SendToMonitorWeb(String(voltageValue_conv), String(currentValue_conv), getDateTime());
      Serial.println("Tensão: " + String(voltageValue_conv) + " V");
      Serial.println("Corente: " + String(currentValue_conv) + " A");
      Serial.println("Potência: " + String(potFV) + " W");
}

 void SendToThingSpeak()
{
  
  Serial.println("...");
  Serial.println("Preparing to send data");

  // Use WiFiClient class to create TCP connections
  WiFiClient client;

  const int httpPort = 80;

  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  else
  {
    String data_to_send = api_key;
    data_to_send += "&field1=";
    data_to_send += String(voltageValue_conv);
    data_to_send += "&field2=";
    data_to_send += String(currentValue_conv);
    data_to_send += "&field3=";
    data_to_send += String(potFV);
    data_to_send += "&field4=";
    data_to_send += String(voltageValue);
    data_to_send += "&field5=";
    data_to_send += String(currentValue);
    data_to_send += "\r\n\r\n";

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + api_key + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(data_to_send.length());
    client.print("\n\n");
    client.print(data_to_send);

    delay(250);
  }

  client.stop();

}

void SendToMonitorWeb(String current, String voltage, String dateTime){
  HTTPClient http;

  String serverPath = serverName + "/getESP_maracanau.php?action=send3&tensao1=" + voltage + "&corrente1=" + current + "&date=" + dateTime;
  Serial.println(serverPath);
  // Your Domain name with URL path or IP address with path
  http.begin(serverPath.c_str());
  
  // Send HTTP GET request
  int httpResponseCode = http.GET();
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String payload = http.getString();
    Serial.println(payload);
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();
}

String getDateTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return "Failed to obtain time";
  }
  
  //Serial.println(&timeinfo, "%Y-%m-%d-%H:%M:%S");
  String day, mon, hour, minute, sec;
  if(int(timeinfo.tm_mday) < 10){ day = "0" + String(timeinfo.tm_mday); } else{day = String(timeinfo.tm_mday); }
  if(int(timeinfo.tm_mon) < 10){ mon = "0" + String(timeinfo.tm_mon + 1); }  else {mon = String(timeinfo.tm_mon + 1); }
  if(int(timeinfo.tm_hour) < 10){ hour = "0" + String(timeinfo.tm_hour); } else {hour = String(timeinfo.tm_hour); }
  if(int(timeinfo.tm_min) < 10){ minute = "0" + String(timeinfo.tm_min); } else {minute = String(timeinfo.tm_min); } 
  if(int(timeinfo.tm_sec) < 10){ sec = "0" + String(timeinfo.tm_sec); } else { sec = String(timeinfo.tm_sec); }

  String dateTime = String(timeinfo.tm_year + 1900) + "-" + mon + "-" + day + "-" + 
                    hour + ":" + minute + ":" + sec;

  return dateTime;
  
}
