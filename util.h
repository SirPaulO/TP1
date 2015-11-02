#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>
#include <string.h>

/* Codigos de error escritos user-friendly*/
typedef enum {
    LECTURA,
    MESA_ABIERTA,
    MESA_CERRADA,
    NUMERO_NEGATIVO,
    VOTO_REALIZADO,
    NO_ENPADRONADO,
    NO_VOTANTES,
    NO_DESHACER,
    FALTA_VOTAR,
    OTRO,
    COLA_NO_VACIA
} error_code;

/* Imprime codigo de error */
bool error_manager(error_code code);
/* Copia la clave en memoria */
char* copiar_clave(const char *clave);

/* Obtiene cantidad de columnas de una cadena, separadas por el parametro separador */
size_t obtener_cantidad_columnas(char* cadena, char separador);

#endif // UTIL_H

