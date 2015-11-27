#include "util.h"
#include "lista.h"
#include "lectura.h"
#include "parser.h"
#include "votante_partido.h"


typedef struct maquina_votacion maquina_votacion_t;

/************ PROTOTYPES ************/
lista_t* cargar_csv_en_lista(maquina_votacion_t* maquina, const char* nombre, bool (*func)(fila_csv_t*, lista_t*, size_t) );
bool enlistar_partido(fila_csv_t* fila, lista_t* lista, size_t columnas);
bool enlistar_votante(fila_csv_t* fila, lista_t* lista, size_t columnas);
/************************************/

lista_t* cargar_csv_en_lista(maquina_votacion_t* maquina, const char* nombre, bool (*func)(fila_csv_t*, lista_t*, size_t) ) {

    FILE* f = fopen(nombre,"r");
    if(!f) { error_manager(LECTURA); return NULL; }

    char* linea = leer_linea(f);
    if(!linea) { error_manager(OTRO); return NULL; }

    // Saltear primer linea.
    free(linea);
    linea = leer_linea(f);

    lista_t* lista = lista_crear();
    if(!lista) { error_manager(OTRO); return NULL; }

    size_t columnas = obtener_cantidad_columnas(linea, ',');

    while(linea)
    {
        fila_csv_t* fila = parsear_linea_csv(linea, columnas, false);
        free(linea); // No se usa mas, se libera.

        if(!fila)
        {
            // Leer proxima linea
            linea = leer_linea(f);
            continue;
        }

        bool insertado = func(fila, lista, columnas);

        destruir_fila_csv(fila, false); // No se usa mas, se libera.
        if(!insertado) break;

        // Leer proxima linea
        linea = leer_linea(f);
    }
    free(linea);
    fclose(f);

    return lista;
}

/*
 Abre el archivo de listas y crea un partido_t por cada lista de candidatos (cada linea del archivo es una lista).
 Carga cada partido_t creado dentro de la lista de maquina->listas
 Post: Devuelve false en caso de no haber modificado maquina->listas.
*/
bool enlistar_partido(fila_csv_t* fila, lista_t* lista, size_t columnas) {

    size_t partido_id = (size_t)strtol(obtener_columna(fila, 0), NULL, 10);
    free(obtener_columna(fila, 0));

    char** postulantes = malloc(sizeof(char*)*(columnas-2));
    //char* postulantes[columnas-2];
    size_t** votos = malloc(sizeof(size_t*)*(columnas-2));
    //size_t* votos[columnas-2];

    for(size_t i=0;i<columnas-2;i++)
    {
        size_t* cero = malloc(sizeof(size_t*));
        *cero = 0;
        votos[i] = cero;
        postulantes[i] = obtener_columna(fila, i+2);
        #ifdef DEBUG
        printf("Postulante: %s, votos: %zu\n", postulantes[i], *votos[i]);
        #endif
    }

    partido_politico_t* partido = partido_crear(partido_id, obtener_columna(fila, 1), postulantes, votos, columnas-2);
    if(!partido) return NULL; // TODO: O falso?

    bool insertar = lista_insertar_ultimo(lista, partido);
    if(!insertar)
    {
        free(partido);
        lista_destruir(lista, destruir_partido);
        return error_manager(OTRO); // TODO O simplemente False?
    }

    return true;
}

/*
 Abre el archivo de padron y crea un votante_t por cada votante (cada linea del archivo es un votante).
 Carga cada votante_t creado dentro del hash de maquina->padron
 Post: Devuelve false en caso de no haber modificado maquina->padron.
*/
bool enlistar_votante(fila_csv_t* fila, lista_t* lista, size_t columnas) {

    votante_t* votante = votante_crear(obtener_columna(fila, 0), obtener_columna(fila, 1));
    if(!votante) return error_manager(OTRO); // TODO O simplemente False?

    #ifdef DEBUG
    printf("Padron: %s, %s\n", votante->documento_tipo, votante->documento_numero);
    #endif

    bool insertar = lista_insertar_ultimo(lista, votante);
    if(!insertar)
    {
        free(votante);
        lista_destruir(lista, votante_destruir);
        return error_manager(OTRO); // TODO O simplemente False?
    }

    return true;
}
