/*

Creado por @nobrayan
WeBT (Bluetooth Classic)
en conjunto con webt 1.0.0-beta.apk (o superior)

*/

#include <Arduino.h>
#include <Preferences.h>
#include <BluetoothSerial.h>
#include <WiFi.h>
#include <WebServer.h>

// [WeBT] es el prefijo que se mostrara en el nombre del dispositivo
// Si el prefijo se cambia, tambien se debe cambiar en la aplicacion movil
const String prefijobt = "[WeBT]"; // Prefijo del nombre del dispositivo Bluetooth
const String btname = "Web Viewer"; // Nombre del dispositivo Bluetooth

String ssid = "";  // SSID
String password = "";  // PASS
String guardar_red = "false";  // Guardar red
String web_ip = "none";  // IP
String estado_web = "false";

Preferences preferences; // Crea un objeto de almacenamiento

BluetoothSerial SerialBT;
int conexion_bt = 0; // Proceso de conexion Bluetooth Classic

WebServer server(80); // Crea un servidor web en el puerto 80

// Funciones
void bt_begin();
void bluetooth();
void wifi_begin(String ssid, String password);
void handleRoot();

/*========================================================================================================*/

void setup() {
  // Iniciar la comunicación serial para monitoreo
  Serial.begin(115200);

  // Iniciar el almacenamiento de preferencias
  preferences.begin("config_wifi", true); // Iniciar el almacenamiento
  ssid = preferences.getString("ssid", ""); // Leer el SSID
  password = preferences.getString("password", ""); // Leer password
  preferences.end(); // Finalizar el almacenamiento
  
  // Iniciar el Wi-Fi, si hay datos guardados
  if ((ssid != "") && (password != "")) wifi_begin(ssid, password);

  // Iniciar Bluetooth Classic
  bt_begin();
}

void loop() {
  bluetooth();
  if (estado_web != "false") server.handleClient();
  delay(2500);
}

/*========================================================================================================*/

void bt_begin() {
  SerialBT.end();
  delay(2000);

  if (!SerialBT.begin(prefijobt + " " + btname)) Serial.println("No se pudo iniciar el Bluetooth...");
  else Serial.println("Dispositivo Bluetooth iniciado...");
}

// Funcion Bluetooth
void bluetooth() {
  if (SerialBT.connected()) {
    // Si la conexion es exitosa, enviar peticion
    if (conexion_bt == 0) {
      SerialBT.println("&ESP.PASS&");
      conexion_bt = 1;
    }

    // Si no recibe la peticion correcta, reiniciar la conexion
    else if (conexion_bt == 1) {
      if (SerialBT.available()) {
        String receivedData = SerialBT.readString();
        if (receivedData == "%ESP.ENTER%") {
          Serial.println("Dispositivo conectado correctamente...");
          conexion_bt = 2;
        }
        else {
          conexion_bt = 0;
          bt_begin();
        } 
      }
    }

    // Conexion exitosa, enviar datos
    else if (conexion_bt == 2) {
      if (estado_web == "true") SerialBT.println("/" + ssid + "&" + web_ip + "&true&error#"); // Envio de datos al dispositivo movil
      else if (estado_web == "false") SerialBT.println("/ESP.OK&IP&false&error#"); // Si el estado de la web es falso, enviar un mensaje de "simple" al dispositivo movil
      
      // Si hay datos disponibles, recibir y procesar
      if (SerialBT.available()) {
        String msg = SerialBT.readString();
        
        // Filtrar y separar los datos recibidos
        if ((msg[0] != '%') || (msg[msg.length()-1] != '%')) Serial.println("Datos BT descartados...");
        else {
          estado_web = "false";
          // Separar los datos recibidos
          Serial.println("Datos: " + msg);
          String separacion_msg[4];
          int x = 0;
          for (int i=0; i<msg.length(); i++) {
            if (msg[i] == '%');
            else if (msg[i] == '&') x++;
            else separacion_msg[x] = separacion_msg[x] + msg[i];
          }

          // Asignar los datos recibidos
          ssid = separacion_msg[1];
          password = separacion_msg[2];
          guardar_red = separacion_msg[3];

          // Guardar los datos en la memoria
          preferences.begin("config_wifi", false); // Iniciar el almacenamiento
          if (guardar_red == "true") {
            Serial.println("Guardando red en memoria...");
            preferences.putString("ssid", ssid); // Guardar el SSID
            preferences.putString("password", password); // Guardar la password
          }
          else {
            // Limpiar la memoria, si no se desea guardar la red
            Serial.println("Borrando red de memoria...");
            preferences.clear(); // Limpiar el almacenamiento
          }
          preferences.end(); // Finalizar el almacenamiento
          
          // Conectar a la red WiFi
          wifi_begin(ssid, password);
        }
      }
    }
  }
  else {
    conexion_bt = 0;
    Serial.println("Esperando reconexion...");
  }
}

// Iniciar Wi-Fi
void wifi_begin(String ssid, String password) {
  // Desconectar de la red Wi-Fi actual
  WiFi.disconnect();
  delay(500);

  // Conectar a la red Wi-Fi
  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.print("Conectando a WiFi");

  // Tiempo de espera
  int intentos = 0;
  int max_intentos = 5;
  while (WiFi.status() != WL_CONNECTED && intentos < max_intentos) {
      delay(500);
      Serial.print(".");
      intentos++;
  }
  Serial.println("");

  if (WiFi.status() != WL_CONNECTED) {
      // Si no se conecta despues de los intentos, mostrar un mensaje de error
      // ERROR 503: No se pudo conectar
      estado_web = "false";
      SerialBT.println("/ESP.OK&IP&false&503#");
      Serial.println("Error al conectar a WiFi...");
      return; // Salir de la función
  }

  Serial.println("");

  // Conexion exitosa, guardar IP
  Serial.print("Dirección IP: ");
  web_ip = WiFi.localIP().toString();
  Serial.println(web_ip);

  // Configurar las rutas del servidor web
  server.on("/", HTTP_GET, handleRoot);
  server.on("/submit", HTTP_POST, handleRoot);

  // Iniciar el servidor
  server.begin();
  estado_web = "true";
}

// Pagina web
void handleRoot() {
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