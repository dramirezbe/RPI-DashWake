import os
import json
import time
import logging
from pathlib import Path
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler
from json.decoder import JSONDecodeError
from threading import Lock # Importar Lock para asegurar acceso a LAST_PROCESSED_DATA

# Para el servidor web
import http.server
import socketserver
import threading
import sys # Import sys for better exception info

# --- Configuración de Logging ---
# Configura el nivel de logging y el formato para ver más detalles.
# DEBUG para desarrollo, INFO para producción.
logging.basicConfig(level=logging.INFO,
                    format='%(asctime)s - %(levelname)s - %(threadName)s - %(message)s',
                    handlers=[logging.StreamHandler(sys.stdout)])
logger = logging.getLogger(__name__)

# --- Configuración del servidor ---
PORT = 50000 # Puerto donde se servirá la página web
# WEBSOCKET_PORT = 50001 # Eliminado, ya no se usa WebSockets

# --- Variables globales para Polling ---
# Almacenará el último estado de todos los JSON procesados por el Watchdog.
# Las claves serán los nombres de archivo (ej. 'sensor.json')
# Los valores serán objetos: {"state": "SENSOR_UPDATE", "data": {...}}
LAST_PROCESSED_DATA = {}
data_lock = Lock() # Usamos un Lock para proteger esta variable global.

class StateMachine:
    STATE_MAP = {
        'ntp.json':      'NTP_UPDATE',
        'sensor.json':   'SENSOR_UPDATE',
        'alarm.json':    'ALARM_STOP',
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
        # self.loop = loop # Este parámetro fue eliminado, ya no es necesario
        logger.info(f"JSONFileHandler initialized. Monitoring {tmp_path}")

    def _read_json_with_retries(self, file_path: str) -> dict | None:
        for attempt in range(self.max_retries):
            try:
                with open(file_path, 'r') as f:
                    data = json.load(f)
                logger.debug(f"Successfully read JSON from {file_path} on attempt {attempt + 1}")
                return data
            except JSONDecodeError as e:
                logger.error(f"Intento {attempt + 1}/{self.max_retries} - Error de JSON en '{file_path}': {e}")
                if attempt < self.max_retries - 1:
                    time.sleep(self.retry_delay)
                else:
                    logger.critical(f"Fallo al leer '{file_path}' después de {self.max_retries} intentos. Datos corruptos o formato incorrecto.")
                    return None
            except FileNotFoundError:
                logger.error(f"Archivo no encontrado: {file_path}")
                return None
            except Exception as e:
                logger.error(f"Un error inesperado ocurrió al leer '{file_path}': {e}", exc_info=True) # exc_info=True para el traceback
                return None
        return None

    def on_modified(self, event):
        if event.is_directory or not event.src_path.endswith('.json'):
            logger.debug(f"Ignorando evento: {event.src_path} (no es un archivo JSON o es un directorio)")
            return

        now = time.time()
        last = self.last_modified.get(event.src_path, 0)
        if now - last < self.debounce_time:
            logger.debug(f"Debouncing event for {event.src_path}. Last modified {now - last:.2f}s ago.")
            return
        self.last_modified[event.src_path] = now

        logger.info(f"Detectado cambio en el archivo: {event.src_path}")
        data = self._read_json_with_retries(event.src_path)

        if data is None:
            logger.warning(f"No se pudieron leer los datos del archivo {event.src_path}. No se actualizará el estado global.")
            return

        filename = os.path.basename(event.src_path)
        state = StateMachine.get_state(filename)

        logger.info(f"[WATCHDOG] Archivo modificado: {filename}")
        if state == 'NTP_UPDATE':
            logger.info("[EVENT] Estado: NTP_UPDATE")
        elif state == 'SENSOR_UPDATE':
            logger.info("[EVENT] Estado: SENSOR_UPDATE")
        elif state == 'ALARM_STOP':
            logger.info("[EVENT] Estado: ALARM_STOP")
        else:
            logger.warning(f"[EVENT] Estado: UNKNOWN para '{filename}'")

        # --- Actualizar la variable global con los últimos datos ---
        with data_lock: # Proteger el acceso a la variable global
            LAST_PROCESSED_DATA[filename] = {"state": state, "data": data}
        logger.info(f"[POLLING] Estado global actualizado para '{filename}'.")
        # logger.debug(f"Estado global actual: {json.dumps(LAST_PROCESSED_DATA, indent=2)}") # Descomentar para ver el estado global

# --- Funciones de servidor web ---

class CustomHTTPHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, directory=None, **kwargs):
        super().__init__(*args, directory=directory, **kwargs)

    def do_GET(self):
        logger.info(f"HTTP GET request: {self.path} from {self.client_address[0]}")

        # --- Nuevo endpoint para Polling ---
        if self.path == '/data':
            self._serve_json_data()
            return
        # --- Fin Nuevo endpoint para Polling ---

        try:
            super().do_GET() # Sirve archivos estáticos (index.html, app.js, style.css)
        except Exception as e:
            logger.error(f"Error serving HTTP GET for {self.path}: {e}", exc_info=True)
            self.send_error(500, "Internal Server Error")

    def _serve_json_data(self):
        """Sirve los datos JSON más recientes para polling."""
        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        # CORS: Esto permite que tu página JS acceda a este endpoint.
        # '*' significa que cualquier origen puede acceder. Para mayor seguridad,
        # deberías reemplazar '*' con el origen exacto de tu página (ej. 'http://192.168.1.4:50000').
        self.send_header('Access-Control-Allow-Origin', '*')
        self.end_headers()
        
        with data_lock: # Leer la variable global de forma segura
            response_data = json.dumps(LAST_PROCESSED_DATA, indent=2).encode('utf-8')
        self.wfile.write(response_data)
        logger.debug(f"Servido /data endpoint: {len(LAST_PROCESSED_DATA)} archivos JSON")


    def end_headers(self):
        # Mejora de seguridad: Añadir cabeceras HTTP recomendadas
        self.send_header('X-Content-Type-Options', 'nosniff')
        self.send_header('X-Frame-Options', 'DENY') # Previene clickjacking
        self.send_header('Cache-Control', 'no-store, no-cache, must-revalidate, max-age=0')
        self.send_header('Pragma', 'no-cache')
        self.send_header('Expires', '0')
        self.server_version = "MyCustomServer/1.0"
        logger.debug("Added security headers.")
        super().end_headers()


def start_http_server(web_dir_path):
    """Inicia el servidor HTTP en un hilo separado."""
    class HandlerWithDirectory(CustomHTTPHandler):
        def __init__(self, *args, **kwargs):
            super().__init__(*args, directory=str(web_dir_path), **kwargs)

    try:
        socketserver.TCPServer.allow_reuse_address = True
        with socketserver.TCPServer(("", PORT), HandlerWithDirectory) as httpd:
            logger.info(f"[HTTP SERVER] Sirviendo en el puerto {PORT} desde el directorio '{web_dir_path}'")
            httpd.serve_forever()
    except Exception as e:
        logger.critical(f"[HTTP SERVER] Fallo crítico al iniciar el servidor HTTP: {e}", exc_info=True)
        raise

# --- ELIMINADAS FUNCIONES WEBSOCKET ---
# No hay funciones relacionadas con websockets aquí.
# --- FIN ELIMINADAS FUNCIONES WEBSOCKET ---

# --- FUNCIÓN PRINCIPAL SÍNCRONA (YA NO ES ASÍNCRONA) ---
def main(): # CAMBIADO de 'async def async_main():' a 'def main():'
    current_file = Path(__file__).resolve()
    logger.info(f"[INFO] Script ejecutado desde: {current_file}")

    project_root = current_file.parent.parent
    logger.info(f"[INFO] Directorio raíz del proyecto: {project_root}")

    tmp_dir = project_root / 'tmp'
    web_dir_path = project_root / 'page'
    
    if not web_dir_path.exists():
        logger.error(f"[ERROR] La carpeta web/page no existe en: {web_dir_path}. Por favor, créala y coloca tus archivos HTML/CSS/JS.")
        sys.exit(1) # Exit if critical folder is missing

    if not tmp_dir.exists():
        logger.error(f"[ERROR] La carpeta tmp no existe en: {tmp_dir}. Por favor, créala y coloca tus archivos JSON.")
        sys.exit(1) # Exit if critical folder is missing

    # Iniciar el servidor HTTP en un hilo separado
    http_thread = threading.Thread(target=start_http_server, args=(web_dir_path,), daemon=True, name="HTTP-Server-Thread")
    http_thread.start()
    logger.info("Hilo del servidor HTTP iniciado.")

    # Configurar watchdog. 'loop' ya no es un parámetro del constructor de JSONFileHandler
    handler = JSONFileHandler(tmp_dir, max_retries=5, retry_delay=0.2)
    observer = Observer()
    observer.schedule(handler, str(tmp_dir), recursive=False)
    observer.start()
    logger.info(f"Watchdog iniciado para monitorear '{tmp_dir}'")

    try:
        logger.info("[INFO] Esperando cambios en archivos JSON y conexiones web... Presiona Ctrl+C para salir.")
        # Mantener el hilo principal vivo con un bucle síncrono
        while True:
            time.sleep(1) # Espera 1 segundo para no consumir 100% CPU
    except KeyboardInterrupt:
        logger.info("[INFO] Terminando observador y servidores debido a KeyboardInterrupt...")
    except Exception as e:
        logger.critical(f"[ERROR] Error inesperado en main: {e}", exc_info=True)
    finally:
        logger.info("[INFO] Deteniendo servicios...")
        observer.stop()
        observer.join() # Esperar a que el hilo de Watchdog termine
        logger.info("Watchdog detenido.")
        
        logger.info("[INFO] Servidores y observadores detenidos.")

if __name__ == "__main__":
    # Ahora llamamos a 'main()' directamente, ya no es una coroutine
    try:
        main() # CAMBIADO de 'asyncio.run(async_main())' a 'main()'
    except Exception as main_e:
        logger.critical(f"La aplicación falló al iniciar o durante la ejecución principal: {main_e}", exc_info=True)