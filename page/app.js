document.addEventListener('DOMContentLoaded', () => {
    // Referencias a elementos del DOM
    const pollingStatus = document.getElementById('websocket-status');
    const ntpDate = document.getElementById('ntp-date');
    const ntpHour = document.getElementById('ntp-hour');
    const sensorDataList = document.getElementById('sensor-data-list');
    const alarmStatus = document.getElementById('alarm-status');
    const alarmLastEvent = document.getElementById('alarm-last-event');
    const logList = document.getElementById('log-list');
    const toggleAlarmBtn = document.getElementById('toggle-alarm-btn');

    // Estado local de la alarma (para controlar visualmente)
    // Se inicializa en 'false' (DISARMED) por defecto.
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

    // Función para actualizar el estado visual de la alarma
    // `active` true para ARMED, false para DISARMED.
    // Esta función solo cambia el estado si es diferente para evitar spam de logs.
    function setAlarmVisualState(active, source = 'UI') {
        if (active && !isAlarmActive) {
            // Activar alarma si no estaba activa
            alarmStatus.textContent = 'ARMED // ALERT';
            alarmStatus.className = 'display-value status-alarm-on';
            alarmLastEvent.textContent = `ALERT active as of ${new Date().toLocaleTimeString('es-CO')}`;
            toggleAlarmBtn.textContent = 'Deactivate Alarm'; 
            addLog(`Alarm state changed to ARMED (Source: ${source}).`, 'ALARM');
            isAlarmActive = true;
        } else if (!active && isAlarmActive) {
            // Desactivar alarma si estaba activa
            alarmStatus.textContent = 'DISARMED';
            alarmStatus.className = 'display-value status-alarm-off';
            alarmLastEvent.textContent = `DISARMED by ${source} at ${new Date().toLocaleTimeString('es-CO')}`;
            toggleAlarmBtn.textContent = 'Activate Alarm'; 
            addLog(`Alarm state changed to DISARMED (Source: ${source}).`, 'ALARM');
            isAlarmActive = false;
        }
        // Si el estado es el mismo, solo nos aseguramos de que el texto del botón sea correcto.
        else if (active && isAlarmActive) {
            toggleAlarmBtn.textContent = 'Deactivate Alarm';
        } else if (!active && !isAlarmActive) {
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

    // Esta función es la encargada de DESACTIVAR la alarma si el JSON lo indica y está activa.
    // "no me importa cómo llegue, lo importante es que llegue"
    function updateAlarmDisplay(alarmJsonData) { // alarmJsonData ahora es el objeto que contiene 'state' y 'data'
        // Primero, verificamos si la entrada 'alarm.json' existe en la respuesta del polling y es un objeto válido.
        if (alarmJsonData && typeof alarmJsonData === 'object' && 'data' in alarmJsonData) { // Asegurarse de que 'data' anidada existe
            // Logueamos el contenido real del JSON para confirmación de que llegó y cómo llegó.
            addLog(`Received alarm.json data: ${JSON.stringify(alarmJsonData)}.`, 'ALARM_JSON_DATA');

            // Acceder a la propiedad anidada: alarmJsonData.data.alarm_stopped
            const alarmStoppedValue = alarmJsonData.data.alarm_stopped;

            // Ahora, solo si la propiedad anidada 'alarm_stopped' es explícitamente true
            if (alarmStoppedValue === true) {
                // Y si la alarma de la UI está ARMED, la desactiva.
                if (isAlarmActive) {
                    setAlarmVisualState(false, 'JSON Override');
                    addLog('Alarm forced to DISARMED by `alarm_stopped: true` from JSON.', 'ALARM_EVENT');
                } else {
                    // Si ya está desarmada y el JSON dice true, no hacemos nada.
                    addLog('Alarm already DISARMED. `alarm_stopped: true` received from JSON but no change needed.', 'INFO');
                }
            } else {
                // Si 'alarm_stopped' no es true (ej. false, null, undefined, u otro tipo),
                // o si la propiedad no existe dentro de 'data', no se desactiva. Solo se ignora su contenido para el estado de la alarma.
                addLog('`alarm_stopped` is not `true` or missing in `alarm.json` data. UI state not affected.', 'INFO');
            }
        } else {
            // Si alarmJsonData no es un objeto válido o no tiene la clave 'data'
            // No es una señal de alarma válida para activar/desactivar.
            addLog('`alarm.json` entry in polling response is missing, malformed, or missing `data` key. UI state not affected.', 'WARN');
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
            // La actualización de la alarma: el JSON puede DESACTIVARLA si está activa
            // Le pasamos el objeto 'alarm.json' tal como viene del servidor
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
            // El botón ahora alterna el estado de la alarma.
            setAlarmVisualState(!isAlarmActive, 'Button Click');
        });
        // Inicializa el estado visual y el texto del botón al cargar la página a "DISARMED"
        setAlarmVisualState(isAlarmActive, 'Initial Load');
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