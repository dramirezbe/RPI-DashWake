import os
import json
import time
from pathlib import Path
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler

class StateMachine:
    """
    Máquina de estados simplificada: 
    mapea cada nombre de archivo JSON a un estado lógico.
    """
    # Definimos los estados posibles
    STATE_MAP = {
        'ntp.json':       'NTP_UPDATE',
        'sensor.json':    'SENSOR_UPDATE',
        'alarm.json':     'ALARM_STOP',
    }

    @classmethod
    def get_state(cls, filename: str) -> str:
        """
        Dado un nombre de archivo, devuelve el estado asociado.
        Si no lo conoce, devuelve 'UNKNOWN'.
        """
        return cls.STATE_MAP.get(filename, 'UNKNOWN')

class JSONFileHandler(FileSystemEventHandler):
    def __init__(self, tmp_path, debounce_time=0.5):
        super().__init__()
        self.tmp_path = tmp_path
        self.last_modified = {}  # Guarda timestamp por archivo
        self.debounce_time = debounce_time

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

        # Cargamos el JSON
        try:
            with open(event.src_path, 'r') as f:
                data = json.load(f)
        except Exception as e:
            print(f"[ERROR] Leyendo {event.src_path}: {e}")
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
    handler = JSONFileHandler(tmp_dir)
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
