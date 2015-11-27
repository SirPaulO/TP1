#ifndef VOTANTE_PARTIDO_H
#define VOTANTE_PARTIDO_H

#include <stdbool.h>
#include <stdlib.h>

/* Struct para almacenar los votantes en el padron y la cola */
typedef struct votante votante_t;

/* Struct para almacenar los votos que reciba un determinado partido politico */
typedef struct partido_politico partido_politico_t;

votante_t* votante_crear(char* doc_tipo, char* doc_num);

char* votante_doc_tipo(votante_t*);

char* votante_doc_num(votante_t*);

bool votante_get_voto_realizado();

void votante_set_voto_realizado(votante_t*);

bool votante_iguales(votante_t*, votante_t*);

/* ========================================== */

partido_politico_t* partido_crear(size_t, char*, char**, size_t**, size_t);

void votante_destruir(void* dato);

int partido_id(partido_politico_t*);

char* partido_nombre(partido_politico_t*);

char** partido_postulantes(partido_politico_t*);

size_t** partido_votos(partido_politico_t*);

size_t partido_largo(partido_politico_t*);

bool partido_iguales(partido_politico_t*, partido_politico_t*);

void destruir_partido(void* dato);

#endif
