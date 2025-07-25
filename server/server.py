import os
import json
import time
import logging
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
import sys # Import sys for better exception info

# --- Configuración de Logging ---
# Configura el nivel de logging y el formato para ver más detalles.
# DEBUG para desarrollo, INFO para producción.
logging.basicConfig(level=logging.INFO,
                    format='%(asctime)s - %(levelname)s - %(threadName)s - %(message)s',
                    handlers=[logging.StreamHandler(sys.stdout)])
logger = logging.getLogger(__name__)

# --- Configuración del servidor ---
PORT = 8000 # Puerto donde se servirá la página web
WEBSOCKET_PORT = 8001 # Puerto para la conexión WebSocket

# --- Variables globales para WebSockets ---
CONNECTED_CLIENTS = set() # Conjunto para guardar las conexiones WebSocket activas

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
    def __init__(self, tmp_path, loop, debounce_time=0.5, max_retries=3, retry_delay=0.1):
        super().__init__()
        self.tmp_path = tmp_path
        self.last_modified = {}
        self.debounce_time = debounce_time
        self.max_retries = max_retries
        self.retry_delay = retry_delay
        self.loop = loop # Store the event loop here
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
            logger.warning(f"No se pudieron leer los datos del archivo {event.src_path}. No se enviará el mensaje.")
            return

        filename = os.path.basename(event.src_path)
        state = StateMachine.get_state(filename)

        logger.info(f"[WATCHDOG] Archivo modificado: {filename}")
        if state == 'NTP_UPDATE':
            logger.info("[EVENT] Estado: NTP_UPDATE → preparándose para enviar JSON de NTP")
        elif state == 'SENSOR_UPDATE':
            logger.info("[EVENT] Estado: SENSOR_UPDATE → preparándose para enviar datos de sensor")
        elif state == 'ALARM_STOP':
            logger.info("[EVENT] Estado: ALARM_STOP → preparándose para enviar alarma STOP")
        else:
            logger.warning(f"[EVENT] Estado: UNKNOWN para '{filename}'")

        # --- Envío de datos por WebSocket a todos los clientes conectados ---
        try:
            message = json.dumps({"filename": filename, "state": state, "data": data}, indent=2)
            
            # Schedule the coroutine in the main event loop
            self.loop.call_soon_threadsafe(
                asyncio.create_task,
                self.broadcast_message(message)
            )
            logger.info(f"[WEBSOCKET] Solicitud de envío de mensaje a {len(CONNECTED_CLIENTS)} clientes. (desde Watchdog thread)")
            # logger.debug(f"Mensaje a enviar: {message}") # Descomentar para ver el contenido completo
        except Exception as e:
            logger.error(f"Error al preparar o programar el mensaje WebSocket: {e}", exc_info=True)

    async def broadcast_message(self, message: str):
        """Envía un mensaje a todos los clientes WebSocket conectados."""
        if not CONNECTED_CLIENTS:
            logger.info("[WEBSOCKET] No hay clientes conectados para enviar el mensaje.")
            return

        logger.info(f"[WEBSOCKET] Realizando envío a {len(CONNECTED_CLIENTS)} clientes activos.")
        
        # Guardar una copia para evitar 'RuntimeError: Set changed size during iteration'
        # si un cliente se desconecta mientras se itera.
        clients_to_send = list(CONNECTED_CLIENTS)
        
        results = await asyncio.gather(
            *[client.send(message) for client in clients_to_send],
            return_exceptions=True
        )

        for i, result in enumerate(results):
            if isinstance(result, Exception):
                client = clients_to_send[i]
                logger.error(f"[WEBSOCKET] Error al enviar a cliente {client.remote_address}: {result}. Eliminando cliente de la lista.", exc_info=True)
                # Opcional: remover clientes que fallan al enviar
                if client in CONNECTED_CLIENTS:
                    CONNECTED_CLIENTS.remove(client)
            # else:
            #     logger.debug(f"Mensaje enviado con éxito a cliente {clients_to_send[i].remote_address}")


# --- Funciones de servidor web y WebSocket ---

# Handler para servir archivos estáticos
class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, directory=None, **kwargs):
        logger.debug(f"Initializing SimpleHTTPRequestHandler for directory: {directory}")
        super().__init__(*args, directory=directory, **kwargs)

    def do_GET(self):
        logger.info(f"HTTP GET request: {self.path} from {self.client_address[0]}")
        try:
            super().do_GET()
        except Exception as e:
            logger.error(f"Error serving HTTP GET for {self.path}: {e}", exc_info=True)
            self.send_error(500, "Internal Server Error")

    def end_headers(self):
        # Mejora de seguridad: Añadir cabeceras HTTP recomendadas
        self.send_header('X-Content-Type-Options', 'nosniff')
        self.send_header('X-Frame-Options', 'DENY') # Previene clickjacking
        self.send_header('Cache-Control', 'no-store, no-cache, must-revalidate, max-age=0')
        self.send_header('Pragma', 'no-cache')
        self.send_header('Expires', '0')
        # Puedes eliminar o cambiar el Server header para ocultar la versión de Python
        self.server_version = "MyCustomServer/1.0"
        logger.debug("Added security headers.")
        super().end_headers()


def start_http_server(web_dir_path):
    """Inicia el servidor HTTP en un hilo separado."""
    class CustomHandler(Handler):
        def __init__(self, *args, **kwargs):
            super().__init__(*args, directory=str(web_dir_path), **kwargs)

    try:
        # Usamos TCPServer para especificar el directorio.
        # Allow reuse address to prevent "address already in use" errors after quick restarts
        socketserver.TCPServer.allow_reuse_address = True
        with socketserver.TCPServer(("", PORT), CustomHandler) as httpd:
            logger.info(f"[HTTP SERVER] Sirviendo en el puerto {PORT} desde el directorio '{web_dir_path}'")
            httpd.serve_forever()
    except Exception as e:
        logger.critical(f"[HTTP SERVER] Fallo crítico al iniciar el servidor HTTP: {e}", exc_info=True)
        # Puedes querer detener el programa principal aquí si el servidor HTTP es esencial

async def websocket_handler(websocket, path):
    """Maneja las conexiones WebSocket entrantes."""
    logger.info(f"[{threading.current_thread().name}] [WEBSOCKET_HANDLER_ENTRY] Tipo de 'websocket': {type(websocket)}, Tipo de 'path': {type(path)}, Valor de 'path': '{path}'")
    
    # Check if the arguments are what we expect
    if not isinstance(websocket, websockets.WebSocketServerProtocol):
        logger.critical(f"ERROR FATAL: 'websocket' no es una instancia de WebSocketServerProtocol. Tipo: {type(websocket)}")
        # This indicates a severe misconfiguration or an incompatible library version
        return # Cannot proceed

    if not isinstance(path, str):
        logger.critical(f"ERROR FATAL: 'path' no es una cadena. Tipo: {type(path)}, Valor: {path}")
        # This is the exact error you were getting.
        return # Cannot proceed

    logger.info(f"[WEBSOCKET] Cliente conectado desde {websocket.remote_address}, Path: '{path}'")
    CONNECTED_CLIENTS.add(websocket)
    logger.info(f"[WEBSOCKET] Clientes activos después de la conexión: {len(CONNECTED_CLIENTS)}")

    try:
        # This will keep the handler alive as long as the connection is open
        await websocket.wait_closed()
    except websockets.exceptions.ConnectionClosed as e:
        logger.info(f"[WEBSOCKET] Cliente desconectado desde {websocket.remote_address} (Código: {e.code}, Razón: '{e.reason}')")
    except Exception as e:
        logger.error(f"[WEBSOCKET] Error inesperado en el manejador de WebSocket para {websocket.remote_address}: {e}", exc_info=True)
    finally:
        if websocket in CONNECTED_CLIENTS:
            CONNECTED_CLIENTS.remove(websocket)
            logger.info(f"[WEBSOCKET] Cliente {websocket.remote_address} removido. Clientes activos: {len(CONNECTED_CLIENTS)}")
        else:
            logger.warning(f"[WEBSOCKET] Cliente {websocket.remote_address} ya no estaba en CONNECTED_CLIENTS al finalizar.")

async def start_websocket_server():
    """Inicia el servidor WebSocket."""
    logger.info(f"[WEBSOCKET] Intentando iniciar servidor WebSocket en ws://0.0.0.0:{WEBSOCKET_PORT}")
    try:
        # Use a more explicit way to start the server for better error handling visibility
        server = await websockets.serve(websocket_handler, "0.0.0.0", WEBSOCKET_PORT)
        logger.info(f"[WEBSOCKET] Servidor WebSocket escuchando en ws://0.0.0.0:{WEBSOCKET_PORT}")
        await server.wait_closed() # Keep the server running indefinitely
    except Exception as e:
        logger.critical(f"[WEBSOCKET] Fallo crítico al iniciar el servidor WebSocket: {e}", exc_info=True)
        # Re-raise or handle appropriately, as this is a core component
        raise # Propagate the error so async_main can potentially catch it

async def async_main():
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
    # Daemon=True ensures the thread exits when the main program exits.
    http_thread = threading.Thread(target=start_http_server, args=(web_dir_path,), daemon=True, name="HTTP-Server-Thread")
    http_thread.start()
    logger.info("Hilo del servidor HTTP iniciado.")


    # Get the current running event loop to pass to the Watchdog handler
    # This must be called from an async function that has an active event loop
    main_loop = asyncio.get_running_loop()
    logger.info(f"Bucle de eventos principal obtenido: {main_loop}")


    # Iniciar el servidor WebSocket como una tarea asíncrona
    websocket_server_task = asyncio.create_task(start_websocket_server(), name="WebSocket-Server-Task")
    logger.info("Tarea del servidor WebSocket creada.")

    # Configurar watchdog, pasando el bucle de eventos
    handler = JSONFileHandler(tmp_dir, main_loop, max_retries=5, retry_delay=0.2)
    observer = Observer()
    observer.schedule(handler, str(tmp_dir), recursive=False)
    observer.start()
    logger.info(f"Watchdog iniciado para monitorear '{tmp_dir}'")

    try:
        logger.info("[INFO] Esperando cambios en archivos JSON y conexiones web... Presiona Ctrl+C para salir.")
        # This future keeps the async_main loop running indefinitely.
        # It will be cancelled by KeyboardInterrupt.
        await asyncio.Future()

    except asyncio.CancelledError:
        logger.info("[INFO] Tarea principal cancelada (probablemente por KeyboardInterrupt).")
    except KeyboardInterrupt:
        logger.info("[INFO] Terminando observador y servidores debido a KeyboardInterrupt...")
    except Exception as e:
        logger.critical(f"[ERROR] Error inesperado en async_main: {e}", exc_info=True)
    finally:
        logger.info("[INFO] Deteniendo servicios...")
        observer.stop()
        observer.join() # Wait for the Watchdog thread to finish
        logger.info("Watchdog detenido.")

        # Cancel WebSocket server task gracefully
        if not websocket_server_task.done():
            websocket_server_task.cancel()
            logger.info("Tarea del servidor WebSocket cancelada.")
            try:
                await websocket_server_task # Wait for it to finish cancelling
            except asyncio.CancelledError:
                logger.info("Servidor WebSocket finalizó su cancelación.")
            except Exception as e:
                logger.error(f"Error al esperar la cancelación del servidor WebSocket: {e}", exc_info=True)
        
        logger.info("[INFO] Servidores y observadores detenidos.")

if __name__ == "__main__":
    try:
        asyncio.run(async_main())
    except Exception as main_e:
        logger.critical(f"La aplicación falló al iniciar o durante la ejecución principal: {main_e}", exc_info=True)