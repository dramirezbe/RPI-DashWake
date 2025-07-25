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
    // Inicialmente desactivada, el primer polling la confirmará.
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
    // Esta función SÓLO cambia el estado si es diferente.
    function setAlarmVisualState(active, source = 'UI') {
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
        }
        // Si el estado es el mismo, no hacemos nada (evita logs y actualizaciones innecesarias)
        // Solo nos aseguramos de que el texto del botón sea correcto en caso de que la UI se haya cargado
        // y el estado local ya coincida.
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

    function updateAlarmDisplay(data) {
        // La información de alarm.json SIEMPRE es {"alarm_stopped": true}.
        // Esto significa que, si alarm.json *confirma* que está detenida (true),
        // entonces la UI de la alarma DEBE estar desactivada.
        // PERO si el alarm.json no se encuentra o no tiene el formato esperado,
        // NO debe cambiar el estado actual de la alarma visual.
        if (data && typeof data === 'object' && 'alarm_stopped' in data) {
            const jsonAlarmStopped = data.alarm_stopped;

            if (jsonAlarmStopped === true) {
                // Si el JSON dice que la alarma está "parada", entonces la UI debe estar "desarmada".
                // Esto es el "reset" o "apagado" de la alarma.
                setAlarmVisualState(false, 'JSON Control');
                addLog(`Alarm forced to DISARMED by JSON. (alarm_stopped: ${jsonAlarmStopped})`, 'ALARM_EVENT');
            }
            // Si jsonAlarmStopped es false, o cualquier otro valor, no hacemos nada aquí
            // porque la única acción del JSON es desactivar la alarma.
            // Si el JSON viene con `false` (que no debería según tu descripción)
            // o no se encuentra el archivo, la alarma visual se mantiene con el estado que le dio el botón.
        } else {
            // Esto ocurre si alarm.json no se encuentra o no tiene el formato esperado.
            // En este caso, no "reseteamos" la alarma, sino que dejamos que el estado del botón la controle.
            // Solo lo logueamos para depuración.
            if (!data) {
                addLog('alarm.json data not found in polling response. UI state unchanged.', 'WARN');
            } else {
                addLog('Invalid Alarm data received from JSON. UI state unchanged.', 'WARN');
            }
            // Si es la primera carga y no hay alarm.json, inicializamos el botón
            if (!isAlarmActive && alarmStatus.textContent.includes('Awaiting')) {
                 setAlarmVisualState(false, 'Initial Load');
            }
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

            // La lógica de la alarma: el JSON solo sirve para "apagarla"
            // Si alarm.json no está presente o es inválido, updateAlarmDisplay
            // no forzará la desactivación.
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
            // El botón SIEMPRE activa la alarma si está desactivada.
            // Si ya está activa, no hace nada porque solo alarm.json la desactiva.
            if (!isAlarmActive) {
                setAlarmVisualState(true, 'Button Click');
            } else {
                // Si el botón se presiona cuando ya está activa, no hace nada,
                // solo se asegura de que el texto del botón sea "Deactivate Alarm".
                // La desactivación real vendrá del polling de alarm.json
                toggleAlarmBtn.textContent = 'Deactivate Alarm';
                addLog('Alarm already active. Waiting for JSON reset.', 'INFO');
            }
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