#include <ArduinoJson.h>
#include <UniversalTelegramBot.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <LiquidCrystal.h>
#include <PubSubClient.h>

//bottone cambio schermata
int pinButton = 22;
bool buttonValue = true;
unsigned int refreshRate = 200;

//inizializzazione del display
LiquidCrystal lcd(14, 12, 33, 25, 26, 27);

//SSID e PASS del wifi
#define WIFI_SSID "eir-4927"
#define WIFI_PASSWORD "89574951881699087854"

//indirizzo e porta del server MQTT
#define MQTT_HOST "broker.hivemq.com"
#define MQTT_PORT 1883

//topic di MQTT dell'ESP1
#define MQTT_ESP1_TEMP "temperature"
#define MQTT_ESP1_HUM "humidity"
#define MQTT_ESP1_LUM "brightness"
#define MQTT_ESP1_ALL "esp1/#"
//topic di MQTT dell'ESP2
#define MQTT_ESP2_TEMP "esp2/temperature"
#define MQTT_ESP2_HUM "esp2/humidity"
#define MQTT_ESP2_LUM "esp2/brightness"
#define MQTT_ESP2_ALL "esp2/#"

//robe di telegram
#define BOTtoken "5039029687:AAE1sgr9d_WOaiKACnrFCg6RyBhlpFtysj0"  // your Bot Token (Get from Botfather)
#define CHAT_ID "485901444"

WiFiClientSecure clientSec;
UniversalTelegramBot bot(BOTtoken, clientSec);

int botRequestDelay = 1000;
unsigned long lastTimeBotRan;

//robe di MQTT

WiFiClient espClient;
PubSubClient client(espClient);

//robe del display

byte lampadina[] = {
    B00000,
    B01110,
    B10111,
    B10001,
    B10001,
    B01010,
    B01110,
    B00100
};


byte termometro[8] = {
    B00100,
    B01010,
    B01010,
    B01110,
    B01110,
    B11111,
    B11111,
    B01110
};

byte goccia[8] = {
    B00100,
    B00100,
    B01010,
    B01010,
    B10001,
    B10001,
    B10001,
    B01110,
};

//delay for some refresh 
int delayAfterOn = 100;




void connectToWifi() {
  Serial.println("");
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  #ifdef ESP32
    clientSec.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  #endif
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");  
  }

  Serial.println("");
  Serial.println("Wifi connected");
  Serial.println("Indirizzo IP: ");
  Serial.println(WiFi.localIP());
}

/**
 * Funzione che gestisce il bot telegram
 */
void TelegramBotTaskCode(void* param) {
  for(;;) {
     if (millis() > lastTimeBotRan + botRequestDelay)  {
     int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
     while(numNewMessages) {
       Serial.println("got response");
       handleNewMessages(numNewMessages);
       numNewMessages = bot.getUpdates(bot.last_message_received + 1);
     }
     lastTimeBotRan = millis();
    }
  }
}
TaskHandle_t TelegramBotTask;

String temp[2] = {"----", "----"};
String hum[2] = {"--", "--"};
String lum[2] = {"---", "---"};

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
  
  int i = 1;
  if(buttonValue) {
    i = 0;
  }

  int index = String(topic).indexOf('/');
  String sub = String(topic).substring(index+1);
  String espNum = String(topic).substring(0, index);

  if(espNum == "esp1"){
    i=0;  
  }
  else{
    i=1;
  }
  if (sub == MQTT_ESP1_TEMP) {
    temp[i] = messageTemp;
  }

  if (sub == MQTT_ESP1_HUM) {
    hum[i] = messageTemp;
  }
    
  if (sub == MQTT_ESP1_LUM) {
    lum[i] = messageTemp;
  }
}

void draw(int i) {
  lcd.setCursor(0, 0);
  lcd.print("Room");
  lcd.print(i+1);
  lcd.print("  ");
  temp[i].remove(4);
  lcd.write(1);
  lcd.print(" "+temp[i]);
  lcd.print((char)223);
  lcd.print("C");

  lcd.setCursor(0, 1);
  lcd.write(2);
  lcd.print(" "+hum[i]+"%  ");

  lcd.setCursor(7, 1);
  lcd.write(3);
  lcd.print(" "+lum[i]+"Lum ");  
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP_GATEWAY")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe(MQTT_ESP1_ALL);
      client.subscribe(MQTT_ESP2_ALL);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(1000);
    }
  }
}

void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i=0; i<numNewMessages; i++) {
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Unauthorized user " + chat_id, "");
      continue;
    }
    
    // Print the received message
    String text = bot.messages[i].text;
    Serial.println(text);

    String from_name = bot.messages[i].from_name;

    if (text == "/start") {
      String welcome = "Welcome, " + from_name + ".\n";
      welcome += "Use the following commands to control your outputs.\n\n";
      welcome += "/state1 to request current value for Room1\n";
      welcome += "/state2 to request current value for Room2\n";
      bot.sendMessage(chat_id, welcome, "");
    }
    if (text == "/state1") {
      String msg = "Stanza1\nTemperatura: "+temp[0] +"°C \nUmidità: "+hum[0]+"%\nLuminosità: "+lum[0];
      bot.sendMessage(chat_id, msg,"");
    }
    
    if (text == "/state2") {
      String msg = "Stanza2\nTemperatura: "+temp[1] +"°C \nUmidità: "+hum[1]+"%\nLuminosità: "+lum[1];
      bot.sendMessage(chat_id, msg,"");
    }
  }
}

bool buttonPressed = false;
void IRAM_ATTR buttonPressedInterrupt() {
  buttonPressed = true;
}

void setup() {
  Serial.begin(115200);
  
  //pin del bottone come input
  pinMode(pinButton, INPUT_PULLUP);
  attachInterrupt(pinButton, buttonPressedInterrupt, FALLING);
  
  // Setto il numero di righe e di colonne del display
  lcd.begin(16, 2);
  //Messaggio iniziale
  lcd.print("Welcome...");
  lcd.setCursor(0, 1);
  lcd.print("WIFI Thermostat");
  //creo i caratteri speciali per LCD
  lcd.createChar(1, termometro);
  lcd.createChar(2, goccia);
  lcd.createChar(3, lampadina);
  //Connessione al wifi
  connectToWifi();
  delay(delayAfterOn);

  //connessione a MQTT
  client.setServer(MQTT_HOST, MQTT_PORT);
  client.setCallback(callback);

  draw(0);

  //Istanziamento del task relativo alla gestione del bot telegram sul secondo core del processore
  xTaskCreatePinnedToCore(TelegramBotTaskCode, "TelegramBotTask", 10000, NULL, 1, &TelegramBotTask, 1);
}

int startMillis= 0;
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  int currentMillis = millis();
  
  if( currentMillis - startMillis >= refreshRate){
    if(buttonPressed) {
      buttonValue = !buttonValue;
      buttonPressed = false;
    }
    
    startMillis = currentMillis;
    draw((int)!buttonValue);
  }
}
