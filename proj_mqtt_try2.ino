
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <EEPROM.h>
#include <UniversalTelegramBot.h>
#include <PubSubClient.h>

#include "max6675.h"

int thermoDO = D7;
int thermoCS = D5;
int thermoCLK = D6;
int relayPin = D0;

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

const word MQTT_INTERVAL = 15000;
const word LTH_INTERVAL = 2500;
long lastLTH,lastMQTT,lastMC;
byte preReq;
const byte authNumber = 10;
word updateID = 1; 
float temperature[3] = {420, -127, -127};

struct authObj {
  String id;
  float TT;  //temperature threshold
  byte TD;
 };
authObj auth[authNumber];  
 


char ssid[] = "Dhanu";         
char password[] = "123456789";   
#define BOTtoken "828658125:AAFQjalg2OrF7gVGE5cbPFE1qHk7gfePzM8"  //Bot Token Telegram
const char* mqtt_server = "m24.cloudmqtt.com";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
int value = 0;

char* topic = "Topic";
char* User = "qfguwdmz";
char* Pass = "T7fcZ18gRo_L";

WiFiClientSecure espclient;
UniversalTelegramBot bot(BOTtoken, espclient);
int Bot_mtbs = 1000; 
long Bot_lasttime;   
bool Start = false;

struct confObj {
  byte en; 
 };  
confObj conf = {
  42,
};
 

void handleNewMessages(int numNewMessages) {
  Serial.print("handleNewMessage: ");
  byte is_auth;
  String msg;
 
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;
 
    Serial.println(text);
    String from_name = bot.messages[i].from_name;
    if (from_name == "") from_name = "Guest";
 
    is_auth = 0;
    for (byte j = 0; j < authNumber; j++) {
    
      if (chat_id == auth[j].id) {
        is_auth = 1;
        j = authNumber;
      }
    }
 
    if (!is_auth) {
      Serial.println("unauthorized");
 
      if ((text == "/status") || (text == "/start") || (text == "/help")) {
        msg = "This is temp_bot.\n";
        msg += "This Chat ID is: " + chat_id + "\n\n";
        msg += "/status : Get this Message\n";
        msg += "/test : Get Test Messages";
        bot.sendMessage(chat_id, msg, "");
      }
      if (text == "/admin_xxxx") {
        for (byte j = 0; j < authNumber; j++) {
          if (auth[j].id == "") {
            auth[j].id = chat_id;
            for (byte i = 0; i < chat_id.length() + 1; ++i) {
              EEPROM.write(100 + (42 * j) + i, chat_id[i]);
            }
            EEPROM.commit();
 
            is_auth = 1;
            text = "/help"; 
            j = authNumber;
          }
        }
      }
    }
    if (is_auth) {
      Serial.println("authorized");
      if (text == "/logout") {
        for (byte j = 0; j < authNumber; j++) {
          if (chat_id == auth[j].id) {
            auth[j].id = "";
            EEPROM.write(100 + (42 * j), 255);
            EEPROM.commit();
            is_auth = 0;
            j = authNumber;
          }
        }
        bot.sendMessage(chat_id, "You have been unregistered!", "");
      }
       if (text == "on") 
      {
        bot.sendMessage(chat_id, "Relay ON");
        Serial.println("Lamp ON");
        digitalWrite(relayPin, 0);     
      }
     if (text == "off") 
      {
        bot.sendMessage(chat_id, "Relay OFF");
        Serial.println("Lamp Off");
        digitalWrite(relayPin, 1);   
      }
      if (text == "/test") {
        for (byte j = 0; j < authNumber; j++) {
          if (auth[j].id != "") {
            bot.sendMessage(auth[j].id, "Test Message", "");
          }
        }
      }
 
 
      if (text == "/admin_out") {
        for (byte j = 0; j < authNumber; j++) {
          if (auth[j].id != "") {
            bot.sendMessage(auth[j].id, "You have been unregistered!", "");
            auth[j].id = "";
            EEPROM.write(100 + (42 * j), 255);
            is_auth = 0;
          }
        }
        EEPROM.commit();
        bot.sendMessage(chat_id, "All Admins have been unregistered!", "");
      }
 
      if (text == "/restart") {
        bot.sendMessage(chat_id, "restarting", "");
        delay(1000);
        ESP.restart();
        delay(3000);
      }
 
      if (text == "/tech") {
        msg = "Temp_bot Statistics.\n";
        msg += String(WiFi.SSID()) + ": " + String(WiFi.RSSI()) + " dBm\n";
        msg += "IP address: " + WiFi.localIP().toString() + "\n";
        msg += "Temperature: " + String(temperature[1]) + " °C\n";
        msg += "uptime : " + String(millis() / (1000 * 60 * 60)) + " h\n\n";
        msg += "Your alarm Thresholds:\n";
        for (byte j = 0; j < authNumber; j++) {
          if (chat_id == auth[j].id) {
            if (auth[j].TD) {
              msg += "T: " + String(auth[j].TT) + " °C (" + String(auth[j].TD) + ")\n";
            }
          
            j = authNumber;
          }
        }
        msg += "\nYou are Registered!\n";
        msg += "This Chat ID is: " + chat_id + "\n";
        msg += "Registered Chat IDs:\n";
        for (byte j = 0; j < authNumber; j++) {
          msg += auth[j].id + " ";
        }
        bot.sendMessage(chat_id, msg, "");
 
      }
 
      if ((text == "/status")) {
        bot.sendMessage(chat_id, SState(), ""); 
      }
 
      if ((text == "/help") || (text == "/start")) {
        
        msg = "This is temp_bot.\n";
        msg += "You are Registered!\n\n";
        msg += "/help : Get this Message\n";
        msg += "/status : Get Sensor Status\n";
        msg += "/tech : Get Statistics\n";
        msg += "/logout : Unregister\n";
        msg += "/test : Get Test Messages\n\n";
        msg += "Threshold Notifications:\n";
        msg += "/alarm_T_ : Temperature\n";
        msg += "eg: /alarm_T_40\n";
        bot.sendMessage(chat_id, msg, "");
      }
 
      if (text.startsWith("/alarm_T_")) {
        for (byte j = 0; j < authNumber; j++) {
          if (chat_id == auth[j].id) {
            if (text.substring(9) != "") {
              auth[j].TT = text.substring(9).toFloat();
              if (auth[j].TT > temperature[1]) {
                auth[j].TD = 2;
                msg = "I will notify you once the Temperature increases to:\n";
              } else {
                auth[j].TD = 1;
                msg = "I will notify you once the Temperature drops to:\n";
              }
              msg += String(auth[j].TT) + " °C";
              bot.sendMessage(chat_id, msg, "");
            } else {
              auth[j].TD = 0;
              msg = "Alarm deactivated";
              bot.sendMessage(chat_id, msg, "");
            }
            EEPROM.put(100 + (42 * j) + 17, auth[j].TT);
            EEPROM.put(100 + (42 * j) + 21, auth[j].TD);
            EEPROM.commit();
            j = authNumber;
          }
        }
      }
     
     
    if (text == "/test") {
      for (int i = 0; i < 5; i++) {
        bot.sendMessage(chat_id, "test", "");
        delay(0);
      }
    }
  }
}
}
String SState() {
  String msg = "This is temp_bot.\n";
  msg += "Temperature: " + String(temperature[1]) + " °C\n";
  msg += "Min:\n" + String(temperature[0]) + "°C\n " ;
  msg += "Max:\n" + String(temperature[2]) + "°C " ;
  return msg;
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

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

void reconnect() {
  String msg;
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect("Arduino",User, Pass)) {
      Serial.println("connected");
        msg = String(updateID);
    client.publish("demm_bot/updateID", msg.c_str());
      // Once connected, publish an announcement...
    
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() 
{
  Serial.begin(115200);
  espclient.setInsecure();
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, 1);
   setup_wifi();
  client.setServer(mqtt_server,15876);

  EEPROM.begin(768);
 

 
  if (EEPROM.read(0) == conf.en) {
    Serial.println("reading valid EEPROM");
    EEPROM.get(0, conf);
    
    for (byte j = 0; j < authNumber; j++) {
      for (byte i = 0; i < 17; ++i) {
        char tmp = char(EEPROM.read(100 + (42 * j) + i));
 
        if ((tmp == '\0') || (tmp == 255)) {
          i = 17;
        }
        else {
          auth[j].id += tmp;
        }
      }
     
      EEPROM.get(100 + (42 * j) + 17, auth[j].TT);
      EEPROM.get(100 + (42 * j) + 21, auth[j].TD);
     }
  } else {
    Serial.println("config not found on EEPROM, using defaults");
    for (byte j = 0; j < authNumber; j++) {
      EEPROM.write(100 + (42 * j), 255);
      EEPROM.write(100 + (42 * j) + 21, 0);
      EEPROM.write(100 + (42 * j) + 26, 0);
      EEPROM.write(100 + (42 * j) + 31, 0);
      EEPROM.write(100 + (42 * j) + 36, 0);
    }
    EEPROM.commit();
  }
  EEPROM.put(0, conf);
  EEPROM.commit();
}

 void getdht() {
    temperature[1] = -127;
    float t =thermocouple.readCelsius();   
    temperature[1]=t;      
    Serial.print("temperature = ");
    Serial.print(t); 
    Serial.println("C  ");
  delay(800);
 
  }
  


void loop() 
{
   String msg;
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (((millis() - lastMQTT) > MQTT_INTERVAL) && (preReq >= 2)) {  //
    lastMQTT = millis();
    preReq = 0;
  if (client.connected()) {
    
      msg = String(temperature[1]);
      client.publish("Temp_bot/T", msg.c_str());
  }
    }
    
    if ((millis() - lastLTH) > LTH_INTERVAL) {
    lastLTH = millis();
    preReq++;
 
    Serial.print("lt");
  
    getdht();
    
    if (temperature[1] < temperature[0]) {
      temperature[0] = temperature[1];
    }
    if (temperature[1] > temperature[2]) {
      temperature[2] = temperature[1];
    }
   
    for (byte j = 0; j < authNumber; j++) {
      if (auth[j].TD == 2) {
        if (auth[j].TT < temperature[1]) {
          auth[j].TD = 0;
          EEPROM.write(100 + (42 * j) + 21, 0);
          EEPROM.commit();
          msg = "The Temperature reached:\n";
          msg += String(auth[j].TT) + " °C\n";
          bot.sendMessage(auth[j].id, msg, "");
          bot.sendMessage(auth[j].id, SState(), ""); 
            digitalWrite(relayPin, 1); 
           bot.sendMessage(auth[j].id, "Relay OFF");
        }
      }
      if (auth[j].TD == 1) {
        if (auth[j].TT > temperature[1]) {
          auth[j].TD = 0;
          EEPROM.write(100 + (42 * j) + 21, 0);
          EEPROM.commit();
          msg = "The Temperature reached:\n";
          msg += String(auth[j].TT) + " °C\n";
          bot.sendMessage(auth[j].id, msg, "");
          bot.sendMessage(auth[j].id, SState(), "");  
          digitalWrite(relayPin, 0); 
           bot.sendMessage(auth[j].id, "Relay ON");
        }
      }
     
      }

  if (millis() > Bot_lasttime + Bot_mtbs)  
  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while(numNewMessages) 
    {
      Serial.println("");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    Bot_lasttime = millis();
  }
}
if (millis() < lastMQTT) {
    lastMQTT = millis();
  }
  if (millis() < lastLTH) {
    lastLTH = millis();
  }
  if (millis() < lastMC) {
    lastMC = millis();
  }
  if (millis() < Bot_lasttime) {
    Bot_lasttime = millis();
  }
}
