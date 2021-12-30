#include <WiFi.h>
#include <WiFiClient.h>

#include <PubSubClient.h>
#include <DHT.h>

#define DHTPIN 23
#define DHTTYPE DHT11
#define FOTO_PIN 32
#define VREF_PIN 35

#define r0 10000.0

DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "Sitecom8110b2";
const char* password = "16266773";
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883; 


int analogInput = 0;
int vRefInput = 0;

float Volt_FOTO;
float Volt_VREF;
float coeff = -1.25;
float Resistenza;
float temp;
int Luce;


WiFiClient espClient;
PubSubClient client(espClient);

void connect_WiFi() {

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
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
        delay(5000);
    }
  }
}


int volt_to_lux(int v1, int v2) {

  Volt_FOTO = 500.0 + (float)(2000*(v1-493))/(2990-493);
  Serial.print(" ADC = ");
  Serial.print(v1);
  Serial.print(" V_FOTO [mV] = ");
  Serial.print(Volt_FOTO);

  Volt_VREF = 500.0 + (float)(2000*(v2-493))/(2990-493); 
  Serial.print(" ADC = ");
  Serial.print(v2);
  Serial.print(" V_VREF [mV] = ");
  Serial.print(Volt_VREF);

  temp = Volt_VREF - Volt_FOTO;
  if (temp <= 0.0) {
    temp = 0.001; // controllo che il denominatore non sia zero o negativo e saturo a 1mV (livello di rumore)
  }
  Resistenza = r0*Volt_FOTO/temp;
  
  if (Resistenza >= 999999.0) {
    Resistenza = 999999.0; // controllo che il monitor non vda in ovf
  }

  Serial.print(" Resistenza[Ohm]=");
  Serial.print(Resistenza);
 
  temp = log10(Resistenza/100000.0);
  temp = coeff*temp;
  Luce = (int)pow(10, temp);

  return Luce;
  
  
}

void setup() {
  
  Serial.begin(115200);
  pinMode(FOTO_PIN, INPUT);
  pinMode(VREF_PIN, INPUT);

  connect_WiFi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  dht.begin();
}

void loop() {
 

  if (!client.connected()) {
    mqtt_connect();
  }
  
  client.loop();
  delay(2000);
  
  
  int humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  analogInput = analogRead(FOTO_PIN);
  vRefInput = analogRead(VREF_PIN);

  Luce = volt_to_lux(analogInput, vRefInput);
  Serial.print(" Luce[Lux]=");
  Serial.print(Luce);
  Serial.println(" ");
  delay(1000);


  if (isnan(humidity) || isnan(temperature)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }else{  
    delay(500);

    client.publish("aaabbbccc/Room1/temperature",String(temperature).c_str());
    client.publish("aaabbbccc/Room1/humidity",String(humidity).c_str());
    client.publish("aaabbbccc/Room1/brightness", String(Luce).c_str());  
  }
  
  
}
