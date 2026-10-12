
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// ==================== WIFI ====================
const char* WIFI_SSID = "A56 ";
const char* WIFI_PASSWORD = "12345678";

// ==================== MQTT ====================
const char* MQTT_SERVER = "10.118.160.61";
const int MQTT_PORT = 1883;

const char* TOPIC_TEMPERATURA = "clima/esp32/temperatura";
const char* TOPIC_HUMEDAD = "clima/esp32/humedad";
const char* TOPIC_LUZ = "clima/esp32/luz";

// ==================== SENSORES ====================
#define DHTPIN D4
#define DHTTYPE DHT11


#define LDR_PIN D7

DHT dht(DHTPIN, DHTTYPE);

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

unsigned long ultimoEnvio = 0;
const unsigned long INTERVALO_ENVIO = 2000;

// ==================== CONECTAR WIFI ====================
void conectarWiFi() {
  Serial.print("Conectando a WiFi...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado");
  Serial.print("IP del ESP8266: ");
  Serial.println(WiFi.localIP());
}

// ==================== CONECTAR MQTT ====================
void conectarMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Conectando a Mosquitto...");

    String clientId = "ESP8266-";
    clientId += String(ESP.getChipId(), HEX);

    if (mqttClient.connect(clientId.c_str())) {
      Serial.println(" conectado al broker MQTT");
    } else {
      Serial.print(" error rc=");
      Serial.println(mqttClient.state());
      delay(2000);

      if (WiFi.status() != WL_CONNECTED) {
        conectarWiFi();
      }
    }
  }
}

// ==================== PUBLICAR SENSORES ====================
void publicarSensores() {
  float temperatura = dht.readTemperature();
  float humedad = dht.readHumidity();

  // Leer el valor analógico del LDR: 0 a 1023
  int lecturaLDR = analogRead(D7);

  // Convertir la lectura a porcentaje relativo
  float porcentajeLuz = (lecturaLDR / 1023.0) * 100.0;

  // Verificar el DHT11
  if (isnan(temperatura) || isnan(humedad)) {
    Serial.println("Error leyendo DHT11. Revisa sus conexiones.");
    return;
  }

  // Preparar los datos para MQTT
  String tempTexto = String(temperatura, 1);
  String humedadTexto = String(humedad, 1);
  String luzTexto = String(porcentajeLuz, 1);

  // Publicar los tres valores
  bool okTemp = mqttClient.publish(
    TOPIC_TEMPERATURA, tempTexto.c_str(), true
  );

  bool okHum = mqttClient.publish(
    TOPIC_HUMEDAD, humedadTexto.c_str(), true
  );

  bool okLuz = mqttClient.publish(
    TOPIC_LUZ, luzTexto.c_str(), true
  );

  // Mostrar resultados
    Serial.println("\n------ MEDICIONES ------")

  Serial.print("Temperatura: ");
  Serial.print(temperatura, 1);
  Serial.println(" °C");

  Serial.print("Humedad: ");
  Serial.print(humedad, 1);
  Serial.println(" %");

  Serial.print("Lectura analogica LDR: ");
  Serial.print(lecturaLDR);
  Serial.println(" / 1023");

  Serial.print("Iluminacion relativa: ");
  Serial.print(porcentajeLuz, 1);
  Serial.println(" %");

  Serial.print("Publicacion MQTT: ");
  if (okTemp && okHum && okLuz) {
    Serial.println("correcta");
  } else {
    Serial.println("fallida");
  }

  Serial.println("------------------------");
}

// ==================== CONFIGURACION INICIAL ====================
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\nSistema IoT - NodeMCU ESP8266");

  dht.begin();

  conectarWiFi();

  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setBufferSize(256);

  conectarMQTT();
}

// ==================== BUCLE PRINCIPAL ====================
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
  }

  if (!mqttClient.connected()) {
    conectarMQTT();
  }

  mqttClient.loop();

  if (millis() - ultimoEnvio >= INTERVALO_ENVIO) {
    ultimoEnvio = millis();
    publicarSensores();
  }
}


