#include <Wire.h>
#include <EEPROM.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include <ArduinoJson.h> // Instala esta librería desde el Administrador de Librerías de Arduino
#include <Arduino_LSM6DS3.h>

#define MAX_VALORS 100 // Quan arribem a aquest número de valors els enviarà al servidor

typedef struct{
  float x;
  float y;
  float z;
  float timestamp;
}dades_t;

const char* ssid = "URV"; // Nombre de  red WiFi
const char* password = "UrbotsVibes"; // Contraseña de  red WiFi
const char* serverUrl = "smarttechnologiesurv.000webhostapp.com"; // URL de la web para recibir los datos (cambia esto a la URL correcta)
const char* worldTimeApiUrl = "http://worldtimeapi.org/api/timezone/Etc/UTC"; // URL de worldtimeapi.org

dades_t valores[MAX_VALORS];
long timestamp;
int last_valor = 0;
void setup() {
  Serial.begin(9600);
  Wire.begin();
  
  // Conexión a la red WiFi
  Serial.print("Conectando a la red WiFi...");
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("No se encuentra el shield WiFi");
    while (true);
  }
  
  while (WiFi.begin(ssid, password) != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nConectado a la red WiFi!");
   if (!IMU.begin()) {
    Serial.println("Error al inicializar el acelerómetro.");
    while (1);
  }
    Serial.print("Connected to Wi-Fi. IP address: ");
  Serial.println(WiFi.localIP());

  //com no podem saber la hora, agafarem quina hora és a l'encendre's
  timestamp = getCurrentTime();
}
String paramGetRequest(const char * url, const char * endpoint, String param){
  WiFiClient client;
  if (client.connect(url, 80)) {
    Serial.println("Conectado al servidor.");
    client.print("GET "+((String)endpoint)+" HTTP/1.1\r\n");
    client.print("Host: "+(String)url+"\r\n");
    client.print("Connection: close\r\n\r\n");
    while (client.connected() && !client.available()) {
      delay(10);
    }
  } else {
    //printamos código de error
    Serial.print("Error al conectar con el servidor de tiempo. Código de error: ");
    Serial.println(client.connect(url, 80));
    Serial.println("Error al conectar con el servidor de tiempo.");
  }

   // Skip HTTP headers
  bool headerPassed = false;
  while (client.connected()) {
    if (client.available()) {
      if (!headerPassed) {
        if (client.readStringUntil('\n').equals("\r")) {
          headerPassed = true;
        }
      } else {
        break; // Stop reading once the header is skipped
      }
    }
  }

  // Read the response body into a String
  String response;
  while (client.available()) {
    response += client.readStringUntil('\r');
  }
  Serial.println(response);
  // Leer la respuesta del servidor
  DynamicJsonDocument json(1024);
  DeserializationError error = deserializeJson(json, response);
  if (error) {
        
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
      }

  // Obtener el tiempo UTC en formato UNIX timestamp
  return json[param];

}
long getCurrentTime(){
 

  // Obtener el tiempo UTC en formato UNIX timestamp
  String str_utcTime = paramGetRequest("worldtimeapi.org","/api/timezone/Etc/UTC","unixtime");

  // Imprimir el tiempo UTC en segundos desde el 1 de enero de 1970
  Serial.print("UTC Timestamp: ");
  Serial.println(str_utcTime);
  //convertimos utcTime a long
  long utcTimeLong = str_utcTime.toInt();
  return utcTimeLong;
}
void loop() {
  // Leer los valores x, y, z del acelerómetro
 
    float accelX, accelY, accelZ; // Valores del acelerómetro (m/s²)
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(accelX, accelY, accelZ);
    dades_t escriure;
    //quiero coger los segundos, UTC

    escriure.timestamp = timestamp+millis()/1000;
    escriure.x = accelX;
    escriure.y = accelY;
    escriure.z = accelZ;
  
    //inserim el valor al final
    valores[last_valor] = escriure;
    last_valor ++;
   
    }else{
      Serial.println("Acceleration not available");
  }
   delay(100); //esperem 100 ms
  // Verificar si hay conexión WiFi para enviar los datos
  if (last_valor >= MAX_VALORS&&WiFi.status() == WL_CONNECTED) {
    // Leer los datos del archivo y enviarlos al servidor
    
    sendDataToWeb();
    last_valor = 0; //tornem a començar
  }
}


void sendDataToWeb() {
  const char* token;
     // Crear una instancia de cliente HTTP
    WiFiClient client;
  //hacemos un GET request para autenticar-nos
    String dataAuth="{\"publicKey\":\"TOKENDEPROVA\"}";
    //Realizamos solicitud HTTP GET a serverUrl/api/authorization.php
    if (client.connect(serverUrl, 80)) {
      client.println("GET /api/authorization.php HTTP/1.1");
      client.println("Host: " + String(serverUrl));
      client.println("Content-Type: application/json");
      client.println("Content-Length: " + String(dataAuth.length()));
      client.println();
      client.println(dataAuth);
      client.println();
      Serial.println("Datos enviados!");
       // Skip HTTP headers
      bool headerPassed = false;
      while (client.connected()) {
        if (client.available()) {
          if (!headerPassed) {
            if (client.readStringUntil('\n').equals("\r")) {
              headerPassed = true;
            }
          } else {
            break; // Stop reading once the header is skipped
          }
        }
      }

      // Read the response body into a String
      String response;
      while (client.available()) {
        response += client.readStringUntil('\r');
      }
      Serial.println("Response=>"+response);
      // Leer la respuesta del servidor
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, response);
      // Obtener el valor del token
      // Check if the "token" key exists in the JSON.
      if(doc.containsKey("status")){
        if(doc["status"]==false){
          Serial.println("ERROR");
          return;
        }
      }
      if (doc.containsKey("token")) {
          // Get a reference to the "token" value.
          const JsonVariant& tokenVariant = doc["token"];

          // Check if the value is of type `const char*`.
          if (tokenVariant.is<const char*>()) {
              // Access the token value as a `const char*`.
              token = tokenVariant.as<const char*>();

              // Now you can use the `token` variable as needed.
              Serial.println("Token: " + String(token)); // Print the token as a String.
          } else {
              // Handle the case when the "token" value is not of type `const char*`.
              Serial.println("Token value is not a const char*.");
          }
      } else {
          // Handle the case when the "token" key does not exist in the JSON.

          Serial.println("Token key not found in JSON.");
      }
      
    } else {

      Serial.println("Error al conectar con el servidor de autentiacion.");
      return;
    }
    // Leer los datos del archivo
    String dataToSend = "{\"sensor\":1,\"data\":[";
    for(int i = 0; i < MAX_VALORS; i++){
      
      dataToSend += "{\"timestamp\":"+String(valores[i].timestamp)+",\"values\":["+String(valores[i].x)+","+String(valores[i].y)+","+String(valores[i].z)+"]},";
    }

    //quitamos la ultima coma
    dataToSend = dataToSend.substring(0,dataToSend.length()-1);
    dataToSend += "]}";

    Serial.println(dataToSend);
 
  
    // Realizar la solicitud HTTP POST
    if (client.connect(serverUrl, 80)) {
      client.println("POST " + String(serverUrl)+"/api/sensorData.php" + " HTTP/1.1");
      client.println("Host: " + String(serverUrl));
      client.println("Content-Type: application/json");
      client.println("Content-Length: " + String(dataToSend.length()));
      client.println("Authorization: " + String(token));
      client.println();
      client.println(dataToSend);
      client.println();
      Serial.println("Datos enviados!");
      
    } else {
      Serial.println("Error al conectar con el servidor.");
    }
  
    // Cerrar la conexión
    client.stop();
 
}
