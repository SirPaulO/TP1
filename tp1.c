#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "lectura/lectura.h"
#include"lectura_csv/parser.h"

#include "tda/cola.h"
#include "tda/hash.h"
#include "tda/lista.h"
#include "tda/pila.h"
#include "tda/vector_dinamico.h"

#define VECTOR_TAM_INICIAL 50

typedef struct maquina_votacion{
    cola_t* cola;
    hash_t* padron;
    vector_t* listas;
    pila_t* ciclo;
} maquina_votacion_t;

// Struct para almacenar como dato en el Hash
typedef struct votante {
    char* documento_tipo;
    char* documento_numero;
    bool voto_realizado;
} votante_t;

/*
1- While de comandos de entrada por stdin
2- Cargar archivos en lista y hash
*/


/*
 Abrir archivos .csv y almacenar la informacion en TDAs
 Lista para las listas.
 Hash para el padron.
*/
bool comando_abrir(char* listas, char* padron) {
    printf("Comando abrir ejecutado: %s , %s \n", listas, padron);
    return true;
}

/*
 Crear y encolar struct votante_t.
*/
void comando_ingresar(char* documento_tipo, char* documento_numero){
    printf("Comando ingresar ejecutado: %s , %s \n", documento_tipo, documento_numero);
}

/*
 Desencolar y realizar validacion del Documento tipo/numero del votante y no haber votado
 Crear pila para ciclo de votacion actual
*/
void comando_votar_inicio(){
    printf("Comando votar inicio ejecutado \n");
}

/*
 Almacenar en la pila de votacion el partido votado.
 Validar que exista ciclo de votacion.
*/
void comando_votar_idPartido(char* id){
    printf("Comando votar idPartido ejecutado \n");
}

/*
 Retroceder la pila de votacion
*/
void comando_votar_deshacer(){
    printf("Comando votar deshacer ejecutado \n");
}


/*
 Cerrar ciclo de votacion y procesar resultados
*/
void comando_votar_fin(){
    printf("Comando votar fin ejecutado \n");
}

/*
 Procesar todos los votos y volcal resultados
*/
void comando_cerrar(){
    printf("Comando cerrar ejecutado \n");
}

/*
 Formatear y mostrar menu de votacion
*/
void mostrar_listado(int cargo);


/*
 Leer entrada e intentar formatear comandos
*/
void leer_entrada() {
	bool terminar = false;

	// Valor maximo de columnas
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

        fila_csv_t* fila = parsear_linea_csv(linea, columnas);

        cmd_ingresado = obtener_columna(fila, 0);
        cmd_param1    = obtener_columna(fila, 1);
        cmd_param2    = obtener_columna(fila, 2);

        if( strcmp(cmd_ingresado, cmd_abrir)==0 )
            comando_abrir(cmd_param1, cmd_param2);

        else if( strcmp(cmd_ingresado, cmd_ingresar)==0 )
            comando_ingresar(cmd_param1, cmd_param2);

        else if( strcmp(cmd_ingresado, cmd_votar)==0 )
        {
            if(!cmd_param1) continue;

            if( strcmp(cmd_param1,"inicio")== 0 )
                comando_votar_inicio();

            else if( strcmp(cmd_param1,"deshacer")== 0 )
                comando_votar_deshacer();

            else if( strcmp(cmd_param1,"fin")== 0 )
                comando_votar_fin();

            else // TODO Validar entrada
                comando_votar_idPartido(cmd_param1);
        }
        else if( strcmp(cmd_ingresado, cmd_cerrar)==0 )
            comando_cerrar();

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
    if(!maquina) return NULL;

    cola_t* cola = cola_crear();
    if(!cola) { free(maquina); return NULL; }

    /*hash_t* padron = hash_crear(free);
    if(!padron)
    {
        free(maquina);
        cola_destruir(cola, NULL);
        return NULL;
    }*/

    pila_t* ciclo = pila_crear();
    if(!ciclo)
    {
        free(maquina);
        cola_destruir(cola, NULL);
        //hash_destruir(padron);
        return NULL;
    }

    vector_t* listas = vector_crear(VECTOR_TAM_INICIAL);
    if(!listas)
    {
        free(maquina);
        cola_destruir(cola, NULL);
        //hash_destruir(padron);
        pila_destruir(ciclo);
        return NULL;
    }

    maquina->ciclo = ciclo;
    maquina->cola = cola;
    maquina->listas = listas;
    //maquina->padron = padron;

    leer_entrada(maquina);

    free(maquina);
    cola_destruir(cola, free);
    //hash_destruir(padron);
    pila_destruir(ciclo);

	return 0;
}
