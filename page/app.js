document.addEventListener('DOMContentLoaded', () => {
    // Referencias a elementos del DOM
    const pollingStatus = document.getElementById('websocket-status'); // Renombrado para mayor claridad
    const ntpDate = document.getElementById('ntp-date');
    const ntpHour = document.getElementById('ntp-hour');
    const sensorDataList = document.getElementById('sensor-data-list');
    const alarmStatus = document.getElementById('alarm-status');
    const alarmLastEvent = document.getElementById('alarm-last-event');
    const logList = document.getElementById('log-list');
    const toggleAlarmBtn = document.getElementById('toggle-alarm-btn'); // NUEVO: Referencia al botón

    // Estado local de la alarma (para controlar visualmente)
    // Se inicializa en un estado desconocido o por defecto (desactivado)
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
    // `active` significa que la alarma está "armada/sonando"
    // `false` significa que está "desarmada/detenida"
    function setAlarmVisualState(active, source = 'UI') { // Añadimos 'source' para el log
        if (active && !isAlarmActive) { 
            alarmStatus.textContent = 'ARMED // ALERT';
            alarmStatus.className = 'display-value status-alarm-on';
            alarmLastEvent.textContent = `ALERT active as of ${new Date().toLocaleTimeString('es-CO')}`;
            toggleAlarmBtn.textContent = 'Deactivate Alarm'; // Texto del botón
            addLog(`Alarm state changed to ARMED (Source: ${source}).`, 'ALARM');
            isAlarmActive = true;
        } else if (!active && isAlarmActive) {
            alarmStatus.textContent = 'DISARMED';
            alarmStatus.className = 'display-value status-alarm-off';
            alarmLastEvent.textContent = `DISARMED by ${source} at ${new Date().toLocaleTimeString('es-CO')}`;
            toggleAlarmBtn.textContent = 'Activate Alarm'; // Texto del botón
            addLog(`Alarm state changed to DISARMED (Source: ${source}).`, 'ALARM');
            isAlarmActive = false;
        } else if (active && isAlarmActive) {
            // Si ya está activa y se intenta activar de nuevo, solo actualiza el botón
            toggleAlarmBtn.textContent = 'Deactivate Alarm';
        } else if (!active && !isAlarmActive) {
            // Si ya está inactiva y se intenta desactivar de nuevo, solo actualiza el botón
            toggleAlarmBtn.textContent = 'Activate Alarm';
        }
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
            const sensorLabels = {
                "hum": "Humidity",
                "tempC": "Temperature (°C)",
                "mq7Adc": "MQ7 ADC"
            };

            for (const key in data) {
                if (Object.hasOwnProperty.call(data, key)) {
                    let value = data[key];
                    let displayValue;

                    if (key === "tempC" && typeof value === 'number') {
                        displayValue = value.toFixed(1); // Redondea a un decimal
                    } else {
                        displayValue = (value !== null && value !== undefined) ? String(value) : 'N/A';
                    }
                    
                    const label = sensorLabels[key] || key.replace(/([A-Z])/g, ' $1').replace(/^./, str => str.toUpperCase());
                    const sensorRow = document.createElement('p');
                    sensorRow.className = 'sensor-row';
                    sensorRow.innerHTML = `<span>${label}:</span> <span class="display-value">${displayValue}</span>`;
                    sensorDataList.appendChild(sensorRow);
                }
            }
            addLog('Sensor data updated.', 'UPDATE');
        } else {
            addLog('Invalid Sensor data received.', 'ERROR');
        }
    }

    function updateAlarmDisplay(data) {
        // La información de alarm.json SIEMPRE es {"alarm_stopped": true}.
        // Esto significa que, cada vez que llega un polling con alarm.json,
        // la alarma visual debe RESETEARSE a 'DESARMED' si no estaba ya así.
        if (data && typeof data === 'object' && 'alarm_stopped' in data && data.alarm_stopped === true) {
            // Forzar el estado visual a DESARMED si no lo está ya, basado en el JSON
            setAlarmVisualState(false, 'JSON Reset'); 
            addLog(`Alarm reset to DISARMED by JSON. (alarm_stopped: ${data.alarm_stopped})`, 'ALARM_EVENT');
        } else {
            // Si alarm.json no existe o no tiene el formato esperado, también forzar a inactivo
            setAlarmVisualState(false, 'JSON Init/Error');
            addLog('Invalid or missing alarm.json. Alarm forced to DISARMED.', 'INFO');
        }
    }

    // --- Código para Polling ---

    // Función para obtener datos del servidor vía HTTP
    async function fetchData() {
        try {
            const response = await fetch('/data');
            if (!response.ok) {
                pollingStatus.textContent = '[ STATUS // ERROR // HTTP ]';
                pollingStatus.className = 'status-disconnected';
                addLog(`HTTP Error fetching data: ${response.status} ${response.statusText}`, 'ERROR');
                throw new Error(`HTTP error! status: ${response.status}`);
            }
            const data = await response.json();
            console.log('Datos recibidos por polling:', data);
            pollingStatus.textContent = '[ STATUS // POLLING // ONLINE ]';
            pollingStatus.className = 'status-connected';
            addLog(`Data fetched from /data endpoint. (${Object.keys(data).length} files)`, 'POLLING');

            if (data['ntp.json']) {
                updateNTPDisplay(data['ntp.json'].data);
            }
            if (data['sensor.json']) {
                updateSensorDisplay(data['sensor.json'].data);
            }
            // La actualización de la alarma se basa EXCLUSIVAMENTE en el JSON
            // Se asume que alarm.json siempre mandará {"alarm_stopped": true}
            updateAlarmDisplay(data['alarm.json']); 
            
        } catch (error) {
            console.error('Error al obtener datos por polling:', error);
            pollingStatus.textContent = '[ STATUS // POLLING // ERROR ]';
            pollingStatus.className = 'status-disconnected';
            addLog(`Polling Error: ${error.message}`, 'CRITICAL');
        }
    }

    // --- Event Listener para el botón de alarma ---
    if (toggleAlarmBtn) {
        toggleAlarmBtn.addEventListener('click', () => {
            // Invierte el estado *interno* de la alarma y actualiza la UI
            // NO se comunica con el servidor ni modifica alarm.json
            setAlarmVisualState(!isAlarmActive, 'Button Click');
        });
    } else {
        addLog("Error: 'toggle-alarm-btn' not found in DOM.", "CRITICAL");
        console.error("Button with ID 'toggle-alarm-btn' not found.");
    }

    // Iniciar el polling
    const pollingIntervalMs = 1000; 
    pollingStatus.textContent = '[ STATUS // INIT // POLLING... ]';
    addLog('Client interface loaded. Starting polling.', 'INFO');

    // Realiza una primera solicitud de inmediato para cargar los datos iniciales
    fetchData(); 

    // Luego, iniciar el polling regular
    setInterval(fetchData, pollingIntervalMs);
    console.log(`Polling iniciado cada ${pollingIntervalMs / 1000} segundos.`);

});