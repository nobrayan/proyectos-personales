/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*

  Creado por @nobrayan
  Carro de Combate
  en conjunto con MandoBLE.apk

*/
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLECharacteristic.h>

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NOMBRE DE LA CONEXION BLUETOOTH (CAMBIAR)
String NOMBRE = "NOMBRE";

// Pines del puente H
// MOTOR 1
const int IN1 = 4;
const int IN2 = 5;
const int ENA = 16;
// MOTOR 2
const int IN3 = 6;
const int IN4 = 7;
const int ENB = 17;

// ARMA
const int POT = 15;

// WASD
bool W = 0; // Adelante
bool S = 0; // Atras
bool A = 0; // Izquierda
bool D = 0; // Derecha

// BOTONES
bool B_1 = 0; // B1
bool B_2 = 0; // B2
bool B_3 = 0; // B3
bool B_4 = 0; // B1
bool B_5 = 0; // B2
bool B_6 = 0; // B3

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

// UUIDs para el servicio y la característica
#define SERVICE_UUID        "12345678-1234-1234-1234-123456789abc" // Cambia según tus necesidades
#define CHARACTERISTIC_UUID "abcdef01-1234-1234-1234-123456789abc" // Cambia según tus necesidades

bool Arma1 = 0;
int contBot1 = 0;

BLEServer *pServer = nullptr;
BLECharacteristic *pCharacteristic = nullptr;
bool deviceConnected = false;
bool restartedAfterDisconnect = false; // Bandera para evitar múltiples reinicios
String receivedMessage = ""; // Variable global para almacenar el mensaje recibido

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Motores() {
  // ADELANTE
  if (W == 1) {
    digitalWrite(IN1, 1);
    digitalWrite(IN2, 0);
    digitalWrite(IN3, 1);
    digitalWrite(IN4, 0);
    analogWrite(ENA, 255);
    analogWrite(ENB, 255);
  }
  // ATRAS
  else if (S == 1) {
    digitalWrite(IN1, 0);
    digitalWrite(IN2, 1);
    digitalWrite(IN3, 0);
    digitalWrite(IN4, 1);
    analogWrite(ENA, 255);
    analogWrite(ENB, 255);
  }
  // IZQUIERDA
  else if (A == 1) {
    digitalWrite(IN1, 0);
    digitalWrite(IN2, 1);
    digitalWrite(IN3, 1);
    digitalWrite(IN4, 0);
    analogWrite(ENA, 255);
    analogWrite(ENB, 255);
  }
  // DERECHA
  else if (D == 1) {
    digitalWrite(IN1, 1);
    digitalWrite(IN2, 0);
    digitalWrite(IN3, 0);
    digitalWrite(IN4, 1);
    analogWrite(ENA, 255);
    analogWrite(ENB, 255);
  }
  // LIBRE
  else {
    digitalWrite(IN1, 0);
    digitalWrite(IN2, 0);
    digitalWrite(IN3, 0);
    digitalWrite(IN4, 0);
    analogWrite(ENA, 0);
    analogWrite(ENB, 0);
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Identificacion(String m) {
  if (m == "W1") { W = 1; }
  else if (m == "W0") { W = 0; }
  if (m == "S1") { S = 1; }
  else if (m == "S0") { S = 0; }
  if (m == "A1") { A = 1; }
  else if (m == "A0") { A = 0; }
  if (m == "D1") { D = 1; }
  else if (m == "D0") { D = 0; }

  if (m == "B11") { B_1 = 1; }
  else if (m == "B10") { B_1 = 0; }
  if (m == "B21") { B_2 = 1; }
  else if (m == "B20") { B_2 = 0; }
  if (m == "B31") { B_3 = 1; }
  else if (m == "B30") { B_3 = 0; }
  if (m == "B41") { B_4 = 1; }
  else if (m == "B40") { B_4 = 0; }
  if (m == "B51") { B_5 = 1; }
  else if (m == "B50") { B_5 = 0; }
  if (m == "B61") { B_6 = 1; }
  else if (m == "B60") { B_6 = 0; }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Callback para manejar conexión y desconexión
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    Serial.println("Dispositivo conectado");
    deviceConnected = true;
    restartedAfterDisconnect = false; // Reinicia la bandera al conectarse
  }

  void onDisconnect(BLEServer* pServer) {
    Serial.println("Dispositivo desconectado");
    deviceConnected = false;
    // Permitir nuevas conexiones
    pServer->startAdvertising();
  }
};

// Callback para manejar escritura en la característica
class MyCharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String value = pCharacteristic->getValue();
    if (value.length() > 0) {
      // Guardar el mensaje recibido en la variable global
      receivedMessage = value;

      Identificacion(receivedMessage);

      //Serial.print("Datos recibidos: ");
      //Serial.println(receivedMessage);
    }
  }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  // Configuración inicial
  Serial.begin(115200);
  Serial.println("Iniciando servidor BLE...");

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(POT, OUTPUT);

  digitalWrite(IN1, 0);
  digitalWrite(IN2, 0);
  digitalWrite(IN3, 0);
  digitalWrite(IN4, 0);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  digitalWrite(POT, 0);

  // Inicializar BLE
  BLEDevice::init(NOMBRE); // Cambia el nombre según prefieras
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Crear el servicio y la característica
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
  );

  // Configurar callbacks para manejar los datos recibidos
  pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());

  // Iniciar el servicio y empezar a anunciar
  pService->start();
  pServer->getAdvertising()->start();
  Serial.println("Servidor BLE iniciado y en espera de conexiones...");
}

void loop() {
  // Si no hay conexión y aún no se ha reiniciado después de la desconexión
  if (!deviceConnected && !restartedAfterDisconnect) {
    Serial.println("Reiniciando BLE tras desconexión...");
    restartedAfterDisconnect = true; // Evitar múltiples reinicios

    // Detener publicidad y liberar BLE
    pServer->getAdvertising()->stop();
    BLEDevice::deinit();

    // Reiniciar el servidor BLE
    BLEDevice::init(NOMBRE);
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Configurar el servicio y la característica nuevamente
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
    );
    pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());

    // Reiniciar servicio y publicidad
    pService->start();
    pServer->getAdvertising()->start();

    Serial.println("Servidor BLE reiniciado.");
  }

  if (deviceConnected) {
    Serial.println("W:" + String(W) + " A:" + String(A) + " S:" + String(S) + " D:" + String(D) + " B1:" + String(B_1) + " B2:" + String(B_2) + " B3:" + String(B_3) + " B4:" + String(B_4) + " B5:" + String(B_5) + " B6:" + String(B_6));
    
    Motores();
    if (contBot1 != 0) {
      contBot1++;
      if (contBot1 == 6) { contBot1 = 0; }
    }
    else {
      if (B_1 == 0) {}
      else {
        contBot1++;
        Arma1 = !Arma1;
        digitalWrite(POT, Arma1);
      }
    }
  }
  else {
    digitalWrite(IN1, 0);
    digitalWrite(IN2, 0);
    digitalWrite(IN3, 0);
    digitalWrite(IN4, 0);
    analogWrite(ENA, 0);
    analogWrite(ENB, 0);
    digitalWrite(POT, 0);
  }

  delay(50);
}