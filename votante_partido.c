#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* Struct para almacenar los votantes en el padron y la cola */
typedef struct votante {
    char* documento_tipo;
    char* documento_numero;
    bool voto_realizado;
} votante_t;

/* Struct para almacenar los votos que reciba un determinado partido politico */
typedef struct partido_politico {
    size_t id;
    char* nombre;
    char** postulantes;
    size_t** votos;
    size_t largo;
} partido_politico_t;

votante_t* votante_crear(char* doc_tipo, char* doc_num) {
    votante_t* votante = malloc(sizeof(votante_t));
    if(!votante) return NULL;
    votante->documento_tipo = doc_tipo;
    votante->documento_numero = doc_num;
    votante->voto_realizado = false;
    return votante;
}

char* votante_doc_tipo(votante_t* votante) {
    return votante->documento_tipo;
}

char* votante_doc_num(votante_t* votante) {
    return votante->documento_numero;
}

bool votante_get_voto_realizado(votante_t* votante) {
    return votante->voto_realizado;
}

void votante_set_voto_realizado(votante_t* votante) {
    votante->voto_realizado = true;
}

/* Destruye la lista del padron y sus elementos */
void votante_destruir(void* dato) {
    votante_t* votante = dato;
    if(!votante) return;

    // Liberar memoria
    free(votante->documento_tipo);
    free(votante->documento_numero);
    free(votante);
}

bool votante_iguales(votante_t* a, votante_t* b) {
    return ( strcmp(a->documento_numero, b->documento_numero) == 0 && strcmp(a->documento_tipo, b->documento_tipo) == 0 );
}

/* ======================================================= */

partido_politico_t* partido_crear(size_t id, char* nombre, char** postulantes, size_t** votos, size_t largo) {

    partido_politico_t* partido = malloc(sizeof(partido_politico_t));
    if(!partido) return NULL;

    partido->id = id;
    partido->nombre = nombre;
    partido->postulantes = postulantes;
    partido->votos = votos;
    partido->largo = largo;

    #ifdef DEBUG
    printf("Partido: %s, %s, %s\n", partido->id, partido->nombre, partido->postulantes[0]);
    #endif

    return partido;
}

size_t partido_id(partido_politico_t* partido) {
    return partido->id;
}

char* partido_nombre(partido_politico_t* partido) {
    return partido->nombre;
}

char** partido_postulantes(partido_politico_t* partido) {
    return partido->postulantes;
}

size_t** partido_votos(partido_politico_t* partido) {
    return partido->votos;
}

size_t partido_largo(partido_politico_t* partido) {
    return partido->largo;
}

bool partido_iguales(partido_politico_t* partido1, partido_politico_t* partido2) {
    return partido1->id == partido2->id;
}

/* Destruye la lista de partidos y sus elementos */
void destruir_partido(void* dato) {
    partido_politico_t* partido = dato;
    if(!partido) return;

    // Liberar memoria
    free(partido->nombre);
    for(size_t i=0;i<partido->largo;i++)
    {
        free(partido->postulantes[i]);
        free(partido->votos[i]);
    }

    free(partido->postulantes);
    free(partido->votos);
    free(partido);
}
