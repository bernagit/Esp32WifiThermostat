#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <DHT.h>
#include "Client.h"
#include "esp_wifi.h"

DHT dht(DHTPIN, DHTTYPE);

int analogInput = 0;
int vRefInput = 0;
float Volt_FOTO;
float Volt_VREF;
float Resistenza;
float temp;
int luce;

float setTemp = 0;

WiFiClient espClient;
PubSubClient client(espClient);

void connect_WiFi() {

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while(WiFi.status() != WL_CONNECTED) {
    delay(DELAY_WIFI_TRY);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());  
  
}


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]");

  char message[length];

  for(int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }

  Serial.print(message);

  setTemp = String(message).toFloat();
}

bool mqtt_connect() {
  while(!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    String clientID = "esp1-";
    clientID += String(random(0xffff), HEX);

    if(client.connect(clientID.c_str())) {
      Serial.println("Connected to MQTT broker!");
      
    } else {
      Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        delay(DELAY_AFTER_SETUP);
    }
  }
}

void setWiFiPowerSavingMode(){
    //funzione che rallenta la frequenza di arrivo dei messaggi Beacon dall'access point a cui è collegato
    //Minimum modem power saving. In this mode, station wakes up to receive beacon every DTIM period (DOCUM UFF ESPRESSIF)
    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
}

void setCpuFrequency(int freq){
  setCpuFrequencyMhz(freq);
 
  Serial.print("CPU Freq: ");
  Serial.println(getCpuFrequencyMhz());
}

int volt_to_lux(int v) {
  Volt_FOTO = float(v) * (VREF / float(4095));
  Serial.print(" ADC = ");
  Serial.print(v);
  Serial.print(" V_FOTO [mV] = ");
  Serial.print(Volt_FOTO);

  Resistenza = R * Volt_FOTO / (VREF - Volt_FOTO);
  if (Resistenza >= 999999.0) {
    Resistenza = 999999.0; 
  }

  Serial.print(" Resistenza[Ohm]=");
  Serial.print(Resistenza);
 
  temp = log10(Resistenza/100000.0);
  temp = -(1/0.72)*temp;
  luce = (int)pow(10, temp);

  if(luce > 9999.0) {
    luce = 9999.0;
  }
  return luce;  
}

void setup() {
  Serial.begin(115200);
  
  pinMode(FOTO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  
  connect_WiFi();
  client.setServer(MQTT_HOST, MQTT_PORT);
  client.setCallback(callback);

  //funzioni per il risparmio energetico
  setCpuFrequency(80);  //abbasso la velocità del clock da 240MHz a 80MHz
  setWiFiPowerSavingMode();
  
  dht.begin();
  
}

void loop() {
 
  if (!client.connected()) {
    mqtt_connect();
    client.subscribe("progettoEle/Room2/setT");
  }
  
  client.loop();
  
  int humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  analogInput = analogRead(FOTO_PIN);

  luce = volt_to_lux(analogInput);
  Serial.print(" Luce[Lux]=");
  Serial.print(luce);
  Serial.println(" ");

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println(F("Lettura da DHT fallita!"));
    return;
  }else{  
    client.publish("progettoEle/Room2/temperature",String(temperature).c_str());
    client.publish("progettoEle/Room2/humidity",String(humidity).c_str());
    client.publish("progettoEle/Room2/brightness", String(luce).c_str());
  }

  if(temperature <= setTemp) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
  
  delay(SEND_TIME);
}
