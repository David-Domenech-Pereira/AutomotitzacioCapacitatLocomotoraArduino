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
const char* serverUrl = "http://tudominio.com/tu_ruta_de_recepcion"; // URL de la web para recibir los datos (cambia esto a la URL correcta)
dades_t valores[MAX_VALORS];
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
}

void loop() {
  // Leer los valores x, y, z del acelerómetro
 
    float accelX, accelY, accelZ; // Valores del acelerómetro (m/s²)
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(accelX, accelY, accelZ);
    dades_t escriure;
    escriure.timestamp = millis();
    escriure.x = accelX;
    escriure.y = accelY;
    escriure.z = accelZ;
      // Imprimim els valors en el monitor serial
    Serial.print("X: "); Serial.print(accelX);
    Serial.print("\tY: "); Serial.print(accelY);
    Serial.print("\tZ: "); Serial.println(accelZ);
    //inserim el valor al final
    valores[last_valor] = escriure;
    last_valor ++;
    delay(100); //esperem 100 ms
    }else{
      Serial.println("Acceleration not available");
  }

  // Verificar si hay conexión WiFi para enviar los datos
  if (last_valor >= MAX_VALORS&&WiFi.status() == WL_CONNECTED) {
    // Leer los datos del archivo y enviarlos al servidor
    
    sendDataToWeb();
    last_valor = 0; //tornem a començar
  }
}


void sendDataToWeb() {
  //TODO Falta autenticar-se al servidor web
    // Leer los datos del archivo
    String dataToSend = "{\"sensor\":1,\"data\":[";
    for(int i = 0; i < MAX_VALORS; i++){
      
      dataToSend += "{\"timestamp\":"+String(valores[i].timestamp)+",\"values\":["+String(valores[i].x)+","+String(valores[i].y)+","+String(valores[i].z)+"]},";
    }

    //quitamos la ultima coma
    dataToSend = dataToSend.substring(0,dataToSend.length()-1);
    dataToSend += "]}";

    Serial.println(dataToSend);
    // Crear una instancia de cliente HTTP
    WiFiClient client;
  
    // Realizar la solicitud HTTP POST
    if (client.connect(serverUrl, 80)) {
      client.println("POST " + String(serverUrl) + " HTTP/1.1");
      client.println("Host: " + String(serverUrl));
      client.println("Content-Type: application/json");
      client.println("Content-Length: " + String(dataToSend.length()));
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
