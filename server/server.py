import os
import json
import time
from pathlib import Path
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler
from json.decoder import JSONDecodeError

# Para el servidor web y websockets
import asyncio
import websockets
import http.server
import socketserver
import threading

# --- Configuración del servidor ---
PORT = 8000 # Puerto donde se servirá la página web
WEBSOCKET_PORT = 8001 # Puerto para la conexión WebSocket
# WEB_DIR ahora es relativo a project_root, no a server.py
# La carpeta 'page' está al mismo nivel que la carpeta 'server'
# por lo tanto, no necesitamos especificar una ruta relativa desde server.py
# El servidor HTTP la buscará desde el "directorio de trabajo" que le pasaremos.

# --- Variables globales para WebSockets ---
CONNECTED_CLIENTS = set() # Conjunto para guardar las conexiones WebSocket activas

class StateMachine:
    # ... (Tu clase StateMachine existente) ...
    STATE_MAP = {
        'ntp.json':      'NTP_UPDATE',
        'sensor.json':   'SENSOR_UPDATE', # Asegúrate de crear este archivo si lo usas
        'alarm.json':    'ALARM_STOP',    # Asegúrate de crear este archivo si lo usas
    }

    @classmethod
    def get_state(cls, filename: str) -> str:
        return cls.STATE_MAP.get(filename, 'UNKNOWN')

class JSONFileHandler(FileSystemEventHandler):
    def __init__(self, tmp_path, debounce_time=0.5, max_retries=3, retry_delay=0.1):
        super().__init__()
        self.tmp_path = tmp_path
        self.last_modified = {}
        self.debounce_time = debounce_time
        self.max_retries = max_retries
        self.retry_delay = retry_delay

    def _read_json_with_retries(self, file_path: str) -> dict | None:
        # ... (Tu método _read_json_with_retries existente) ...
        for attempt in range(self.max_retries):
            try:
                with open(file_path, 'r') as f:
                    data = json.load(f)
                return data
            except JSONDecodeError as e:
                print(f"[ERROR] Intento {attempt + 1}/{self.max_retries} - Leyendo {file_path}: {e}")
                if attempt < self.max_retries - 1:
                    time.sleep(self.retry_delay)
                else:
                    print(f"[ERROR] Fallo al leer {file_path} después de {self.max_retries} intentos.")
                    return None
            except FileNotFoundError:
                print(f"[ERROR] Archivo no encontrado: {file_path}")
                return None
            except Exception as e:
                print(f"[ERROR] Un error inesperado ocurrió al leer {file_path}: {e}")
                return None
        return None

    def on_modified(self, event):
        if event.is_directory or not event.src_path.endswith('.json'):
            return

        now = time.time()
        last = self.last_modified.get(event.src_path, 0)
        if now - last < self.debounce_time:
            return
        self.last_modified[event.src_path] = now

        data = self._read_json_with_retries(event.src_path)

        if data is None:
            return

        filename = os.path.basename(event.src_path)
        state = StateMachine.get_state(filename)

        print(f"[WATCHDOG] Archivo modificado: {filename}")
        if state == 'NTP_UPDATE':
            print("[EVENT] Estado: NTP_UPDATE → preparándose para enviar JSON de NTP")
        elif state == 'SENSOR_UPDATE':
            print("[EVENT] Estado: SENSOR_UPDATE → preparándose para enviar datos de sensor")
        elif state == 'ALARM_STOP':
            print("[EVENT] Estado: ALARM_STOP → preparándose para enviar alarma STOP")
        else:
            print(f"[EVENT] Estado: UNKNOWN para '{filename}'")

        # --- Envío de datos por WebSocket a todos los clientes conectados ---
        message = json.dumps({"filename": filename, "state": state, "data": data}, indent=2)
        asyncio.run_coroutine_threadsafe(
            self.broadcast_message(message),
            asyncio.get_event_loop()
        )
        print(f"[WEBSOCKET] Enviando mensaje a {len(CONNECTED_CLIENTS)} clientes.")
        print(message)
        print()

    async def broadcast_message(self, message: str):
        """Envía un mensaje a todos los clientes WebSocket conectados."""
        if CONNECTED_CLIENTS:
            await asyncio.gather(*[client.send(message) for client in list(CONNECTED_CLIENTS)], return_exceptions=True)
        else:
            print("[WEBSOCKET] No hay clientes conectados para enviar el mensaje.")


# --- Funciones de servidor web y WebSocket ---

# Handler para servir archivos estáticos
class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, directory=None, **kwargs):
        # Asegúrate de que el directorio se pase correctamente
        super().__init__(*args, directory=directory, **kwargs)

def start_http_server(web_dir_path):
    """Inicia el servidor HTTP en un hilo separado."""
    # Usamos socketserver.TCPServer para especificar el directorio.
    # El 'directory' se pasa al Handler.
    class CustomHandler(Handler):
        def __init__(self, *args, **kwargs):
            super().__init__(*args, directory=str(web_dir_path), **kwargs)

    with socketserver.TCPServer(("", PORT), CustomHandler) as httpd:
        print(f"[HTTP SERVER] Sirviendo en el puerto {PORT} desde el directorio '{web_dir_path}'")
        httpd.serve_forever()

async def websocket_handler(websocket, path):
    """Maneja las conexiones WebSocket entrantes."""
    print(f"[WEBSOCKET] Cliente conectado desde {websocket.remote_address}")
    CONNECTED_CLIENTS.add(websocket)
    try:
        await websocket.wait_closed()
    except websockets.exceptions.ConnectionClosed as e:
        print(f"[WEBSOCKET] Cliente desconectado desde {websocket.remote_address} ({e.code}: {e.reason})")
    finally:
        CONNECTED_CLIENTS.remove(websocket)
        print(f"[WEBSOCKET] Clientes activos: {len(CONNECTED_CLIENTS)}")

async def start_websocket_server():
    """Inicia el servidor WebSocket."""
    # Se asegura de que el bucle de eventos esté configurado correctamente
    # asyncio.set_event_loop(asyncio.new_event_loop()) # Esto podría crear problemas si ya hay un loop
    print(f"[WEBSOCKET] Servidor WebSocket escuchando en ws://0.0.0.0:{WEBSOCKET_PORT}")
    async with websockets.serve(websocket_handler, "0.0.0.0", WEBSOCKET_PORT):
        await asyncio.Future() # Corre indefinidamente

async def async_main():
    current_file = Path(__file__).resolve()
    print(f"[INFO] Script ejecutado desde: {current_file}")

    # project_root ahora es la carpeta RPI-DASHWAKE
    # server.py está en RPI-DASHWAKE/server/server.py
    # Entonces, subimos dos niveles para llegar a RPI-DASHWAKE
    project_root = current_file.parent.parent
    print(f"[INFO] Directorio raíz del proyecto: {project_root}")

    # Rutas absolutas a las carpetas 'tmp' y 'page'
    tmp_dir = project_root / 'tmp'
    web_dir_path = project_root / 'page' # Ahora es 'page' en lugar de 'web'
    
    if not web_dir_path.exists():
        print(f"[ERROR] La carpeta web/page no existe en: {web_dir_path}. Por favor, créala y coloca tus archivos HTML/CSS/JS.")
        return

    if not tmp_dir.exists():
        print(f"[ERROR] La carpeta tmp no existe en: {tmp_dir}. Por favor, créala y coloca tus archivos JSON.")
        return

    # Iniciar el servidor HTTP en un hilo separado, pasándole la ruta de la carpeta web
    http_thread = threading.Thread(target=start_http_server, args=(web_dir_path,), daemon=True)
    http_thread.start()

    # Iniciar el servidor WebSocket
    websocket_server_task = asyncio.create_task(start_websocket_server())

    # Configurar watchdog
    handler = JSONFileHandler(tmp_dir, max_retries=5, retry_delay=0.2)
    observer = Observer()
    observer.schedule(handler, str(tmp_dir), recursive=False)
    observer.start()

    try:
        print("[INFO] Esperando cambios en archivos JSON y conexiones web...")
        await asyncio.Future() # Esto mantendrá el bucle de eventos corriendo

    except KeyboardInterrupt:
        print("[INFO] Terminando observador y servidores...")
        observer.stop()
    finally:
        observer.join()
        websocket_server_task.cancel() # Cancelar la tarea del servidor WebSocket
        try:
            await websocket_server_task # Esperar que se cancele
        except asyncio.CancelledError:
            pass
        print("[INFO] Servidores detenidos.")

if __name__ == "__main__":
    asyncio.run(async_main()) # Ejecutar la función asíncrona principal