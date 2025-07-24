#ifndef FORCE_NTP_SYNC_H
#define FORCE_NTP_SYNC_H

/**
 * @brief Intenta forzar una sincronización de la hora del sistema utilizando NTP.
 *
 * Esta función ejecuta un comando del sistema para reiniciar el servicio
 * systemd-timesyncd, lo que fuerza una resincronización de la hora.
 * Requiere que el programa se ejecute con permisos de root (sudo) para
 * que el comando tenga éxito.
 *
 * @return 0 si el comando se ejecutó exitosamente y se presume que la
 * sincronización se inició.
 * @return -1 si hubo un error al ejecutar el comando (ej., no se pudo iniciar).
 * @return Un valor positivo si el comando se ejecutó pero retornó un código
 * de salida distinto de cero (indicando un posible fallo en la
 * sincronización o un problema con el servicio).
 */
int force_system_ntp_sync(void);

#endif // FORCE_NTP_SYNC_H