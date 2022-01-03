#include <WiFi.h>
#include <WiFiClient.h>

#include <PubSubClient.h>
#include <DHT.h>
#include "Client.h"
#include "driver/adc.h"

DHT dht(DHTPIN, DHTTYPE);

int analogInput = 0;
int vRefInput = 0;

float Volt_FOTO;
float Volt_VREF;
float Resistenza;
float temp;
int Luce;


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
}



void mqtt_connect() {

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


int volt_to_lux(int v) {

  Volt_FOTO = float(v) * (VREF / float(4095));
  Serial.print(" ADC = ");
  Serial.print(v);
  Serial.print(" V_FOTO [mV] = ");
  Serial.print(Volt_FOTO);

  Resistenza = R * Volt_FOTO / (VREF - Volt_FOTO);
  if (Resistenza >= 999999.0) {
    Resistenza = 999999.0; // controllo che il monitor non vda in ovf
  }

  Serial.print(" Resistenza[Ohm]=");
  Serial.print(Resistenza);
 
  temp = log10(Resistenza/100000.0);
  temp = -(1/0.72)*temp;
  Luce = (int)pow(10, temp);

  if(Luce > 9999.0) {
    Luce = 9999.0;
  }

  return Luce;
  
  
}

void setup() {
  
  Serial.begin(115200);
  pinMode(FOTO_PIN, INPUT);

  connect_WiFi();
  client.setServer(MQTT_HOST, MQTT_PORT);
  client.setCallback(callback);

  dht.begin();
}

void disableWiFi(){
    adc_power_off();
    WiFi.disconnect(true);  // Disconnect from the network
    WiFi.mode(WIFI_OFF);    // Switch WiFi off
    Serial2.println("");
    Serial2.println("WiFi disconnected!");
}

void loop() {
 
  if (!client.connected()) {
    mqtt_connect();
  }
  
  client.loop();
  
  int humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  analogInput = analogRead(FOTO_PIN);

  Luce = volt_to_lux(analogInput);
  Serial.print(" Luce[Lux]=");
  Serial.print(Luce);
  Serial.println(" ");

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }else{  
    client.publish("aaabbbccc/Room2/temperature",String(temperature).c_str());
    client.publish("aaabbbccc/Room2/humidity",String(humidity).c_str());
    client.publish("aaabbbccc/Room2/brightness", String(Luce).c_str());  
  }
  Serial.println(WiFi.status());
  WiFi.setSleep(true);
  //disableWiFi();
  Serial.println(WiFi.status());
  delay(5000);
}
