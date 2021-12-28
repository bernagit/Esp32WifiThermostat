#include <WiFi.h>
#include <WiFiClient.h>

#include <PubSubClient.h>
#include <DHT.h>
#define DHT11PIN 23 
#define DHTTYPE DHT11
#define LUM_PIN A4

DHT dht(DHT11PIN, DHTTYPE);

const char* ssid = "sexyberna";

const char* password = ""; //WIFI password

const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883; //MQTT broker port

WiFiClient espClient;
PubSubClient client(espClient);

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

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  char message[length];
  for (int i=0; i< length; i++){
    message[i] = (char)payload[i];
  }
  Serial.println(message);
}

  void reconnect() {
    // Loop until we're reconnected
    while (!client.connected()) {
      Serial.print("Attempting MQTT connection...");
      // Create a random client ID
      String clientId = "esp1-";
      clientId += String(random(0xffff), HEX);
      // Attempt to connect
      if (client.connect(clientId.c_str())) {
        Serial.println("Connected!");
       
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
  Serial.begin(115200);
  dht.begin();
  
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  delay(2000);
  
  int h = dht.readHumidity();
  float t = dht.readTemperature();
  int b = analogRead(LUM_PIN);
  
  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }else{   
    Serial.print("BRIGHTNESS = ");
    Serial.println(b);
    client.publish("esp1/temperature",String(t).c_str());
    client.publish("esp1/humidity",String(h).c_str());
    client.publish("esp1/brightness", String(b).c_str());  
  }

}
