document.addEventListener('DOMContentLoaded', () => {
    // Referencias a elementos del DOM
    const websocketStatus = document.getElementById('websocket-status'); // Este elemento puede renombrarse si quieres (ej. 'polling-status')
    const ntpDate = document.getElementById('ntp-date');
    const ntpHour = document.getElementById('ntp-hour');
    const sensorDataList = document.getElementById('sensor-data-list');
    const alarmStatus = document.getElementById('alarm-status');
    const alarmLastEvent = document.getElementById('alarm-last-event');
    const logList = document.getElementById('log-list');

    // Estado actual de la alarma (para controlar visualmente)
    let isAlarmActive = false;

    // Función para añadir mensajes al log
    function addLog(message, type = 'INFO') {
        const li = document.createElement('li');
        const now = new Date();
        const timestamp = `[${now.toLocaleDateString('es-CO')} ${now.toLocaleTimeString('es-CO')}]`;
        li.textContent = `${timestamp} [${type.toUpperCase()}] ${message}`;
        logList.prepend(li);
        if (logList.children.length > 50) {
            logList.removeChild(logList.lastChild);
        }
    }

    // Función para activar/desactivar el estado visual de la alarma
    function setAlarmVisualState(active) {
        if (active && !isAlarmActive) { // Si la alarma se activa y no estaba activa
            alarmStatus.textContent = 'ARMED // ALERT';
            alarmStatus.className = 'display-value status-alarm-on';
            alarmLastEvent.textContent = `ALERT active as of ${new Date().toLocaleTimeString('es-CO')}`;
            addLog('Alarm state changed to ARMED.', 'ALARM');
            isAlarmActive = true;
        } else if (!active && isAlarmActive) { // Si la alarma se desactiva y estaba activa
            alarmStatus.textContent = 'DISARMED';
            alarmStatus.className = 'display-value status-alarm-off';
            alarmLastEvent.textContent = `DISARMED by external trigger at ${new Date().toLocaleTimeString('es-CO')}`;
            addLog('Alarm state changed to DISARMED.', 'ALARM');
            isAlarmActive = false;
        }
        // Si el estado no cambia, no hacemos nada para evitar spam de logs.
    }

    // --- Funciones de actualización de la UI (Mismos que antes, con pequeñas mejoras de seguridad) ---

    function updateNTPDisplay(data) {
        if (data && typeof data === 'object') {
            ntpDate.textContent = data.date || 'N/A';
            ntpHour.textContent = data.hour || 'N/A';
            addLog('NTP data updated.', 'UPDATE');
        } else {
            addLog('Invalid NTP data received.', 'ERROR');
        }
    }

    function updateSensorDisplay(data) {
        sensorDataList.innerHTML = ''; // Limpiar datos anteriores
        if (data && typeof data === 'object') {
            const sensorLabels = {
                "hum": "Humidity",
                "tempC": "Temperature (°C)",
                "mq7Adc": "MQ7 ADC"
            };

            for (const key in data) {
                // Asegúrate de que la propiedad pertenece al objeto y no es heredada
                if (Object.hasOwnProperty.call(data, key)) {
                    const value = data[key];
                    // Sanitiza el valor si fuera necesario (ej. para evitar XSS si los datos no son de confianza)
                    const sanitizedValue = (value !== null && value !== undefined) ? String(value) : 'N/A';
                    
                    const label = sensorLabels[key] || key.replace(/([A-Z])/g, ' $1').replace(/^./, str => str.toUpperCase());
                    const sensorRow = document.createElement('p');
                    sensorRow.className = 'sensor-row';
                    // Usar textContent para el valor si no esperas HTML
                    sensorRow.innerHTML = `<span>${label}:</span> <span class="display-value">${sanitizedValue}</span>`;
                    sensorDataList.appendChild(sensorRow);
                }
            }
            addLog('Sensor data updated.', 'UPDATE');
        } else {
            addLog('Invalid Sensor data received.', 'ERROR');
        }
    }

    function updateAlarmDisplay(data) {
        if (data && typeof data === 'object' && 'alarm_stopped' in data) {
            // Si alarm_stopped es true, la alarma está desactivada
            setAlarmVisualState(!data.alarm_stopped); // true -> false (desactivada), false -> true (activada)
            addLog(`Alarm stopped status received: ${data.alarm_stopped}`, 'ALARM_EVENT');
        } else {
            addLog('Invalid Alarm data received.', 'ERROR');
        }
    }

    // --- Código para Polling ---

    // Función para obtener datos del servidor vía HTTP
    async function fetchData() {
        try {
            // Asumiendo que el servidor HTTP está en el mismo host y puerto
            const response = await fetch('/data'); // Solicita datos al nuevo endpoint /data
            if (!response.ok) {
                // Actualiza el estado visual si hay un error HTTP
                websocketStatus.textContent = '[ STATUS // ERROR // HTTP ]';
                websocketStatus.className = 'status-disconnected';
                addLog(`HTTP Error fetching data: ${response.status} ${response.statusText}`, 'ERROR');
                throw new Error(`HTTP error! status: ${response.status}`);
            }
            const data = await response.json();
            console.log('Datos recibidos por polling:', data);
            addLog(`Data fetched from /data endpoint. (${Object.keys(data).length} files)`, 'POLLING');

            // Actualiza el estado del "WebSocket" para reflejar el estado del Polling
            websocketStatus.textContent = '[ STATUS // POLLING // ONLINE ]';
            websocketStatus.className = 'status-connected';

            // 'data' ahora contendrá un objeto donde las claves son los nombres de archivo
            // y los valores son objetos con 'state' y 'data'.
            // Ejemplo: data = { "ntp.json": { "state": "NTP_UPDATE", "data": {...} }, "sensor.json": { "state": "SENSOR_UPDATE", "data": {...} } }

            // Recorre todos los datos recibidos y actualiza la UI para cada tipo
            if (data['ntp.json']) {
                updateNTPDisplay(data['ntp.json'].data);
            }
            if (data['sensor.json']) {
                updateSensorDisplay(data['sensor.json'].data);
            }
            if (data['alarm.json']) {
                updateAlarmDisplay(data['alarm.json'].data);
            }

        } catch (error) {
            console.error('Error al obtener datos por polling:', error);
            // Si hay un error de red o parsing, actualiza el estado visual
            websocketStatus.textContent = '[ STATUS // POLLING // ERROR ]';
            websocketStatus.className = 'status-disconnected';
            addLog(`Polling Error: ${error.message}`, 'CRITICAL');
        }
    }

    // Iniciar el polling cuando la página cargue
    // Usaremos un intervalo razonable, ej. cada 1 segundo (1000ms)
    const pollingIntervalMs = 1000; 
    
    // Inicializar visualmente con mensajes de espera
    websocketStatus.textContent = '[ STATUS // INIT // POLLING... ]';
    addLog('Client interface loaded. Starting polling.', 'INFO');

    // Realiza una primera solicitud de inmediato para cargar los datos iniciales
    fetchData(); 

    // Luego, iniciar el polling regular
    setInterval(fetchData, pollingIntervalMs);
    console.log(`Polling iniciado cada ${pollingIntervalMs / 1000} segundos.`);

    // Simular que la alarma está activa inicialmente para probar la desactivación.
    // Esto es solo para la demostración visual, no depende de un archivo.
    setAlarmVisualState(true); 
});