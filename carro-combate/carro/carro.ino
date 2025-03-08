/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*

  Creado por @nobrayan
  en conjunto con mando_espnow

*/
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <esp_now.h>
#include <WiFi.h>

/////////////////////////////////////////

// MOTOR POTENCIA
#define M_POT 15

// Pines MOTOR 1
#define P_MA 16
#define M1_I1 4
#define M1_I2 5

// Pines MOTOR 2
#define P_MB 17
#define M2_I1 6
#define M2_I2 7

// ESPNOW
int conexion = 0;
String receivedMessage = "";
bool pingReceived = 0;

String movimiento = "libre";
unsigned int velocidad = 0;
unsigned int potencia = 0;
float VRX = 0;
float VRY = 0;
int freno = 0;

/////////////////////////////////////////

void setup() {
  Serial.begin(115200);

  pinMode(M1_I1, OUTPUT);
  pinMode(M1_I2, OUTPUT);
  pinMode(M2_I1, OUTPUT);
  pinMode(M2_I2, OUTPUT);
  digitalWrite(M1_I1, 0);
  digitalWrite(M1_I2, 0);
  digitalWrite(M2_I1, 0);
  digitalWrite(M2_I2, 0);
  analogWrite(M_POT, 0);
  
  
  // Inicializar el WiFi en modo estación
  WiFi.mode(WIFI_STA);
  Serial.println("Modo WiFi configurado en STA");

  // Inicializar ESPNOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error al inicializar ESPNOW");
    return;
  }

  // Registrar el callback de recepción
  esp_now_register_recv_cb(onDataReceived);

  // Esperar a recibir el "Ping" y enviar el "Pong" antes de continuar
  while (!pingReceived) {
    delay(250);  // Esperar hasta que el "Ping" sea recibido y el "Pong" enviado
  }
  
}

/////////////////////////////////////////

// Callback para la recepción de datos
void onDataReceived(const esp_now_recv_info *info, const uint8_t *data, int len) {
  // Convertir los datos recibidos a String con longitud correcta
  String message((char *)data, len);  
  //Serial.println("Mensaje recibido: " + message);

  // Verificar si el mensaje es "Ping"
  if (message == "ping") {
    Serial.println("Ping recibido, enviando Pong...");
    pingReceived = true;  // Marcar que se ha recibido el "Ping"

    // Configurar el peer con la dirección del remitente
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, info->src_addr, 6);
    peerInfo.channel = 0; // Usar el canal actual
    peerInfo.encrypt = false;

    // Verificar si el peer ya está agregado
    if (!esp_now_is_peer_exist(info->src_addr)) {
      if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Error al agregar peer para responder con Pong");
        return;
      }
    }

    // Enviar el mensaje "Pong" de vuelta al remitente
    if (esp_now_send(info->src_addr, (const uint8_t *)"pong", 4) == ESP_OK) {
      Serial.println("Pong enviado exitosamente");
    } else {
      Serial.println("Error al enviar Pong");
    }
  }

  receivedMessage = message;
  int x = 0;
  String separacion[5];
  for (int i = 0; i < receivedMessage.length(); i++) {
    if (receivedMessage[i] == '&') { x++; }
    else { separacion[x] = separacion[x] + receivedMessage[i]; }
  }
  potencia = separacion[0].toInt();
  velocidad = separacion[1].toInt();
  VRX = separacion[2].toFloat();
  VRY = separacion[3].toFloat();
  freno = separacion[4].toInt();
  conexion = 0;
}

// Función MOTOR
void MoVM(int p, int vx, int vy, int f) {
  int direccion = abs(vx) - abs(vy);
  int inv_direccion = abs(vx) + abs(vy);
  if (direccion < 0 ) { direccion = 0; }
  if (inv_direccion > 255) { inv_direccion = 255; }

  if (direccion > p) { direccion = p; }
  if (inv_direccion > p) { inv_direccion = p; }
  if (abs(vx) > p) { vx = p*(vx/abs(vx)); }
  if (abs(vy) > p) { vy = p*(vy/abs(vy)); }

  Serial.print("VX: "); Serial.print(vx);
  Serial.print(" || VY: "); Serial.println(vy);
  Serial.print("Direccion: "); Serial.print(direccion);
  Serial.print(" || Inv_direccion: "); Serial.println(inv_direccion);

  if (f == 0) {
    analogWrite(P_MA, 255);
    analogWrite(P_MB, 255);
    digitalWrite(M1_I1, 1);
    digitalWrite(M1_I2, 1);
    digitalWrite(M2_I1, 1);
    digitalWrite(M2_I2, 1);
  }
  else if (vx >= 0) {
    if (vy > 0) {
      analogWrite(P_MA, inv_direccion);
      analogWrite(P_MB, direccion);
      digitalWrite(M1_I1, 1);
      digitalWrite(M1_I2, 0);
      digitalWrite(M2_I1, 1);
      digitalWrite(M2_I2, 0);
    }
    else if (vy < 0) {
      analogWrite(P_MA, direccion);
      analogWrite(P_MB, inv_direccion);
      digitalWrite(M1_I1, 1);
      digitalWrite(M1_I2, 0);
      digitalWrite(M2_I1, 1);
      digitalWrite(M2_I2, 0);
    }
    else {
      analogWrite(P_MA, abs(vx));
      analogWrite(P_MB, abs(vx));
      digitalWrite(M1_I1, 1);
      digitalWrite(M1_I2, 0);
      digitalWrite(M2_I1, 1);
      digitalWrite(M2_I2, 0);
    }
  }
  else if (vx < 0) {
    if (vy > 0) {
      analogWrite(P_MA, inv_direccion);
      analogWrite(P_MB, direccion);
      digitalWrite(M1_I1, 0);
      digitalWrite(M1_I2, 1);
      digitalWrite(M2_I1, 0);
      digitalWrite(M2_I2, 1);
    }
    else if (vy < 0) {
      analogWrite(P_MA, direccion);
      analogWrite(P_MB, inv_direccion);
      digitalWrite(M1_I1, 0);
      digitalWrite(M1_I2, 1);
      digitalWrite(M2_I1, 0);
      digitalWrite(M2_I2, 1);
    }
    else {
      analogWrite(P_MA, abs(vx));
      analogWrite(P_MB, abs(vx));
      digitalWrite(M1_I1, 0);
      digitalWrite(M1_I2, 1);
      digitalWrite(M2_I1, 0);
      digitalWrite(M2_I2, 1);
    }
  }
  else {
    analogWrite(P_MA, 0);
    analogWrite(P_MB, 0);
    digitalWrite(M1_I1, 0);
    digitalWrite(M1_I2, 0);
    digitalWrite(M2_I1, 0);
    digitalWrite(M2_I2, 0);
  }
}

/////////////////////////////////////////

void loop() {
  if (conexion >= 5) {
    Serial.println("Sin conexion");
    analogWrite(M_POT, 0);
    MoVM(0, 0, 0, 1);
    conexion = 6;
  }
  else {
    //String mensaje = String(potencia) + "&" + String(velocidad) + "&" + String(VRX) + "&" + String(VRY) + "&" + String(freno);
    //Serial.println(mensaje);
    analogWrite(M_POT, potencia);
    MoVM(velocidad, VRX, VRY, freno);
  }

  if (conexion < 5) {
    conexion++;
  }
  delay(50);
}
