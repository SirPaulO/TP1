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

#define DEBUG 0

const char* comandos[] = {"abrir","ingresar","votar","cerrar"};
const char* CARGOS[] = {"Presidente", "Gobernador", "Intendente"};

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
    char* partido_id;
    size_t cargo;
} voto_t;

/************ PROTOTYPES ************/
void leer_entrada(maquina_votacion_t* maquina);

bool comando_abrir(maquina_votacion_t* maquina, const char* listas, const char* padron);

bool comando_ingresar(maquina_votacion_t* maquina, char* documento_tipo, char* documento_numero);

bool comando_votar_inicio(maquina_votacion_t* maquina);
bool comando_votar_idPartido(maquina_votacion_t* maquina, char* id);
bool comando_votar_deshacer(maquina_votacion_t* maquina);
bool comando_votar_fin(maquina_votacion_t* maquina);

void mostrar_menu_votacion(maquina_votacion_t*);

bool comando_cerrar(maquina_votacion_t* maquina);
void cerrar_maquina(maquina_votacion_t* maquina);

/************************************/

/* Llama a las funciones de destruccion necesarias */
void cerrar_maquina(maquina_votacion_t* maquina) {
    if(maquina->padron)
        lista_destruir(maquina->padron, destruir_votante);
    maquina->padron = NULL;

    if(maquina->listas)
        lista_destruir(maquina->listas, destruir_partido);
    maquina->listas = NULL;

    if(maquina->cola)
        cola_destruir(maquina->cola, destruir_votante);
    maquina->cola = NULL;

    /* Destruye la pila del ciclo de votacion */
    if(maquina->ciclo)
        pila_destruir(maquina->ciclo, free);
    maquina->ciclo = NULL;
}

/*
 Si recibe parametros validos, llama a funciones abrir de listas y padron.
*/
bool comando_abrir(maquina_votacion_t* maquina, const char* listas, const char* padron) {
    if(DEBUG) printf("Comando abrir ejecutado.");
    if(!listas || !padron)
        return error_manager(LECTURA);

    if(maquina->estado >= ABIERTA)
        return error_manager(MESA_ABIERTA);

    maquina->listas = cargar_csv_en_lista(maquina, listas, enlistar_partido);
    if(!maquina->listas) return false;

    maquina->padron = cargar_csv_en_lista(maquina, padron, enlistar_votante);
    if(!maquina->padron) { lista_destruir(maquina->listas, destruir_votante); return false; }

    maquina->cantidad_partidos = lista_largo(maquina->listas);
	maquina->estado = ABIERTA;

    return true;
}

/* Crear y encolar struct votante_t. */
bool comando_ingresar(maquina_votacion_t* maquina, char* documento_tipo, char* documento_numero) {
    if(DEBUG) printf("Comando ingresar ejecutado: %s , %s \n", documento_tipo, documento_numero);

    // Error handling
    if(maquina->estado == CERRADA)              { return error_manager(MESA_CERRADA); }
    if(!documento_tipo || !documento_numero)    { return error_manager(NUMERO_NEGATIVO); } // Segun las pruebas
    if(strtol(documento_numero, NULL, 10) < 1)  { return error_manager(NUMERO_NEGATIVO); }

    votante_t* votante = malloc(sizeof(votante_t));

    if(!votante) {  return error_manager(OTRO); }

    votante->documento_numero = copiar_clave(documento_numero);
    votante->documento_tipo = copiar_clave(documento_tipo);

    if(DEBUG) printf("Votante ingresado: %s, %s\n", votante->documento_tipo, votante->documento_numero);

    if( cola_encolar(maquina->cola, votante) )
        return true;

    return error_manager(OTRO);
}

/*
 Desencolar y realizar validacion del Documento tipo/numero del votante y no haber votado
 Crear pila para ciclo de votacion actual
*/
bool comando_votar_inicio(maquina_votacion_t* maquina) {
    if(DEBUG) printf("Comando votar inicio ejecutado.\n");
    // Error handling
    if(maquina->estado == CERRADA)      { return error_manager(MESA_CERRADA); }
    if(maquina->estado == VOTACION)     { return error_manager(OTRO); }
    if(cola_esta_vacia(maquina->cola))  { return error_manager(NO_VOTANTES); }

    votante_t* votante_padron = NULL;
    bool enpadronado = false;

    votante_t* votante_espera = cola_desencolar(maquina->cola);
    if(!votante_espera) return error_manager(OTRO);

    if(DEBUG) printf("Votante desencolado: %s, %s\n", votante_espera->documento_tipo, votante_espera->documento_numero);

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
        if( strcmp(votante_padron->documento_numero, votante_espera->documento_numero) == 0 && strcmp(votante_padron->documento_tipo, votante_espera->documento_tipo) == 0)
        {
            enpadronado = true;
            break;
        }
        lista_iter_avanzar(lista_iter);
    }
    lista_iter_destruir(lista_iter);
    free(votante_espera->documento_numero);
    free(votante_espera->documento_tipo);
    free(votante_espera);

    if(!enpadronado || votante_padron->voto_realizado)
    {
            pila_destruir(ciclo_votacion, NULL);
            return enpadronado ? error_manager(VOTO_REALIZADO) : error_manager(NO_ENPADRONADO);
    }

    votante_padron->voto_realizado = true;
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
    printf("%d: %s: %s\n", (int)strtol(partido->id, NULL, 10), partido->nombre, partido->postulantes[*cargo]);
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
    if(DEBUG) printf("Comando votar idPartido ejecutado \n");
    if(maquina->estado < VOTACION || (maquina->votando_cargo >= FIN) )
        return error_manager(OTRO);

    long int idPartido_int = strtol(id, NULL, 10);

    if(idPartido_int < 1 || idPartido_int > maquina->cantidad_partidos)
        return error_manager(OTRO);

    voto_t* voto = malloc(sizeof(voto_t));

    voto->partido_id = copiar_clave(id);
    voto->cargo = maquina->votando_cargo;

    pila_apilar(maquina->ciclo, voto);
    maquina->votando_cargo++;

    printf("OK\n");

    if(maquina->votando_cargo < FIN)
        mostrar_menu_votacion(maquina);

    return true;
}

void destruir_voto(voto_t* voto) {
    free(voto->partido_id);
    free(voto);
}

bool votar_partido(void* dato, void* extra) {
    partido_politico_t* partido = dato;
    voto_t* voto = extra;

    if(strcmp(partido->id, voto->partido_id) != 0)
        return true;

    if(DEBUG) printf("Partido ID votado: %s, Cargo %zu\n", voto->partido_id, voto->cargo);
    size_t* votos = partido->votos[voto->cargo];
    (*votos)++;
    return false;
}

/* Cerrar ciclo de votacion y procesar resultados */
bool comando_votar_fin(maquina_votacion_t* maquina) {
    if(DEBUG) printf("Comando votar fin ejecutado \n");

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
    if(pila_esta_vacia(maquina->ciclo) && DEBUG) printf("Pila vacia\n");
    pila_destruir(maquina->ciclo, free);
    maquina->ciclo = NULL;
    maquina->estado = ABIERTA;

    return true;
}

/* Retroceder la pila de votacion */
bool comando_votar_deshacer(maquina_votacion_t* maquina) {
    if(DEBUG) printf("Comando votar deshacer ejecutado \n");

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
bool comando_cerrar(maquina_votacion_t* maquina) {
    if(DEBUG) printf("Comando cerrar ejecutado\n");
    if(maquina->estado < ABIERTA) return error_manager(OTRO);
    if(maquina->estado > ABIERTA || !cola_esta_vacia(maquina->cola) ) return error_manager(COLA_NO_VACIA);

    lista_iter_t* iter = lista_iter_crear(maquina->listas);
    if(!iter) return error_manager(OTRO);

    while(!lista_iter_al_final(iter))
    {
        partido_politico_t* partido = lista_iter_ver_actual(iter);
        if(!partido) { lista_iter_destruir(iter); return error_manager(OTRO); }

        printf("%s:\n", partido->nombre);

        for(size_t i=0;i<partido->largo;i++)
        {
            size_t* votos = partido->votos[i];
            printf("%s: %zu votos\n", CARGOS[i], *votos);
        }
        printf("\n");

        // Liberar memoria
        destruir_partido(partido);
        lista_iter_avanzar(iter);
    }
    lista_iter_destruir(iter);
    lista_destruir(maquina->listas, NULL);
    maquina->listas = NULL;

    return true;
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

        char* cmd_ingresado = obtener_columna(fila, 0);
        char* cmd_param1 = obtener_columna(fila, 1);
        char* cmd_param2 = obtener_columna(fila, 2);

        if( strcmp(cmd_ingresado, comandos[0])==0 ) {
            if( comando_abrir(maquina, cmd_param1, cmd_param2) )
                printf("OK\n");
        }
        else if( strcmp(cmd_ingresado, comandos[1])==0 ) {
            if( comando_ingresar(maquina, cmd_param1, cmd_param2) )
                printf("OK\n");
        }
        else if( strcmp(cmd_ingresado, comandos[2])==0 )
        {
            if(!cmd_param1) continue;

            if( strcmp(cmd_param1,"inicio") == 0 )
            {
                comando_votar_inicio(maquina);
            }
            else if( strcmp(cmd_param1,"deshacer") == 0 ) {
                if( comando_votar_deshacer(maquina) )
                {
                    printf("OK\n");
                    mostrar_menu_votacion(maquina);
                }
            }
            else if( strcmp(cmd_param1,"fin") == 0 ) {
                if( comando_votar_fin(maquina) )
                    printf("OK\n");
            }
            else
            {
                comando_votar_idPartido(maquina, cmd_param1);
            }
        }
        else if( strcmp(cmd_ingresado, comandos[3])==0 )
            if( comando_cerrar(maquina) )
                terminar = true;

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

    leer_entrada(maquina);

    cerrar_maquina(maquina);
    free(maquina);

	return 0;
}
