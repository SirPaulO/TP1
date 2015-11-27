#ifndef ARCHIVOS_H
#define ARCHIVOS_H

#include <stdbool.h>
#include <stdlib.h>

#include "lista.h"
#include "parser.h"

typedef struct maquina_votacion maquina_votacion_t;

lista_t* cargar_csv_en_lista(maquina_votacion_t* maquina, const char* nombre, bool (*func)(fila_csv_t*, lista_t*, size_t) );
bool enlistar_partido(fila_csv_t* fila, lista_t* lista, size_t columnas);
bool enlistar_votante(fila_csv_t* fila, lista_t* lista, size_t columnas);
void destruir_votante(void* dato);
void destruir_partido(void* dato);

#endif
