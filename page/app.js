document.addEventListener('DOMContentLoaded', () => {
    const connectionStatus = document.getElementById('connection-status');
    const dateDisplay = document.getElementById('date-display');
    const hourDisplay = document.getElementById('hour-display');
    const alarmDisplay = document.getElementById('alarm-display');
    const humidityDisplay = document.getElementById('humidity-display');
    const temperatureDisplay = document.getElementById('temperature-display');
    const airQualityDisplay = document.getElementById('air-quality-display');
    const toggleAlarmBtn = document.getElementById('toggle-alarm-btn');
    const logList = document.getElementById('log-list');

    // Función para añadir mensajes al log
    const addLog = (message, isError = false) => {
        const listItem = document.createElement('li');
        const timestamp = new Date().toLocaleTimeString('es-ES', { hour: '2-digit', minute: '2-digit', second: '2-digit' });
        listItem.textContent = `[${timestamp}] ${message}`;
        if (isError) {
            listItem.style.color = 'var(--error-color)';
        }
        logList.appendChild(listItem);
        // Scroll al final del log
        logList.scrollTop = logList.scrollHeight;
    };

    let ws; // Variable para almacenar la instancia del WebSocket

    const connectWebSocket = () => {
        addLog('Attempting to connect to WebSocket...');
        // Asegúrate de que la URL (ws://localhost:8080) coincida con la dirección de tu servidor C
        ws = new WebSocket('ws://localhost:8080');

        ws.onopen = () => {
            connectionStatus.textContent = 'ONLINE';
            connectionStatus.classList.remove('status-disconnected');
            connectionStatus.classList.add('status-connected');
            addLog('WebSocket connection established.');
            // console.log('WebSocket connection established.');
        };

        ws.onmessage = (event) => {
            // console.log('Message from server:', event.data);
            try {
                const data = JSON.parse(event.data);
                // console.log('Parsed JSON:', data);

                // Actualizar Fecha y Hora
                dateDisplay.textContent = data.date || '--/--/----';
                hourDisplay.textContent = data.hour || '--:--:--';

                // Actualizar Alarma
                if (data.alarm) {
                    alarmDisplay.textContent = data.alarm;
                    if (data.alarm === 'ACTIVADA') {
                        alarmDisplay.classList.remove('status-alarm-off');
                        alarmDisplay.classList.add('status-alarm-on');
                    } else {
                        alarmDisplay.classList.remove('status-alarm-on');
                        alarmDisplay.classList.add('status-alarm-off');
                    }
                }

                // Actualizar Sensores
                if (data.sensors) {
                    // Formato: |hum|temp|aire
                    const sensorValues = data.sensors.split('|').filter(Boolean); // Filter(Boolean) elimina strings vacías
                    if (sensorValues.length === 3) {
                        humidityDisplay.textContent = `${sensorValues[0]}%`;
                        temperatureDisplay.textContent = `${sensorValues[1]}°C`;
                        airQualityDisplay.textContent = sensorValues[2];
                    } else {
                        addLog('Sensor data format error.', true);
                    }
                }

            } catch (e) {
                addLog(`Error parsing JSON: ${e.message} - Data: ${event.data}`, true);
                // console.error('Error parsing JSON:', e, 'Data:', event.data);
            }
        };

        ws.onclose = (event) => {
            connectionStatus.textContent = 'OFFLINE';
            connectionStatus.classList.remove('status-connected');
            connectionStatus.classList.add('status-disconnected');
            addLog(`WebSocket connection closed. Code: ${event.code}, Reason: ${event.reason}. Reconnecting in 3 seconds...`, true);
            // console.warn('WebSocket connection closed:', event);
            // Intentar reconectar después de un breve retraso
            setTimeout(connectWebSocket, 3000);
        };

        ws.onerror = (error) => {
            addLog(`WebSocket error: ${error.message || 'Unknown error'}`, true);
            // console.error('WebSocket error:', error);
            ws.close(); // Forzar el cierre para intentar reconectar
        };
    };

    // Manejar el botón de toggle de alarma
    toggleAlarmBtn.addEventListener('click', () => {
        if (ws && ws.readyState === WebSocket.OPEN) {
            ws.send('toggle_alarm'); // Enviar un mensaje al servidor para que cambie el estado
            addLog('Sent "toggle_alarm" command to server.');
        } else {
            addLog('Cannot toggle alarm: WebSocket is not open.', true);
        }
    });

    // Iniciar la conexión cuando la página cargue
    connectWebSocket();
});