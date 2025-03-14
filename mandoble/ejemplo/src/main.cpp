/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*

  Creado por @nobrayan
  Ejemplo de Control Robot
  en conjunto con MandoBLE.apk

*/
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include <NimBLEDevice.h>

// Pines del puente H
/*
// Motor 1
#define IN1 4
#define IN2 5
#define ENA 16
// Motor 2
#define IN3 6
#define IN4 7
#define ENB 17
// ARMA
#define POT 15
*/

// Nombre de la conexion Bluetooth
// [MandoBLE] es el prefijo que se mostrara en el nombre del dispositivo
// Si el prefijo se cambia, tambien se debe cambiar en la aplicacion movil
const char *deviceName = "[MandoBLE] Carro";

// UUIDs para el servicio y la caracteristica
const char *serviceUUID = "1234";
const char *characteristicUUID = "5678";

// Botones: ADELANTE, ATRAS, DERECHA, IZQUIERDA, B1, B2, B3, B4, B5, B6
String botones[10][2] = {
    {"up", "0"},
    {"down", "0"},
    {"right", "0"},
    {"left", "0"},
    {"b1", "0"},
    {"b2", "0"},
    {"b3", "0"},
    {"b4", "0"},
    {"b5", "0"},
    {"b6", "0"}};

// Variables Bluetooth BLE
bool device_connected = false;
String receivedMessage = "";
unsigned int time_notify = 0;

void motores();
void identificacion(String msg);

static NimBLEServer *pServer;
// Callback para las caracteristicas
class CharacteristicCallbacks : public NimBLECharacteristicCallbacks
{
  void onRead(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override
  {
    // onRead()
    // pCharacteristic->getValue().c_str();
  }

  void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override
  {
    // onWrite()
    receivedMessage = pCharacteristic->getValue().c_str();
    identificacion(receivedMessage);
    // Serial.println("Mensaje recibido: " + receivedMessage);
  }
} chrCallbacks;

// Callback para el servidor (conexion, desconexion)
class ServerCallbacks : public NimBLEServerCallbacks
{
  void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) override
  {
    Serial.printf("Cliente conectado... Direccion: %s\n", connInfo.getAddress().toString().c_str());
    device_connected = true;
  }

  void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) override
  {
    Serial.printf("Cliente desconectado... Reiniciando la publicidad...\n");
    NimBLEDevice::startAdvertising();
    device_connected = false;
  }
} serverCallbacks;

/*========================================================================================================*/

void setup()
{
  Serial.begin(115200);
  Serial.println("Iniciando NimBLE Server...");

  // Configuracion de Pines
  /*
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(POT, OUTPUT);

  // Modo Libre
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  digitalWrite(POT, LOW);
  */

  // Inicializa NimBLE y establece el nombre del dispositivo
  NimBLEDevice::init(deviceName);

  // Crea el servidor BLE
  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(&serverCallbacks);

  // Crea el servicio y las caracteristicas
  NimBLEService *pFoodService = pServer->createService(serviceUUID);
  NimBLECharacteristic *pFoodCharacteristic = pFoodService->createCharacteristic(
      characteristicUUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY);

  pFoodCharacteristic->setValue("MandoBLE"); // Valor inicial
  pFoodCharacteristic->setCallbacks(&chrCallbacks);

  // Inicia el servicio
  pFoodService->start();

  // Configura la publicidad
  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->setName(deviceName);
  pAdvertising->addServiceUUID(pFoodService->getUUID());
  pAdvertising->enableScanResponse(true);
  pAdvertising->start();

  Serial.println("Publicidad iniciada...");
}

void loop()
{
  // Notificacion cada 1sg con 50ms de delay
  if (time_notify == 20)
  {
    // Reiniciar el tiempo de envio de notificacion
    time_notify = 0;

    // Si el servidor esta conectado, puedes enviar notificaciones
    if (pServer->getConnectedCount())
    {
      NimBLEService *pSvc = pServer->getServiceByUUID(serviceUUID);
      if (pSvc)
      {
        NimBLECharacteristic *pChr = pSvc->getCharacteristic(characteristicUUID);
        if (pChr)
        {
          // Enviar una notificacion al cliente conectado
          pChr->notify();
        }
      }
    }
  }

  // Control del Robot
  if (!device_connected)
  {
    /*
    // Modo Libre
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    analogWrite(ENA, 0);
    analogWrite(ENB, 0);
    digitalWrite(POT, LOW);
    */
  }
  else
  {
    // Ver botones en monitor serial
    Serial.print("Botones: ");
    for (unsigned int i = 0; i < sizeof(botones) / sizeof(botones[0]); i++)
    {
      if (i == ((sizeof(botones) / sizeof(botones[0])) - 1))
      {
        Serial.print(botones[i][0]);
        Serial.print(" ");
        Serial.print(botones[i][1]);
      }
      else
      {
        Serial.print(botones[i][0]);
        Serial.print(" ");
        Serial.print(botones[i][1]);
        Serial.print(", ");
      }
    }
    Serial.println("");

    // Mover motores
    //motores();

    // Arma usando B1 (4)
    /*
    if (botones[4][1] == "1")
      digitalWrite(POT, HIGH);
    else
      digitalWrite(POT, LOW);
    */
  }

  time_notify++;
  delay(50);
}

/*========================================================================================================*/

// Funcion de motores
/*
void motores()
{
  // Botones:
  // 0 - ADELANTE,
  // 1 - ATRAS,
  // 2 - DERECHA,
  // 3 - IZQUIERDA
  // 4 - B1,
  // 5 - B2,
  // 6 - B3,
  // 7 - B4,
  // 8 - B5,
  // 9 - B6,
  
  // Adelante
  if (botones[0][1] == "1")
  {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    analogWrite(ENA, 255);
    analogWrite(ENB, 255);
  }
  // Atras
  else if (botones[1][1] == "1")
  {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    analogWrite(ENA, 255);
    analogWrite(ENB, 255);
  }
  // Derecha
  else if (botones[2][1] == "1")
  {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    analogWrite(ENA, 255);
    analogWrite(ENB, 255);
  }
  // Izquierda
  else if (botones[3][1] == "1")
  {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    analogWrite(ENA, 255);
    analogWrite(ENB, 255);
  }
  // Modo Libre
  else
  {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    analogWrite(ENA, 0);
    analogWrite(ENB, 0);
  }
}
*/

void identificacion(String msg)
{
  // Comprobar boton
  for (unsigned int i = 0; i < sizeof(botones) / sizeof(botones[0]); i++)
  {
    // Botones 1-6
    if (botones[i][0] == String(msg[0]))
    {
      botones[i][1] = msg[msg.length() - 1];
      break;
    }
    // Botones movimiento
    else if (botones[i][0] == msg.substring(0, msg.length() - 1))
    {
      botones[i][1] = msg[msg.length() - 1];
      break;
    }
  }
}