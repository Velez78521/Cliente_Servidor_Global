import socket
import sqlite3
from flask import Flask, jsonify, render_template
from datetime import datetime
import threading

# Configuración del servidor UDP
UDP_PORT = 6969
BUFFER_SIZE = 128
DB_NAME = "sensores.db"

# Configuración del servidor HTTP
app = Flask(__name__)

# Función para configurar la base de datos
def setup_database():
    conn = sqlite3.connect(DB_NAME)
    cursor = conn.cursor()
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS sensores (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            temperatura REAL,
            humedad REAL
        )
    """)
    conn.commit()
    conn.close()

# Función para guardar datos en la base de datos
def save_to_database(temperatura, humedad):
    conn = sqlite3.connect(DB_NAME)
    cursor = conn.cursor()
    cursor.execute("INSERT INTO sensores (temperatura, humedad) VALUES (?, ?)", (temperatura, humedad))
    conn.commit()
    conn.close()

# Función para iniciar el servidor UDP
def start_udp_server():
    setup_database()

    with socket.socket(socket.AF_INET6, socket.SOCK_DGRAM) as server_socket:
        server_socket.bind(("::", UDP_PORT))
        print(f"Servidor UDP escuchando en el puerto {UDP_PORT}...")

        while True:
            data, addr = server_socket.recvfrom(BUFFER_SIZE)
            mensaje = data.decode('utf-8')
            print(f"Datos recibidos: {mensaje} de {addr}")

            try:
                partes = mensaje.split(", ")
                temperatura = float(partes[0].split(": ")[1][:-2])
                humedad = float(partes[1].split(": ")[1][:-1])

                save_to_database(temperatura, humedad)
                print(f"Datos guardados: Temperatura={temperatura}°C, Humedad={humedad}%")

                response = "Datos guardados correctamente"
                server_socket.sendto(response.encode('utf-8'), addr)

            except Exception as e:
                print(f"Error al procesar los datos: {e}")
                response = "Error al procesar los datos"
                server_socket.sendto(response.encode('utf-8'), addr)

# Ruta para obtener datos en formato JSON
@app.route("/api/datos")
def api_datos():
    conn = sqlite3.connect(DB_NAME)
    cursor = conn.cursor()
    cursor.execute("SELECT timestamp, temperatura, humedad FROM sensores ORDER BY timestamp DESC")
    rows = cursor.fetchall()
    conn.close()

    # Convertir los datos en formato JSON
    data = [
        {"timestamp": row[0], "temperatura": row[1], "humedad": row[2]}
        for row in rows
    ]
    return jsonify(data)

# Ruta para visualizar los datos en Google Charts
@app.route("/")
def index():
    return render_template("index.html")

# Función para iniciar el servidor HTTP
def start_http_server():
    app.run(host="0.0.0.0", port=5000)

# Función principal
if __name__ == "__main__":
    udp_thread = threading.Thread(target=start_udp_server, daemon=True)
    udp_thread.start()

    start_http_server()

