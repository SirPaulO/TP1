#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>


/* Imprime codigo de error */
bool error_manager(int code) {
    /*
    ERROR1: si hubo un error en la lectura de los archivos (o archivos inexistentes).
    ERROR2: si la mesa ya estaba previamente abierta
    ERROR3: si la mesa no está abierta.
    ERROR4: si el número de DNI es menor o igual a 0.
    ERROR5: si la persona ya votó antes.
    ERROR6: si el número y tipo de documento no está en el archivo padrón.csv.
    ERROR8: en caso de que no existan operaciones para deshacer.
    ERROR7: en caso de que no existan votantes en espera.
    ERROR9: en caso de que no se hayan elegido todas las categorías.
    ERROR10: en cualquier otro caso no contemplado.
    ERROR11: En caso de que aún queden votantes ingresados sin emitir su voto
    */
    printf("ERROR%d\n", code+1);
    return false;
}

/* Copia la clave en memoria */
char* copiar_clave(const char *clave) {
    char* clave_copiada = malloc(sizeof(char) * strlen(clave)+1);
    strcpy(clave_copiada, clave);
    return clave_copiada;
}

/* Obtiene cantidad de columnas de una cadena, separadas por el parametro separador */
size_t obtener_cantidad_columnas(char* cadena, char separador) {
    char* caracter = cadena;
    size_t conteo = 1;

    while ((caracter) && (*caracter) && (*caracter != '\n') ) {
        caracter++;
        if (*caracter == separador && *(caracter+1) && *(caracter+1) != separador)
            conteo++;
    }
    return conteo;
}
