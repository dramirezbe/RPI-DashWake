import os
import json
import time
from pathlib import Path
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler
from json.decoder import JSONDecodeError # Import this specific error

class StateMachine:
    """
    Máquina de estados simplificada: 
    mapea cada nombre de archivo JSON a un estado lógico.
    """
    # Definimos los estados posibles
    STATE_MAP = {
        'ntp.json':      'NTP_UPDATE',
        'sensor.json':   'SENSOR_UPDATE',
        'alarm.json':    'ALARM_STOP',
    }

    @classmethod
    def get_state(cls, filename: str) -> str:
        """
        Dado un nombre de archivo, devuelve el estado asociado.
        Si no lo conoce, devuelve 'UNKNOWN'.
        """
        return cls.STATE_MAP.get(filename, 'UNKNOWN')

class JSONFileHandler(FileSystemEventHandler):
    def __init__(self, tmp_path, debounce_time=0.5, max_retries=3, retry_delay=0.1):
        super().__init__()
        self.tmp_path = tmp_path
        self.last_modified = {}  # Guarda timestamp por archivo
        self.debounce_time = debounce_time
        self.max_retries = max_retries
        self.retry_delay = retry_delay

    def _read_json_with_retries(self, file_path: str) -> dict | None:
        """
        Intenta leer un archivo JSON con reintentos en caso de JSONDecodeError.
        """
        for attempt in range(self.max_retries):
            try:
                with open(file_path, 'r') as f:
                    data = json.load(f)
                return data # Successfully read, return data
            except JSONDecodeError as e:
                # This specifically catches JSON parsing errors
                print(f"[ERROR] Intento {attempt + 1}/{self.max_retries} - Leyendo {file_path}: {e}")
                if attempt < self.max_retries - 1:
                    time.sleep(self.retry_delay) # Wait before retrying
                else:
                    print(f"[ERROR] Fallo al leer {file_path} después de {self.max_retries} intentos.")
                    return None # Failed after all retries
            except FileNotFoundError:
                print(f"[ERROR] Archivo no encontrado: {file_path}")
                return None
            except Exception as e:
                # Catch other potential file reading errors
                print(f"[ERROR] Un error inesperado ocurrió al leer {file_path}: {e}")
                return None
        return None # Should not be reached if max_retries is > 0 and loop completes

    def on_modified(self, event):
        # Filtramos sólo archivos JSON
        if event.is_directory or not event.src_path.endswith('.json'):
            return

        # Debounce: ignoramos dobles eventos muy cercanos
        now = time.time()
        last = self.last_modified.get(event.src_path, 0)
        if now - last < self.debounce_time:
            return
        self.last_modified[event.src_path] = now

        # Cargamos el JSON con reintentos
        data = self._read_json_with_retries(event.src_path)

        if data is None:
            # If data is None, it means reading failed after all retries
            return

        # Determinamos el nombre del archivo (solo el basename)
        filename = os.path.basename(event.src_path)
        # Obtenemos el estado según la máquina
        state = StateMachine.get_state(filename)

        # Imprimimos distintos mensajes según el estado
        print(f"[WATCHDOG] Archivo modificado: {filename}")
        if state == 'NTP_UPDATE':
            print("[EVENT] Estado: NTP_UPDATE → preparándose para enviar JSON de NTP")
        elif state == 'SENSOR_UPDATE':
            print("[EVENT] Estado: SENSOR_UPDATE → preparándose para enviar datos de sensor")
        elif state == 'ALARM_STOP':
            print("[EVENT] Estado: ALARM_STOP → preparándose para enviar alarma STOP")
        else:
            print(f"[EVENT] Estado: UNKNOWN para '{filename}'")

        # Simulación de envío por websocket
        print(f"[SIMULACIÓN WEBSOCKET] Enviando JSON (estado={state}):")
        print(json.dumps(data, indent=2))
        print()  # Línea en blanco para separación

def main():
    # Ruta del script
    current_file = Path(__file__).resolve()
    print(f"[INFO] Script ejecutado desde: {current_file}")

    # Directorio raíz del proyecto (subimos un nivel)
    project_root = current_file.parent.parent
    print(f"[INFO] Directorio raíz del proyecto: {project_root}")

    # Carpeta tmp
    tmp_dir = project_root / 'tmp'
    print(f"[INFO] Observando carpeta: {tmp_dir}")

    if not tmp_dir.exists():
        print(f"[ERROR] La carpeta tmp no existe en: {tmp_dir}")
        return

    # Configuramos watchdog
    # You can adjust max_retries and retry_delay here if needed
    handler = JSONFileHandler(tmp_dir, max_retries=5, retry_delay=0.2) 
    observer = Observer()
    observer.schedule(handler, str(tmp_dir), recursive=False)
    observer.start()

    try:
        print("[INFO] Esperando cambios en archivos JSON...")
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("[INFO] Terminando observador...")
        observer.stop()
    observer.join()

if __name__ == "__main__":
    main()