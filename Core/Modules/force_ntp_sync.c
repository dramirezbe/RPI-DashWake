#include "force_ntp_sync.h"
#include <stdio.h>   // Para printf, fprintf
#include <stdlib.h>  // Para system(), WIFEXITED, WEXITSTATUS
#include <errno.h>   // Para errno

/**
 * @brief Implementación de la función para forzar la sincronización NTP del sistema.
 */
int force_system_ntp_sync(void) {
    printf("[NTP Sync] Intentando forzar la sincronización del reloj del sistema con NTP...\n");

    // Comando preferido para Raspberry Pi OS: reiniciar systemd-timesyncd
    // Esto fuerza al demonio del sistema a resincronizar la hora.
    const char *command = "sudo systemctl restart systemd-timesyncd";

    int status = system(command);

    if (status == -1) {
        perror("[NTP Sync Error] Error al ejecutar el comando system()");
        return -1; // Error al lanzar el comando
    } else {
        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            if (exit_status == 0) {
                printf("[NTP Sync] Comando '%s' ejecutado exitosamente. La sincronización debería estar en curso.\n", command);
                return 0; // Comando exitoso
            } else {
                fprintf(stderr, "[NTP Sync Error] El comando '%s' falló con código de salida: %d\n", command, exit_status);
                fprintf(stderr, "[NTP Sync Error] Asegúrate de que 'systemd-timesyncd' esté instalado y que el programa tenga permisos de 'sudo'.\n");
                fprintf(stderr, "[NTP Sync Error] También verifica la conectividad a internet y la accesibilidad a servidores NTP.\n");
                return exit_status; // Retorna el código de salida del comando
            }
        } else {
            fprintf(stderr, "[NTP Sync Error] El comando '%s' no terminó normalmente.\n", command);
            return -2; // El comando no terminó normalmente
        }
    }
}