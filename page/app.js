document.addEventListener('DOMContentLoaded', () => {
    // Referencias a elementos del DOM
    const websocketStatus = document.getElementById('websocket-status');
    const ntpDate = document.getElementById('ntp-date');
    const ntpHour = document.getElementById('ntp-hour');
    const sensorDataList = document.getElementById('sensor-data-list');
    const alarmStatus = document.getElementById('alarm-status');
    const alarmLastEvent = document.getElementById('alarm-last-event');
    // const toggleAlarmBtn = document.getElementById('toggle-alarm-btn'); // Eliminado
    const logList = document.getElementById('log-list');

    // Configuración de WebSocket
    const websocketUrl = `ws://${window.location.hostname}:8001`; 
    let ws;

    // Estado actual de la alarma (para controlar visualmente)
    let isAlarmActive = false;
    let alarmInterval = null; // Para el parpadeo

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

    // Función para activar/desactivar el parpadeo de la alarma
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


    // Función para conectar al WebSocket
    function connectWebSocket() {
        ws = new WebSocket(websocketUrl);

        ws.onopen = (event) => {
            console.log('[WEBSOCKET] Connection opened:', event);
            websocketStatus.textContent = '[ STATUS // ONLINE ]';
            websocketStatus.className = 'status-connected';
            addLog('WebSocket connection established.', 'SUCCESS');
            // Al conectar, asumimos alarma desactivada hasta recibir datos
            setAlarmVisualState(false); 
        };

        ws.onmessage = (event) => {
            try {
                const message = JSON.parse(event.data);
                console.log('[WEBSOCKET] Message received:', message);
                addLog(`Data received for ${message.filename} (State: ${message.state})`, 'DATA');
                
                // Actualizar la interfaz de usuario según el archivo JSON recibido
                switch (message.state) {
                    case 'NTP_UPDATE':
                        updateNTPDisplay(message.data);
                        break;
                    case 'SENSOR_UPDATE':
                        updateSensorDisplay(message.data);
                        break;
                    case 'ALARM_STOP':
                        updateAlarmDisplay(message.data);
                        break;
                    default:
                        addLog(`Unknown state: ${message.state} for ${message.filename}`, 'WARNING');
                        break;
                }
            } catch (e) {
                addLog(`Error parsing WebSocket message: ${e.message}. Raw: ${event.data.substring(0, 100)}...`, 'ERROR');
                console.error('Error parsing message:', e, event.data);
            }
        };

        ws.onclose = (event) => {
            console.warn('[WEBSOCKET] Connection closed:', event);
            websocketStatus.textContent = '[ STATUS // OFFLINE // RECONNECTING... ]';
            websocketStatus.className = 'status-disconnected';
            addLog('WebSocket connection closed. Attempting to reconnect in 3 seconds...', 'ERROR');
            setTimeout(connectWebSocket, 3000); // Intenta reconectar después de 3 segundos
        };

        ws.onerror = (error) => {
            console.error('[WEBSOCKET] Error:', error);
            websocketStatus.textContent = '[ STATUS // ERROR // CHECK CONSOLE ]';
            websocketStatus.className = 'status-disconnected';
            addLog('WebSocket error. Check console for details.', 'CRITICAL');
            ws.close(); // Cierra para forzar un reintento de conexión
        };
    }

    // --- Funciones de actualización de la UI ---

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
            // Mapping para nombres legibles
            const sensorLabels = {
                "hum": "Humidity",
                "tempC": "Temperature (°C)",
                "mq7Adc": "MQ7 ADC"
            };

            for (const key in data) {
                if (Object.hasOwnProperty.call(data, key)) {
                    const value = data[key];
                    const label = sensorLabels[key] || key.replace(/([A-Z])/g, ' $1').replace(/^./, str => str.toUpperCase()); // Formato para otras claves
                    const sensorRow = document.createElement('p');
                    sensorRow.className = 'sensor-row';
                    sensorRow.innerHTML = `<span>${label}:</span> <span class="display-value">${value}</span>`;
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

    // --- Event Listeners (Solo si no hay envío de comandos) ---
    // Si el botón de alarma no envía comandos, se elimina su event listener.
    // toggleAlarmBtn.addEventListener('click', () => { ... });

    // Iniciar la conexión WebSocket cuando el DOM esté completamente cargado
    connectWebSocket();

    // Inicializar visualmente con mensajes de espera
    websocketStatus.textContent = '[ STATUS // INIT // CONNECTING... ]';
    addLog('Client interface loaded.', 'INFO');

    // Simular que la alarma está activa inicialmente para probar la desactivación.
    // Esto es solo para la demostración visual, no depende de un archivo.
    setAlarmVisualState(true); 
});