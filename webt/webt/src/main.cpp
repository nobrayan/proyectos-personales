#include <Arduino.h>
#include <BluetoothSerial.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>

const String btname = "WeBT"; // Nombre del dispositivo Bluetooth
const String mdnsname = "esp32"; // Nombre de la red mDNS (http://esp32.local)

String ssid = "none";  // SSID
String password = "none";  // PASS
String guardar_red = "false";  // Guardar red
String web_ip = "none";  // IP
String estado_web = "false";

BluetoothSerial SerialBT;
int conexion_bt = 0;

void bt_begin();
void bluetooth();
void wifi_begin(String ssid, String password);
void handleRoot();

WebServer server(80);

void setup() {
  // Iniciar la comunicación serial para monitoreo
  Serial.begin(115200);
  
  bt_begin();
}

void loop() {
  bluetooth();
  server.handleClient();
  delay(2500);
}

void bt_begin() {
  SerialBT.end();
  delay(2000);
  // Iniciar el Bluetooth
  // [ESP] es el prefijo que se mostrará en el nombre del dispositivo
  // Si el prefijo se cambia, también se debe cambiar en la aplicación móvil
  if (!SerialBT.begin("[ESP] " + btname)) Serial.println("¡No se pudo iniciar el Bluetooth!");
  else Serial.println("El Bluetooth está encendido y esperando conexión...");
}

void bluetooth() {
  if (SerialBT.connected()) {
    // Si la conexión es exitosa, enviar peticion
    if (conexion_bt == 0) {
      SerialBT.println("&ESP.PASS&");
      conexion_bt = 1;
    }
    // Si no recibe la peticion correcta, reiniciar la conexión
    else if (conexion_bt == 1) {
      if (SerialBT.available()) {
        String receivedData = SerialBT.readString();
        if (receivedData == "%ESP.ENTER%") conexion_bt = 2;
        else {
          conexion_bt = 0;
          bt_begin();
        } 
      }
    }
    // Conexión exitosa, enviar datos
    else if (conexion_bt == 2) {
      if (estado_web == "true") SerialBT.println("/ESP.OK&" + web_ip + "&true#");
      else if (estado_web == "false") SerialBT.println("/ESP.OK&IP&false#");

      if (SerialBT.available()) {
        String msg = SerialBT.readString();
        
        // Filtrar y separar los datos recibidos
        if ((msg[0] != '%') && (msg[msg.length()-1] != '%')) Serial.println("Datos BT descartados...");
        else {
          Serial.println("Datos: " + msg);
          String separacion_msg[4];
          int x = 0;
          for (int i=0; i<msg.length(); i++) {
            if (msg[i] == '%');
            else if (msg[i] == '&') x++;
            else separacion_msg[x] = separacion_msg[x] + msg[i];
          }
          ssid = separacion_msg[1];
          password = separacion_msg[2];
          guardar_red = separacion_msg[3];
          wifi_begin(ssid, password);
        }
      }
    }
  }
  else {
    conexion_bt = 0;
    Serial.println("Esperando reconexión...");
  }
}

void wifi_begin(String ssid, String password) {
  // Desconectar de la red Wi-Fi actual
  WiFi.disconnect();
  delay(500);

  // Conectar a la red Wi-Fi
  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  
  // Conexión exitosa, guardar red
  Serial.print("Dirección IP: ");
  web_ip = WiFi.localIP().toString();
  Serial.println(web_ip);

  // Configurar mDNS para usar 'esp32.local'
  if (!MDNS.begin(mdnsname)) {
    Serial.println("Error al iniciar mDNS");
    while (1);
  }
  Serial.println("mDNS iniciado: esp32.local");

  // Configurar las rutas del servidor web
  server.on("/", HTTP_GET, handleRoot);
  server.on("/submit", HTTP_POST, handleRoot);

  // Iniciar el servidor
  server.begin();
  estado_web = "true";
}

void handleRoot() {
  // Generar la página HTML con formulario, CSS y JS
  String html = "<html><head><title>Formulario ESP32</title>";
  html += "<style>";
  html += "body { font-family: 'Arial', sans-serif; background-color: #f4f4f4; margin: 0; padding: 0; display: flex; justify-content: center; align-items: center; height: 100vh; }";
  html += "form { background-color: #fff; padding: 30px; border-radius: 10px; box-shadow: 0px 4px 10px rgba(0, 0, 0, 0.1); width: 100%; max-width: 400px; }";
  html += "h2 { text-align: center; color: #333; font-size: 24px; margin-bottom: 20px; }";
  html += "label { color: #555; font-size: 16px; margin-bottom: 8px; display: block; }";
  html += "input[type=text] { width: 100%; padding: 12px; border-radius: 8px; border: 1px solid #ddd; font-size: 16px; margin-bottom: 20px; transition: all 0.3s ease; }";
  html += "input[type=text]:focus { border-color: #4CAF50; box-shadow: 0 0 8px rgba(76, 175, 80, 0.6); outline: none; }";
  html += "input[type=submit] { background-color: #4CAF50; color: white; padding: 14px 20px; border: none; border-radius: 8px; width: 100%; font-size: 16px; cursor: pointer; transition: background-color 0.3s ease; }";
  html += "input[type=submit]:hover { background-color: #45a049; }";
  html += "</style></head><body>";
  html += "<form action='/submit' method='POST'>";
  html += "<h2>Formulario de Dirección IP</h2>";
  html += "<label for='ip'>Ingrese la dirección IP:</label>";
  html += "<input type='text' id='ip' name='ip' required placeholder='Ejemplo: 192.168.1.100'>";
  html += "<input type='submit' value='Enviar'>";
  html += "</form>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}