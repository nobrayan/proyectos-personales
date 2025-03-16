"""

Creado por @nobrayan
WeBT (Bluetooth BLE)
en conjunto con webt 1.1.0-beta.apk

"""

import machine
import time
import bluetooth
import network
import socket
import os
import json
from BLE import BLEUART # Libreria BLE (Guardar en el dispositivo)

# [WeBT] es el prefijo que se mostrara en el nombre del dispositivo
# Si el prefijo se cambia, tambien se debe cambiar en la aplicacion movil
prefijobt = "[WeBT]" # Prefijo del nombre del dispositivo Bluetooth
btname = "Web Viewer" # Nombre del dispositivo Bluetooth

msg_ble = ""
dato_recepcion = 0 # Inicio y final de la recepcion de datos

ssid = "ESP.OK"  # SSID
password = ""  # PASS
guardar_red = "false"  # Guardar red
web_ip = "none"  # IP
estado_web = "false" # Estado de la Web

server = None  # Variable para el servidor

# Ruta del archivo donde se guardaran los datos
file_path = '/config.txt'

# Funcion para verificar si el archivo existe (sin usar os.path)
def file_exists(filepath):
    try:
        with open(filepath, 'r'):
            return True
    except OSError:
        return False

# Funcion para guardar los datos de SSID y PASSWORD
def guardar_datos(ssid, password):
    data = {
        'SSID': ssid,
        'PASSWORD': password
    }
    
    try:
        with open(file_path, 'w') as f:
            json.dump(data, f)
        print("Datos guardados correctamente...")
        time.sleep(2)
    except Exception as e:
        print("Error al guardar los datos...")

# Funcion para leer los datos guardados
def leer_datos():
    global ssid, password

    if file_exists(file_path):
        try:
            with open(file_path, 'r') as f:
                contenido = f.read().strip()

                if not contenido:
                    print("Archivo vacio, no hay datos guardados...")
                    return
                
                data = json.loads(contenido)  # Cargar JSON
                
                ssid = data.get('SSID', "")
                password = data.get('PASSWORD', "")

                if ssid and password:
                    print(f"Datos encontrados...")
                    estado_web = "false"
                    wifi_begin()
                else:
                    print("No hay datos de configuracion guardados...")
        except (OSError, ValueError) as e:
            print("Error al leer los datos...")
    else:
        print("No hay datos de configuracion guardados...")

# Funcion para borrar los datos (el archivo)
def borrar_datos():
    if file_exists(file_path):
        try:
            # Borrar el archivo si existe
            os.remove(file_path)
            print("Datos borrados correctamente...")
        except Exception as e:
            print("Error al borrar los datos...")
    else:
        print("No hay datos de configuracion guardados...")

# Enviar datos por BLE
def enviar_msgble(ssid, ip, estado_web, error):
    global ble_uart

    # Formulacion de mensaje
    """
    //S## --> Inicio
    & --> Separador de datos
    //F## --> Fin
    """
    msg_enviar = f'//S##{ssid}&{ip}&{estado_web}&{error}//F##'
    #print(msg_enviar)
    numero = len(msg_enviar) / 20
    n_veces = int(numero) + (numero > int(numero))
    
    if n_veces == 1: ble_uart.write(msg_enviar.encode())
    else:
        for i in range(n_veces):
            if i == 0: ble_uart.write(msg_enviar[0:20].encode())
            elif i == n_veces - 1: ble_uart.write(msg_enviar[20*i:len(msg_enviar)].encode())
            else: ble_uart.write(msg_enviar[20*i:20*(i+1)].encode())
            time.sleep(.1)

# Datos BLE recibidos
def on_data_received():
    global ssid, password, dato_recepcion, msg_ble
    receivedData = ble_uart.read()
    receivedData = receivedData.decode("utf-8").replace("\x00", "").strip()  # Eliminar '\x00' y otros caracteres no deseados

    # Recibir datos de la aplicacion movil
    # Limite de datos BLE: 20 caracteres
    # Inicio: %#S#%
    # Fin: %#F#%
    # Inicio de la informacion, limpiar los datos y descartar
    if receivedData[0:5] == "%#S#%":
        msg_ble = ""
        dato_recepcion = 1
    if dato_recepcion == 1:
        msg_ble = msg_ble + receivedData # Concatenar los datos recibidos
        if receivedData[len(receivedData)-5:len(receivedData)] == "%#F#%":
            dato_recepcion = 0 # Fin de la informacion

            # Procesar los datos recibidos
            if msg_ble[0:5] == "%#S#%" and msg_ble[len(msg_ble)-5:len(msg_ble)] == "%#F#%":
                msg_ble = msg_ble[4:len(msg_ble)-4]
                #print(f'Datos recibidos: {msg_ble}')

                # Filtrar y separar los datos recibidos
                if msg_ble[0] != "%" or msg_ble[len(msg_ble)-1] != "%": print("Datos BT descartados...")
                else:
                    # Serparar los datos recibidos
                    separacion_msg = ["", "", "", ""]
                    x = 0
                    for i in range(len(msg_ble)):
                        if msg_ble[i] == "%": pass
                        elif msg_ble[i] == "&": x += 1
                        else: separacion_msg[x] = separacion_msg[x] + msg_ble[i]
                    
                    if separacion_msg[0] == "ESP.START":
                        print("Enviando informacion de inicio...")
                        enviar_msgble(ssid, web_ip, estado_web, "error")
                    elif separacion_msg[0] == "ESP.RESET":
                        # ERROR 600: Reiniciar ESP...
                        enviar_msgble("ESP.OK", "IP", "false", "600")
                        print(f"Reiniciando...")
                        
                        # Reiniciar el ESP32
                        time.sleep(2)
                        machine.reset()
                    else:
                        # Asignar los datos recibidos
                        ssid = separacion_msg[1]
                        password = separacion_msg[2]
                        guardar_red = separacion_msg[3]

                        print(f'SSID: {ssid}, PASS: {password}, Guardar red: {guardar_red}')

                        if guardar_red == "true":
                            # Guardar los datos
                            guardar_datos(ssid, password)
                        else:
                            # Borrar los datos
                            borrar_datos()

                        # Conectar a la red WiFi
                        estado_web == "false"
                        wifi_begin()

# Conexion con dispositivo BT (Al conectar)
def on_connect():
    pass

# Desconexion con dispositivo BT (Al desconectar) 
def on_disconnect():
    pass

# Iniciar la conexion WiFi
def wifi_begin():
    global tcp_socket, web_ip, estado_web, server

    wifi = network.WLAN(network.STA_IF)
    wifi.active(True)
    
    # Desconectar si ya esta conectado
    estado_web = "false"
    wifi.disconnect()
    print("Wifi desconectado...")

    time.sleep(2)

    wifi.connect(ssid, password)
    print(f"Conectando a {ssid}...")

    for _ in range(5):  # Esperar hasta 10 segundos
        if wifi.isconnected():
            print("IP:", wifi.ifconfig()[0])
            web_ip = wifi.ifconfig()[0]

            # Iniciar servidor
            setup_server()
            estado_web = "true"

            # Enviar datos de conexion
            enviar_msgble(ssid, web_ip, estado_web, "error")

            return
        time.sleep(1)

    # Si no se pudo conectar, enviar un mensaje de error
    estado_web = "false"
    
    if wifi.isconnected():
        wifi.disconnect()

    
    # ERROR 503: No se pudo conectar
    enviar_msgble("ESP.OK", "IP", "false", "503")
    print("Error de conexion...")
    return

# Pagina web
def web_page():  
    html = """<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Web Page</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background-color: #f0f8ff;
            text-align: center;
            padding: 50px;
        }
        h1 {
            color: #007acc;
        }
        .btn {
            background-color: #007acc;
            color: white;
            border: none;
            padding: 15px 32px;
            font-size: 16px;
            cursor: pointer;
            border-radius: 5px;
        }
        .btn:hover {
            background-color: #005fa3;
        }
        .content {
            background-color: #ffffff;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0px 0px 10px rgba(0, 0, 0, 0.1);
            margin-top: 30px;
        }
    </style>
    <script>
        function showMessage() {
            alert("¡Hola desde el ESP32!");
        }
    </script>
</head>
<body>
    <h1>Bienvenido a la pagina web del ESP32</h1>
    <div class="content">
        <p>Haz clic en el siguiente boton para ver el mensaje:</p>
        <button class="btn" onclick="showMessage()">Haz clic aqui</button>
    </div>
</body>
</html>"""
    return html

# Iniciar el servidor web
def setup_server():
    global estado_web, server

    # Intentar crear un nuevo servidor
    try:
        # Crear un nuevo servidor en el puerto 80
        server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server.bind(('', 80))  # Vincular al puerto 80
        server.listen(5)
    except OSError as e:
        # ERROR 500: Error al iniciar el servidor (Reinicio)
        enviar_msgble("ESP.OK", "IP", "false", "500")
        print(f"Error al iniciar el servidor...")

        # Reiniciar el ESP32
        time.sleep(1)
        print(f"Reiniciando...")
        time.sleep(1)
        machine.reset()
    
    # Si todo esta bien, activar estado web
    estado_web = "true"

# setup()
ble = bluetooth.BLE()
name = f'{prefijobt} {btname}'

global ble_uart
ble_uart = BLEUART(ble, name)

# Leer archivo de configuracion Wi-Fi
leer_datos()

# Configurar BLE
ble_uart.irq(on_data_received)
ble_uart.on_connect(on_connect)
ble_uart.on_disconnect(on_disconnect)
print("Dispositivo BLE iniciado...")

# loop()
while True:
    if estado_web == "true" and server is not None:  # Verificar que el servidor este correctamente inicializado
        conn, addr = server.accept()
        #print("Conexion desde:", addr)
        request = conn.recv(1024).decode()
        #print("Solicitud recibida:", request)

        conn.send("HTTP/1.1 200 OK\nContent-Type: text/html\n\n")
        conn.send(web_page())
        conn.close()
    else:
        # Si el servidor no esta disponible, cerrar el servidor
        if server is not None:
            server.close()
            print("Servidor web detenido...")
        time.sleep(1)