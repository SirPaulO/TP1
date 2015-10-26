#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "lectura/lectura.h"
#include "lectura_csv/parser.h"

#include "tda/cola.h"
#include "tda/hash.h"
#include "tda/lista.h"
#include "tda/pila.h"
#include "tda/vector_dinamico.h"

#define VECTOR_TAM_INICIAL 50
#define CARGOS_A_VOTAR 5 // Cantidad de puestos a votar + 2 (Por ID y Nombre de la lista)

typedef enum {
    PRESIDENTE,
    GOBERNADOR,
    INTENDENTE
} cargo;

/* Struct para almacenar los votantes en el padron y la cola */
typedef struct votante {
    char* documento_tipo;
    char* documento_numero;
    bool voto_realizado;
} votante_t;

/* Struct para almacenar los votos que reciba un determinado partido politico */
typedef struct partido_politico {
    char* idPartido;
    char* nombre_partido;
    char* presidente;
    char* gobernador;
    char* intendente;
    size_t votos_presidente;
    size_t votos_gobernador;
    size_t votos_intendente;
} partido_politico_t;

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

/* Posibles estados de la maquina de votar */
typedef enum {
    CERRADA,
    ABIERTA,
    VOTACION
} maquina_estado;

typedef struct maquina_votacion{
    // Estado actual de la maquina
    maquina_estado estado;
    // Cola de votantes esperando
    cola_t* cola;
    // Padron de votantes que deben votar
    lista_t* padron;
    // Listas habilitadas para ser votadas
    lista_t* listas;
    // Vector para almacenar los votos
    vector_t* votos;
    // Ciclo donde se guardan los datos mientras un votante este votando
    pila_t* ciclo;
    // Cargo que se esta votando actualmente (de estar votandose)
    int votando_cargo;
    size_t cantidad_partidos;
} maquina_votacion_t;

/****** PROTOS ******/
void mostrar_menu_votacion(maquina_votacion_t*);

/* Imprime codigo de error */
bool error_manager(error_code code) {
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

/*
 Abre el archivo de listas y crea una lista_t por cada lista de candidatos (cada linea del archivo es una lista).
 Carga cada lista_t creada dentro de la lista de maquina->listas
 Post: Devuelve false en caso de no haber modificado maquina->listas.
*/
bool cargar_listas(maquina_votacion_t* maquina, char* listas) {
    lista_t* lista_candidatos = lista_crear();
    if(!lista_candidatos)
        return error_manager(OTRO);

    FILE* f = fopen(listas,"r");
	if(!f)
    {

        fclose(f);
        return error_manager(LECTURA);
    }

	char* linea = leer_linea(f);
    // Saltear primer linea.
    linea = leer_linea(f);
    size_t cantidad_partidos = 0;

	while(linea)
	{
        cantidad_partidos++;
        partido_politico_t* partido = malloc(sizeof(partido_politico_t));
		fila_csv_t* fila = parsear_linea_csv(linea, CARGOS_A_VOTAR, false);

        if(!fila || !partido)
        {
            if(!partido) free(fila);
            else free(partido);

            lista_destruir(lista_candidatos, free);
            free(linea);
            return error_manager(OTRO);
        }

        partido->idPartido = obtener_columna(fila, 0);
        partido->nombre_partido = obtener_columna(fila, 1);
        partido->presidente = obtener_columna(fila, 2);
        partido->gobernador = obtener_columna(fila, 3);
        partido->intendente = obtener_columna(fila, 4);
        partido->votos_presidente = 0;
        partido->votos_gobernador = 0;
        partido->votos_intendente = 0;

        bool insertar = lista_insertar_ultimo(lista_candidatos, partido);
        if(!insertar)
        {
            fclose(f);
            free(partido);
            lista_destruir(lista_candidatos, free);
            return error_manager(OTRO);
        }

        destruir_fila_csv(fila, true);
        free(linea);

        // Leer proxima linea
        linea = leer_linea(f);
	}
    maquina->listas = lista_candidatos;
    maquina->cantidad_partidos = cantidad_partidos;
	fclose(f);

	return true;
}

/*
 Abre el archivo de padron y crea un votante_t por cada votante (cada linea del archivo es un votante).
 Carga cada votante_t creado dentro del hash de maquina->padron
 Post: Devuelve false en caso de no haber modificado maquina->padron.
*/
bool cargar_padron(maquina_votacion_t* maquina, char* padron) {
    if(maquina->estado >= ABIERTA) return error_manager(ERROR2);

    lista_t* lista_candidatos = lista_crear();
    if(!lista_candidatos) return error_manager(OTRO);

    FILE* f = fopen(padron,"r");
	if(!f)
    {
        lista_destruir(lista_candidatos, NULL);
        return error_manager(LECTURA);
    } 

	char* linea = leer_linea(f);

	// Cantidad de columnas en el padron (Tipo y Numero de documento)
	size_t cantidad_Columnas = 2;
	while(linea)
	{
		fila_csv_t* fila = parsear_linea_csv(linea, cantidad_Columnas, false);
        votante_t* votante = malloc(sizeof(votante_t));

		if(!votante || !fila)
        {
            lista_destruir(lista_candidatos, NULL);
            fclose(f);
            free(linea);
            
            if(fila != NULL) free(fila);
            if(votante != NULL) free(votante);

            return error_manager(OTRO);
        }

        char* documento_tipo = obtener_columna(fila, 0);
        char* documento_numero = obtener_columna(fila, 1);

        votante->documento_numero = documento_numero;
        votante->documento_tipo = documento_tipo;
        votante->voto_realizado = false;

        bool guardado = lista_insertar_ultimo(lista_candidatos, votante);
        if(!guardado)
        {
            lista_destruir(lista_candidatos, free);
            fclose(f);
            free(linea);
            free(fila);
            free(votante);
            return error_manager(OTRO);
        }

        destruir_fila_csv(fila, true);
        votante = NULL;
        free(linea);
        free(fila);

        // Leer proxima linea
        linea = leer_linea(f);
	}
	fclose(f);
    free(linea);
    maquina->padron = lista_candidatos;

	return true;
}

/*
 Abrir archivos .csv y almacenar la informacion en TDAs
 Lista para las listas.
 Hash para el padron.
*/
bool comando_abrir(maquina_votacion_t* maquina, char* listas, char* padron) {
    if(!listas || !padron)
        return error_manager(LECTURA);

    if(maquina->estado >= ABIERTA)
        return error_manager(MESA_ABIERTA);

    maquina->listas = lista_crear();
    if(!maquina->listas)
        return error_manager(OTRO);

    maquina->padron = hash_crear(free);
    if(!maquina->padron)
    {
        lista_destruir(maquina->listas, free);
        return error_manager(OTRO);
    }

    cargar_listas(maquina, listas);
    cargar_padron(maquina, padron);

	maquina->estado = ABIERTA;

    return true;
}

/* Crear y encolar struct votante_t. */
bool comando_ingresar(maquina_votacion_t* maquina, char* documento_tipo, char* documento_numero) {
    printf("Comando ingresar ejecutado: %s , %s \n", documento_tipo, documento_numero);

    // Error handling
    if(maquina->estado == CERRADA)              { return error_manager(MESA_CERRADA); }
    if(!documento_tipo || !documento_numero)    { return error_manager(OTRO); }
    if(strtol(documento_numero, NULL, 10) < 0)  { return error_manager(NUMERO_NEGATIVO); }

    votante_t* votante = malloc(sizeof(votante_t));

    if(!votante) {  return error_manager(OTRO); }

    votante->documento_numero = documento_numero;
    votante->documento_tipo = documento_tipo;

    if( cola_encolar(maquina->cola, votante) )
        return true;

    return error_manager(OTRO);
}

/*
 Desencolar y realizar validacion del Documento tipo/numero del votante y no haber votado
 Crear pila para ciclo de votacion actual
*/
bool comando_votar_inicio(maquina_votacion_t* maquina){
    // Error handling
    if(maquina->estado == CERRADA)      { return error_manager(MESA_CERRADA); }
    if(maquina->estado == VOTACION)     { return error_manager(OTRO); }
    if(cola_esta_vacia(maquina->cola))  { return error_manager(NO_VOTANTES); }

    votante_t* votante_espera = cola_desencolar(maquina->cola);
    if(!votante_espera) { return error_manager(OTRO); }

    bool enpadronado = hash_pertenece(maquina->padron, votante_espera->documento_numero);
    if(!enpadronado)
    {
            free(votante_espera);
            return error_manager(NO_ENPADRONADO);
    }

    votante_t* votante_padron = hash_obtener(maquina->padron, votante_espera->documento_numero);

    if(votante_padron->voto_realizado)
    {
        free(votante_espera);
        free(votante_padron);
        return error_manager(VOTO_REALIZADO);
    }

    pila_t* ciclo = pila_crear();
    if(!ciclo)
        return error_manager(OTRO);

    maquina->estado = VOTACION;
    maquina->ciclo = ciclo;
    maquina->votando_cargo = 0;

    mostrar_menu_votacion(maquina);
    return true;
}

/* Formatear y mostrar menu de votacion */
void mostrar_menu_votacion(maquina_votacion_t* maquina) {
    /*
    Cargo: Presidente
    1: Frente para la Derrota: Alan Información
    */

    lista_iter_t* iter = lista_iter_crear(maquina->listas);
    if(!iter) { error_manager(OTRO); return; }

    switch(maquina->votando_cargo)
    {
        case PRESIDENTE: printf("Cargo: Presidente\n"); break;
        case GOBERNADOR: printf("Cargo: Gobernador\n"); break;
        case INTENDENTE: printf("Cargo: Intendente\n"); break;
    }

    while(!lista_iter_al_final(iter))
    {
        partido_politico_t* partido = lista_iter_ver_actual(iter);
        if(!partido) { error_manager(OTRO); free(iter); return; }

        switch(maquina->votando_cargo)
        {
            case PRESIDENTE:
                printf("%d: %s: %s\n", (int)strtol(partido->idPartido, NULL, 10), partido->nombre_partido, partido->presidente);
                break;
            case GOBERNADOR:
                printf("%d: %s: %s\n", (int)strtol(partido->idPartido, NULL, 10), partido->nombre_partido, partido->gobernador);
                break;
            case INTENDENTE:
                printf("%d: %s: %s\n", (int)strtol(partido->idPartido, NULL, 10), partido->nombre_partido, partido->intendente);
                break;
        }
    }
}

/*
 Almacenar en la pila de votacion el partido votado.
 Validar que exista ciclo de votacion.
*/
bool comando_votar_idPartido(maquina_votacion_t* maquina, char* id) {
    printf("Comando votar idPartido ejecutado \n");
    if(maquina->estado < VOTACION || (maquina->votando_cargo > INTENDENTE) )
        return error_manager(OTRO);

    long int idPartido = strtol(id, NULL, 10);

    if(idPartido < 1 || idPartido > maquina->cantidad_partidos)
    { error_manager(OTRO); return false; }

    pila_apilar(maquina->ciclo, id);
    maquina->votando_cargo++;

    if(maquina->votando_cargo < INTENDENTE)
        mostrar_menu_votacion(maquina);

    return true;
}

/* Cerrar ciclo de votacion y procesar resultados */
bool comando_votar_fin(maquina_votacion_t* maquina) {
    printf("Comando votar fin ejecutado \n");

    if(maquina->estado < VOTACION)
        return error_manager(OTRO);

    if(maquina->votando_cargo < INTENDENTE)
        return error_manager(FALTA_VOTAR);

    while(!pila_esta_vacia(maquina->ciclo))
    {
        char* id = pila_desapilar(maquina->ciclo);
        // TODO if(!id)

        lista_iter_t* iter = lista_iter_crear(maquina->listas);
        if(!iter) { error_manager(OTRO); return false; }

        while(!lista_iter_al_final(iter))
        {
            partido_politico_t* partido = lista_iter_ver_actual(iter);
            if(!partido) { error_manager(OTRO); free(iter); return false; }

            if(strcmp(partido->idPartido, id) == 0)
            {
                switch(maquina->votando_cargo)
                {
                    case PRESIDENTE: partido->presidente++; break;
                    case GOBERNADOR: partido->gobernador++; break;
                    case INTENDENTE: partido->intendente++; break;
                }
            }
        }
        maquina->votando_cargo--;
    }

    maquina->votando_cargo = 0;
    return true;
}

/* Retroceder la pila de votacion */
bool comando_votar_deshacer(maquina_votacion_t* maquina) {
    printf("Comando votar deshacer ejecutado \n");
    if(maquina->estado < VOTACION)
        return error_manager(OTRO);

    if(maquina->votando_cargo == PRESIDENTE)
        return error_manager(NO_DESHACER);

    maquina->votando_cargo--;
    pila_desapilar(maquina->ciclo);

    if(maquina->votando_cargo < INTENDENTE)
        mostrar_menu_votacion(maquina);

    return true;
}

/* Procesar todos los votos y volcal resultados */
void comando_cerrar(maquina_votacion_t* maquina) {
    printf("Comando cerrar ejecutado \n");
}

/* Leer entrada e intentar formatear comandos */
void leer_entrada(maquina_votacion_t* maquina) {
	bool terminar = false;

	// Valor maximo de parametros para cualquier comando
	size_t columnas = 3;

	char* cmd_abrir       = "abrir";
    char* cmd_ingresar    = "ingresar";
    char* cmd_votar       = "votar";
    char* cmd_cerrar      = "cerrar";

	while(!terminar)
	{
	    char* cmd_ingresado = NULL;
	    char* cmd_param1 = NULL;
	    char* cmd_param2 = NULL;

		char* linea = leer_linea(stdin);
		if(!linea || strlen(linea) == 0)
        {
            free(linea);
            continue;
        }

        fila_csv_t* fila = parsear_linea_csv(linea, columnas, true);

        cmd_ingresado = obtener_columna(fila, 0);
        cmd_param1    = obtener_columna(fila, 1);
        cmd_param2    = obtener_columna(fila, 2);

        if( strcmp(cmd_ingresado, cmd_abrir)==0 ) {
            if( comando_abrir(maquina, cmd_param1, cmd_param2) )
                printf("OK\n");
        }
        else if( strcmp(cmd_ingresado, cmd_ingresar)==0 ) {
            if( comando_ingresar(maquina, cmd_param1, cmd_param2) )
                printf("OK\n");
        }
        else if( strcmp(cmd_ingresado, cmd_votar)==0 )
        {
            if(!cmd_param1) continue;

            if( strcmp(cmd_param1,"inicio")== 0 )
                comando_votar_inicio(maquina);

            else if( strcmp(cmd_param1,"deshacer")== 0 ) {
                if( comando_votar_deshacer(maquina) )
                    printf("OK\n");
            }
            else if( strcmp(cmd_param1,"fin") == 0 ) {
                if( comando_votar_fin(maquina) )
                    printf("OK\n");
            }
            else
                if( comando_votar_idPartido(maquina, cmd_param1) )
                    printf("OK\n");
        }
        else if( strcmp(cmd_ingresado, cmd_cerrar)==0 )
            comando_cerrar(maquina);

        destruir_fila_csv(fila, true);
        free(linea);
        //fflush(stdin);
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

    free(maquina);
    cola_destruir(cola, free);

	return 0;
}
