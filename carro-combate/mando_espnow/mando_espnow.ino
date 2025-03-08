/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*

  Creado por @nobrayan
  Mando bajo el protocolo ESPNOW

*/
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <esp_now.h>
#include <WiFi.h>

/////////////////////////////////////////

// Pines Joystick
#define pin_VRX 36
#define pin_VRY 39
#define pin_SW 27

// Pines botones
#define pin_Freno 18
#define pin_Sprint 19
#define pin_Potencia 21
#define pin_Inversion 5

// Pin para el LED
#define pin_Led 2
#define pin_LedSprint 4
#define pin_LedArma 15
#define pin_Ledinversion 26

// Dirección MAC del receptor
#define RECEIVER_MAC {0xF0, 0x9E, 0x9E, 0x22, 0x90, 0xB0}

bool ledState = 0;

int contSprint = 0;
bool noSprint = 1;
int contPotencia = 0;
bool noPotencia = 1;

bool invertir = 0;

const int vChange[3] = {120, 160, 200};
int vSprint = vChange[1];
int contChange_1 = 0;
int iChange_1 = 1;
int contChange_2 = 0;

int vPotencia = vChange[1];

const int aumento = 30;

String movimiento = "libre";
unsigned int velocidad = 0;
unsigned int potencia = 0;

// ESPNOW
uint8_t receiverMac[] = RECEIVER_MAC;
bool isConnected = false;

/////////////////////////////////////////

void setup() {
  // Inicializar comunicación serial
  Serial.begin(115200);
  pinMode(pin_Freno, INPUT_PULLUP);
  pinMode(pin_Sprint, INPUT_PULLUP);
  pinMode(pin_Potencia, INPUT_PULLUP);
  
  pinMode(pin_SW, INPUT_PULLUP);
  pinMode(pin_Inversion, INPUT_PULLUP);

  // Inicializar el WiFi en modo estación (STA)
  WiFi.mode(WIFI_STA);
  Serial.println("Modo WiFi configurado en STA");

  // Inicializar ESPNOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error al inicializar ESPNOW");
    return;
  }

  // Registrar callbacks
  esp_now_register_recv_cb(onDataRecv);
  esp_now_register_send_cb(onDataSent);

  // Configurar el peer (receptor) con la dirección MAC
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, receiverMac, 6);
  peerInfo.channel = 0; // Canal de comunicación (0 para el canal actual)
  peerInfo.encrypt = false;

  // Añadir el peer (receptor)
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Error al agregar peer");
    return;
  }

  Serial.println("Peer agregado con éxito");

  // Inicializar el pin del LED
  pinMode(pin_Led, OUTPUT);
  digitalWrite(pin_Led, 0);

  pinMode(pin_Ledinversion, OUTPUT);
  digitalWrite(pin_Ledinversion, 0);

  analogWrite(pin_LedSprint, 0);
  analogWrite(pin_LedArma, 0);

  
  // Enviar un mensaje de ping al inicio
  while (!isConnected) {
    sendPing();
    ledState = !ledState;
    digitalWrite(pin_Led, ledState);
    delay(1000);
  }
  

  digitalWrite(pin_Led, 1);
  Serial.println("Pong recibido");

  // Crear las tareas de FreeRTOS
  xTaskCreatePinnedToCore(vTaskMotores, "vTaskMotores", 2048, NULL, 0, NULL, 0);
  xTaskCreatePinnedToCore(vTaskPotencia, "vTaskPotencia", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(vTaskSprint, "vTaskSprint", 2048, NULL, 1, NULL, 1);
}

/////////////////////////////////////////

// Callback para recibir datos (nueva firma)
void onDataRecv(const esp_now_recv_info_t *recvInfo, const uint8_t *data, int dataLen) {
  // Verificar si el mensaje recibido es "pong"
  if (dataLen == 4 && strncmp((const char *)data, "pong", 4) == 0) { isConnected = true; }
}

// Callback para el estado del envío
void onDataSent(const uint8_t *macAddr, esp_now_send_status_t status) {
  //Serial.print("Estado de envío a ");
  for (int i = 0; i < 6; i++) {
    //Serial.printf("%02X", macAddr[i]);
    //if (i < 5) Serial.print(":");
  }
  //Serial.println(status == ESP_NOW_SEND_SUCCESS ? " -> Envío exitoso" : " -> Error en el envío");
}

void sendPing() {
  const char *pingMessage = "ping";
  //Serial.println("Enviando mensaje de ping...");
  if (esp_now_send(receiverMac, (uint8_t *)pingMessage, strlen(pingMessage)) == ESP_OK) {}
}

/////////////////////////////////////////
// Núcleo 0

// Tarea Motores, prioridad 1
void vTaskMotores(void *pvParameters) {
  while (true) {
    int SW_1 = digitalRead(pin_SW);
    if (contChange_1 != 0) {
      contChange_1++;
      if (contChange_1 == 6) { contChange_1 = 0; }
    }
    else {
      if (SW_1 == 1) {}
      else {
        contChange_1++;
        iChange_1++;
        if (iChange_1 > 2) { iChange_1 = 0; }
        vSprint = vChange[iChange_1];
      }
    }

    int SW_2 = digitalRead(pin_Inversion);
    if (contChange_2 != 0) {
      contChange_2++;
      if (contChange_2 == 6) { contChange_2 = 0; }
    }
    else {
      if (SW_2 == 1) {}
      else {
        contChange_2++;
        invertir = !invertir;
        digitalWrite(pin_Ledinversion, invertir);
      }
    }

    int VRX = analogRead(pin_VRX);
    int VRY = analogRead(pin_VRY);
    if (VRX >= 1920) { VRX = map(VRX, 1920, 4095, 0, 255); }
    else if (VRX <= 1820) { VRX = map(VRX, 0, 1800, -255, 0); }
    else { VRX = 0; }
    if (VRY >= 2000) { VRY = map(VRY, 2000, 4095, 0, 255); }
    else if (VRY <= 1920) { VRY = map(VRY, 0, 1900, -255, 0); }
    else { VRY = 0; }
    
    if (invertir == 1) {
      VRX = -VRX;
      //VRY = -VRY;
    }
    
    
    // Potencia del Arma
    if (potencia > 255) { potencia = 255; }
    // Movimientos del Carro
    if (velocidad > 255) { velocidad = 255; }

    int ledv_velocidad = velocidad-50;
    if (ledv_velocidad < 0) { ledv_velocidad = 0; }
    int ledv_potencia = potencia-50;
    if (ledv_potencia < 0) { ledv_potencia = 0; }

    if (digitalRead(pin_Freno) == 0) {
      analogWrite(pin_LedSprint, 0);
    }
    else { analogWrite(pin_LedSprint, ledv_velocidad); }
    
    analogWrite(pin_LedArma, ledv_potencia);

    String mensaje = String(potencia) + "&" + String(velocidad) + "&" + String(VRX) + "&" + String(VRY) + "&" + String(digitalRead(pin_Freno));
    Serial.println(mensaje + "&" + String(invertir));
    esp_now_send(receiverMac, (uint8_t *)mensaje.c_str(), mensaje.length());
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

/////////////////////
// Núcleo 1

// Tarea Potencia, prioridad 1
void vTaskPotencia(void *pvParameters) {
  while (true) {
    if (digitalRead(pin_Potencia) == 0) {
      if (contPotencia == 0) { vTaskDelay(pdMS_TO_TICKS(250)); }
      contPotencia++;
      if (digitalRead(pin_Potencia) == 0) { potencia = potencia + aumento; }
      else { noPotencia = !noPotencia; }
    }
    else {
      contPotencia = 0;
      potencia = vPotencia;
    }
    if (noPotencia == 1) { potencia = 0; }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// Tarea Sprint, prioridad 1
void vTaskSprint(void *pvParameters) {
  while (true) {
    if (digitalRead(pin_Sprint) == 0) {
      if (contSprint == 0) { vTaskDelay(pdMS_TO_TICKS(250)); }
      contSprint++;
      if (digitalRead(pin_Sprint) == 0) { velocidad = velocidad + aumento; }
      else { noSprint = !noSprint; }
    }
    else {
      contSprint = 0;
      velocidad = vSprint;
    }
    if (noSprint == 1) { velocidad = 0; }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

/////////////////////////////////////////

void loop() {}
