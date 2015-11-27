#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "util.h"
#include "archivos.h"

#include "lectura.h"
#include "parser.h"

#include "cola.h"
#include "pila.h"

#include "votante_partido.h"


/* Posibles estados de la maquina de votar */
enum {
    CMD_ABRIR,
    CMD_INGRESAR,
    CMD_CERRAR,
    CMD_VOTAR,
    CMD_INICIO,
    CMD_DESHACER,
    CMD_FIN
};

// TODO: Se podria hacer un unico enum que tenga COMANDO, PARAM1, PARAM2. Pero queda feo.
enum { NIL0, ENTRADA_LISTAS, ENTRADA_PADRON };
enum { NIL1, ENTRADA_DOC_TIPO, ENTRADA_DOC_NUM };

#define COMANDOS_CANTIDAD 4
#define COMANDOS_PARAMETROS_MAX 2
const char* COMANDOS[] = {"abrir","ingresar","cerrar", "votar", "inicio", "deshacer", "fin"};

// WARN: Experimental
bool (*COMANDOS_FUNCIONES[COMANDOS_CANTIDAD])(maquina_votacion_t*, char* entrada[]);

const char* CARGOS[] = {"Presidente", "Gobernador", "Intendente"};
const char* mensaje_OK = {"OK"};

typedef enum {
    PRESIDENTE,
    GOBERNADOR,
    INTENDENTE,
    FIN
} cargo_t;

/* Posibles estados de la maquina de votar */
typedef enum {
    CERRADA,
    ABIERTA,
    VOTACION
} maquina_estado;

struct maquina_votacion {
    // Estado actual de la maquina
    maquina_estado estado;
    // Cola de votantes esperando
    cola_t* cola;
    // Padron de votantes que deben votar
    lista_t* padron;
    // Listas habilitadas para ser votadas
    lista_t* listas;
    // Ciclo donde se guardan los datos mientras un votante este votando
    pila_t* ciclo;
    // Cargo que se esta votando actualmente (de estar votandose)
    cargo_t votando_cargo;
    size_t cantidad_partidos;
};

typedef struct voto {
    size_t partido_id;
    size_t cargo;
} voto_t;

/************ PROTOTYPES ************/
void leer_entrada(maquina_votacion_t* maquina);

bool comando_abrir(maquina_votacion_t* maquina, char* entrada[]);

bool comando_ingresar(maquina_votacion_t* maquina, char* entrada[]);

bool comando_votar(maquina_votacion_t* maquina, char* entrada[]);
bool comando_votar_inicio(maquina_votacion_t* maquina);
bool comando_votar_idPartido(maquina_votacion_t* maquina, char* id);
bool comando_votar_deshacer(maquina_votacion_t* maquina);
bool comando_votar_fin(maquina_votacion_t* maquina);

void mostrar_menu_votacion(maquina_votacion_t*);

bool comando_cerrar(maquina_votacion_t* maquina, char* entrada[]);
void cerrar_maquina(maquina_votacion_t* maquina);

/************************************/
void imprimir_mensaje_ok() {
    printf("%s\n", mensaje_OK);
}

/* Llama a las funciones de destruccion necesarias */
void cerrar_maquina(maquina_votacion_t* maquina) {
    if(maquina->padron)
        lista_destruir(maquina->padron, votante_destruir);
    maquina->padron = NULL;

    if(maquina->listas)
        lista_destruir(maquina->listas, destruir_partido);
    maquina->listas = NULL;

    if(maquina->cola)
        cola_destruir(maquina->cola, votante_destruir);
    maquina->cola = NULL;

    /* Destruye la pila del ciclo de votacion */
    if(maquina->ciclo)
        pila_destruir(maquina->ciclo, free);
    maquina->ciclo = NULL;
}

/*
 Si recibe parametros validos, llama a funciones abrir de listas y padron.
*/
bool comando_abrir(maquina_votacion_t* maquina, char* entrada[]) {
    #ifdef DEBUG
    printf("Comando abrir ejecutado.");
    #endif

    if(!entrada[ENTRADA_LISTAS] || !entrada[ENTRADA_PADRON])
        return error_manager(LECTURA);

    if(maquina->estado >= ABIERTA)
        return error_manager(MESA_ABIERTA);

    maquina->listas = cargar_csv_en_lista(maquina, entrada[ENTRADA_LISTAS], enlistar_partido);
    if(!maquina->listas) return false;

    maquina->padron = cargar_csv_en_lista(maquina, entrada[ENTRADA_PADRON], enlistar_votante);
    if(!maquina->padron) { lista_destruir(maquina->listas, votante_destruir); return false; }

    maquina->cantidad_partidos = lista_largo(maquina->listas);
	maquina->estado = ABIERTA;

    return true;
}

/* Crear y encolar struct votante_t. */
bool comando_ingresar(maquina_votacion_t* maquina, char* entrada[]) {
    #ifdef DEBUG
    printf("Comando ingresar ejecutado: %s , %s \n", entrada[ENTRADA_DOC_TIPO], entrada[ENTRADA_DOC_NUM]);
    #endif

    // Error handling
    if(maquina->estado == CERRADA)              { return error_manager(MESA_CERRADA); }
    if(!entrada[ENTRADA_DOC_TIPO] || !entrada[ENTRADA_DOC_NUM])    { return error_manager(NUMERO_NEGATIVO); } // Segun las pruebas
    if(strtol(entrada[ENTRADA_DOC_NUM], NULL, 10) < 1)  { return error_manager(NUMERO_NEGATIVO); }

    char* doc_tipo_copia = copiar_clave(entrada[ENTRADA_DOC_TIPO]);
    char* doc_num_copia = copiar_clave(entrada[ENTRADA_DOC_NUM]);

    votante_t* votante = votante_crear(doc_tipo_copia, doc_num_copia);

    if(!votante) {  return error_manager(OTRO); free(doc_num_copia); free(doc_tipo_copia); }

    #ifdef DEBUG
    printf("Votante ingresado: %s, %s\n", votante_ver_doc_tipo(votante), votante_ver_doc_num(votante));
    #endif

    if( cola_encolar(maquina->cola, votante) )
        return true;

    return error_manager(OTRO);
}

bool comando_votar(maquina_votacion_t* maquina, char* entrada[]) {
    #ifdef DEBUG
    printf("Comando votar ejecutado.\n");
    #endif

    if( strcmp(entrada[1], COMANDOS[CMD_INICIO]) == 0 )
    {
        comando_votar_inicio(maquina);
    }
    else if( strcmp(entrada[1], COMANDOS[CMD_DESHACER]) == 0 ) {
        if( comando_votar_deshacer(maquina) )
        {
            imprimir_mensaje_ok();
            mostrar_menu_votacion(maquina);
        }
    }
    else if( strcmp(entrada[1], COMANDOS[CMD_FIN]) == 0 ) {
        if( comando_votar_fin(maquina) )
            imprimir_mensaje_ok();
    }
    else
        comando_votar_idPartido(maquina, entrada[1]);

    return false;
}

/*
 Desencolar y realizar validacion del Documento tipo/numero del votante y no haber votado
 Crear pila para ciclo de votacion actual
*/
bool comando_votar_inicio(maquina_votacion_t* maquina) {
    #ifdef DEBUG
    printf("Comando votar inicio ejecutado.\n");
    #endif
    // Error handling
    if(maquina->estado == CERRADA)      { return error_manager(MESA_CERRADA); }
    if(maquina->estado == VOTACION)     { return error_manager(OTRO); }
    if(cola_esta_vacia(maquina->cola))  { return error_manager(NO_VOTANTES); }

    votante_t* votante_padron = NULL;
    bool enpadronado = false;

    votante_t* votante_espera = cola_desencolar(maquina->cola);
    if(!votante_espera) return error_manager(OTRO);

    #ifdef DEBUG
    printf("Votante desencolado: %s, %s\n", votante_espera->documento_tipo, votante_espera->documento_numero);
    #endif

    pila_t* ciclo_votacion = pila_crear();
    lista_iter_t* lista_iter = lista_iter_crear(maquina->padron);

    if(!ciclo_votacion || !lista_iter) {
        if(ciclo_votacion) pila_destruir(ciclo_votacion, NULL);
        if(lista_iter) free(lista_iter);
        return error_manager(OTRO);
    }

    while(!lista_iter_al_final(lista_iter))
    {
        votante_padron = lista_iter_ver_actual(lista_iter);
        if(votante_iguales(votante_padron, votante_espera))
        {
            enpadronado = true;
            break;
        }
        lista_iter_avanzar(lista_iter);
    }
    lista_iter_destruir(lista_iter);
    votante_destruir(votante_espera);

    if(!enpadronado || votante_get_voto_realizado(votante_padron))
    {
            pila_destruir(ciclo_votacion, NULL);
            return enpadronado ? error_manager(VOTO_REALIZADO) : error_manager(NO_ENPADRONADO);
    }

    votante_set_voto_realizado(votante_padron);
    maquina->estado = VOTACION;
    maquina->ciclo = ciclo_votacion;
    maquina->votando_cargo = PRESIDENTE;

    mostrar_menu_votacion(maquina);
    return true;
}

/* Imprime el nombre del partido y el postulante para el cargo especificado */
bool imprimir_cargo(void* dato, void* extra){
    partido_politico_t* partido = dato;
    int* cargo = extra;
    printf("%d: %s: %s\n", partido_id(partido), partido_nombre(partido), partido_postulantes(partido)[*cargo]);
    return true;
}

/* Formatear y mostrar menu de votacion */
void mostrar_menu_votacion(maquina_votacion_t* maquina) {
    int votando = maquina->votando_cargo;
    printf("Cargo: %s\n", CARGOS[votando]);
    lista_iterar(maquina->listas, imprimir_cargo, &votando);
}

/*
 Almacenar en la pila de votacion el partido votado.
 Validar que exista ciclo de votacion.
*/
bool comando_votar_idPartido(maquina_votacion_t* maquina, char* id) {
    #ifdef DEBUG
    printf("Comando votar idPartido ejecutado \n");
    #endif
    if(maquina->estado < VOTACION || (maquina->votando_cargo >= FIN) )
        return error_manager(OTRO);

    long int idPartido_int = strtol(id, NULL, 10);

    if(idPartido_int < 1 || idPartido_int > maquina->cantidad_partidos)
        return error_manager(OTRO);

    voto_t* voto = malloc(sizeof(voto_t));

    voto->partido_id = (size_t)strtol(id, NULL, 10);
    voto->cargo = maquina->votando_cargo;

    pila_apilar(maquina->ciclo, voto);
    maquina->votando_cargo++;

    imprimir_mensaje_ok();

    if(maquina->votando_cargo < FIN)
        mostrar_menu_votacion(maquina);

    return true;
}

void destruir_voto(voto_t* voto) {
    free(voto);
}

bool votar_partido(void* dato, void* extra) {
    partido_politico_t* partido = dato;
    voto_t* voto = extra;

    if(partido_id(partido) != voto->partido_id)
        return true;

    #ifdef DEBUG
    printf("Partido ID votado: %s, Cargo %zu\n", voto->partido_id, voto->cargo);
    #endif
    size_t* votos = partido_votos(partido)[voto->cargo];
    (*votos)++;
    return false;
}

/* Cerrar ciclo de votacion y procesar resultados */
bool comando_votar_fin(maquina_votacion_t* maquina) {
    #ifdef DEBUG
    printf("Comando votar fin ejecutado \n");
    #endif

    if(maquina->estado < ABIERTA)
        return error_manager(MESA_CERRADA);

    if(maquina->estado < VOTACION)
        return error_manager(OTRO);

    if(maquina->votando_cargo < FIN)
        return error_manager(FALTA_VOTAR);

    while(!pila_esta_vacia(maquina->ciclo))
    {
        voto_t* voto = pila_desapilar(maquina->ciclo);
        lista_iterar(maquina->listas, votar_partido, voto);
        destruir_voto(voto);
    }

    // Reset de variables.
    #ifdef DEBUG
    if(pila_esta_vacia(maquina->ciclo)) printf("Pila vacia\n");
    #endif
    pila_destruir(maquina->ciclo, free);
    maquina->ciclo = NULL;
    maquina->estado = ABIERTA;

    return true;
}

/* Retroceder la pila de votacion */
bool comando_votar_deshacer(maquina_votacion_t* maquina) {
    #ifdef DEBUG
    printf("Comando votar deshacer ejecutado \n");
    #endif

    if(maquina->estado < ABIERTA)
        return error_manager(MESA_CERRADA);

    if(maquina->estado < VOTACION)
        return error_manager(OTRO);

    if(maquina->votando_cargo == PRESIDENTE)
        return error_manager(NO_DESHACER);

    maquina->votando_cargo--;
    destruir_voto(pila_desapilar(maquina->ciclo));

    return true;
}

/* Procesar todos los votos y volcar resultados */
bool comando_cerrar(maquina_votacion_t* maquina, char* entrada[]) {
    #ifdef DEBUG
    printf("Comando cerrar ejecutado\n");
    #endif
    if(maquina->estado < ABIERTA) return error_manager(OTRO);
    if(maquina->estado > ABIERTA || !cola_esta_vacia(maquina->cola) ) return error_manager(COLA_NO_VACIA);

    lista_iter_t* iter = lista_iter_crear(maquina->listas);
    if(!iter) return error_manager(OTRO);

    while(!lista_iter_al_final(iter))
    {
        partido_politico_t* partido = lista_iter_ver_actual(iter);
        if(!partido) { lista_iter_destruir(iter); return error_manager(OTRO); }

        printf("%s:\n", partido_nombre(partido));

        for(size_t i=0;i<partido_largo(partido);i++)
        {
            size_t* votos = partido_votos(partido)[i];
            printf("%s: %zu votos\n", CARGOS[i], *votos);
        }

        // Liberar memoria
        destruir_partido(partido);
        lista_iter_avanzar(iter);
    }
    lista_iter_destruir(iter);
    lista_destruir(maquina->listas, NULL);
    maquina->listas = NULL;

    return false;
}

/* Leer entrada e intentar formatear comandos */
void leer_entrada(maquina_votacion_t* maquina) {
	bool terminar = false;

	while(!terminar)
	{
		char* linea = leer_linea(stdin);
		if(!linea)
        {
            terminar = true;
            continue;
        }

        // 5 = cantidad minima de caracteres del comando mas corto valido.
		// if(strlen(linea) < 5) { free(linea); continue; }

        size_t columnas = obtener_cantidad_columnas(linea, ' ');

        fila_csv_t* fila = parsear_linea_csv(linea, columnas, true);
        if(!fila)
        {
            free(linea);
            continue;
        }

        // CHETISIMO.

        char* entrada[COMANDOS_PARAMETROS_MAX+1];

        for(size_t i=0; i<COMANDOS_PARAMETROS_MAX+1;i++)
            entrada[i] = obtener_columna(fila, i);

        for(size_t i=0;i<COMANDOS_CANTIDAD;i++)
            if( strcmp(entrada[0], COMANDOS[i]) == 0 )
                if( (*COMANDOS_FUNCIONES[i])(maquina, entrada) )
                    imprimir_mensaje_ok();

        destruir_fila_csv(fila, true);
        free(linea);
	}
}

/*
 Crear maquina de votacion con respectivos TDAs
 Procesar comandos de entrada.
*/
int main() {
    maquina_votacion_t* maquina = malloc(sizeof(maquina_votacion_t));
    if(!maquina) return 1;

    cola_t* cola = cola_crear();
    if(!cola) { free(maquina); return 2; }

    maquina->estado = CERRADA;
    maquina->cola = cola;
    maquina->ciclo = NULL;
    maquina->listas = NULL;
    maquina->padron = NULL;
    maquina->votando_cargo = 0;

    COMANDOS_FUNCIONES[CMD_ABRIR] = comando_abrir;
    COMANDOS_FUNCIONES[CMD_INGRESAR] = comando_ingresar;
    COMANDOS_FUNCIONES[CMD_CERRAR] = comando_cerrar;
    COMANDOS_FUNCIONES[CMD_VOTAR] = comando_votar;

    leer_entrada(maquina);

    cerrar_maquina(maquina);
    free(maquina);

	return 0;
}






