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
#define PUESTOS_A_VOTAR 5 // Cantidad de puestos a votar + 2 (Por ID y Nombre de la lista)

/* Struct para almacenar los votantes en el padron y la cola */
typedef struct votante {
    char* documento_tipo;
    char* documento_numero;
    bool voto_realizado;
} votante_t;

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
    hash_t* padron;
    // Listas habilitadas para ser votadas
    lista_t* listas;
    // Vector para almacenar los votos
    vector_t* votos;
    // Ciclo donde se guardan los datos mientras un votante este votando
    pila_t* ciclo;
    // Cargo que se esta votando actualmente (de estar votandose)
    int votando_cargo;
} maquina_votacion_t;

/* Imprime codigo de error */
void error_manager(error_code code) {
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
}

/*
 Abre el archivo de listas y crea una lista_t por cada lista de candidatos (cada linea del archivo es una lista).
 Carga cada lista_t creada dentro de la lista de maquina->listas
 Post: Devuelve false en caso de no haber modificado maquina->listas.
*/
bool cargar_listas(maquina_votacion_t* maquina, char* listas) {
    // TODO: Cerrar archivo antes de devolver False en algun error
    FILE* f = fopen(listas,"r");
	if(!f)
    {
        error_manager(LECTURA);
        return false;
    }

	char* linea = leer_linea(f);
	while(linea)
	{
	    // TODO: si fila es NULL?
		fila_csv_t* fila = parsear_linea_csv(linea, PUESTOS_A_VOTAR, false);

		lista_t* lista_candidatos = lista_crear();
		if(!lista_candidatos)
        { /* TODO: si falla? */ }

        // TODO: Si devuelve false en algun insertar?
        for(size_t i=0;i<PUESTOS_A_VOTAR;i++)
            lista_insertar_ultimo(lista_candidatos, obtener_columna(fila, i));

        // TODO: Si devuelve false al insertar?
        lista_insertar_ultimo(maquina->listas, lista_candidatos);

        destruir_fila_csv(fila, true);
        lista_candidatos = NULL;
        linea = NULL;

        // Leer proxima linea
        linea = leer_linea(f);
	}
	free(linea);
	fclose(f);

	return true;
}

/*
 Abre el archivo de padron y crea un votante_t por cada votante (cada linea del archivo es un votante).
 Carga cada votante_t creado dentro del hash de maquina->padron
 Post: Devuelve false en caso de no haber modificado maquina->padron.
*/
bool cargar_padron(maquina_votacion_t* maquina, char* padron) {
    // TODO: Cerrar archivo antes de devolver False en algun error
    FILE* f = fopen(padron,"r");
	if(!f)
    {
        error_manager(LECTURA);
        return false;
    }

	char* linea = leer_linea(f);

	// Cantidad de columnas en el padron (Tipo y Numero de documento)
	size_t cantidad_Columnas = 2;
	while(linea)
	{
	    // TODO: si fila es NULL?
		fila_csv_t* fila = parsear_linea_csv(linea, cantidad_Columnas, false);

        votante_t* votante = malloc(sizeof(votante_t));
		if(!votante)
        { /* TODO: si falla? */ }

        char* documento_tipo = obtener_columna(fila, 0);
        char* documento_numero = obtener_columna(fila, 1);

        votante->documento_numero = documento_numero;
        votante->documento_tipo = documento_tipo;
        votante->voto_realizado = false;

        // TODO: Fallo al guardar
        hash_guardar(maquina->padron,documento_numero, votante);

        destruir_fila_csv(fila, true);
        votante = NULL;
        linea = NULL;

        // Leer proxima linea
        linea = leer_linea(f);
	}
	free(linea);
	fclose(f);

	return true;
}

/*
 Abrir archivos .csv y almacenar la informacion en TDAs
 Lista para las listas.
 Hash para el padron.
*/
bool comando_abrir(maquina_votacion_t* maquina, char* listas, char* padron) {
    if(!listas || !padron)
    {
        error_manager(LECTURA);
        return false;
    }

    if(maquina->estado >= ABIERTA)
    {
        error_manager(MESA_ABIERTA);
        return false;
    }

    // TODO: Si falla al crear lista.
    maquina->listas = lista_crear();
    // TODO: Si falla al crear lista.
    maquina->padron = hash_crear(free);

    cargar_listas(maquina, listas);
    cargar_padron(maquina, padron);

	maquina->estado = ABIERTA;

    return true;
}

/* Crear y encolar struct votante_t. */
bool comando_ingresar(maquina_votacion_t* maquina, char* documento_tipo, char* documento_numero) {
    printf("Comando ingresar ejecutado: %s , %s \n", documento_tipo, documento_numero);

    // Error handling
    if(maquina->estado == CERRADA)              { error_manager(MESA_CERRADA);      return false; }
    if(!documento_tipo || !documento_numero)    { error_manager(OTRO);              return false; }
    if(strtol(documento_numero, NULL, 10) < 0)  { error_manager(NUMERO_NEGATIVO);   return false; }

    votante_t* votante = malloc(sizeof(votante_t));

    if(!votante) { error_manager(OTRO); return false; }

    votante->documento_numero = documento_numero;
    votante->documento_tipo = documento_tipo;

    if( cola_encolar(maquina->cola, votante) )
        return true;

    error_manager(OTRO);
    return false;
}

/*
 Desencolar y realizar validacion del Documento tipo/numero del votante y no haber votado
 Crear pila para ciclo de votacion actual
*/
bool comando_votar_inicio(maquina_votacion_t* maquina){
    printf("Comando votar inicio ejecutado \n");

    // Error handling
    if(maquina->estado == CERRADA)      { error_manager(MESA_CERRADA);  return false; }
    if(maquina->estado == VOTACION)     { error_manager(OTRO);          return false; }
    if(cola_esta_vacia(maquina->cola))  { error_manager(NO_VOTANTES);   return false; }

    votante_t* votante_espera = cola_desencolar(maquina->cola);
    if(!votante_espera) { error_manager(OTRO); return false; }

    bool enpadronado = hash_pertenece(maquina->padron, votante_espera->documento_numero);
    if(!enpadronado)
    {
            error_manager(NO_ENPADRONADO);
            free(votante_espera);
            return false;
    }

    votante_t* votante_padron = hash_obtener(maquina->padron, votante_espera->documento_numero);

    if(votante_padron->voto_realizado)
    {
        error_manager(VOTO_REALIZADO);
        free(votante_espera);
        return false;
    }

    pila_t* ciclo = pila_crear();
    if(!ciclo)
    {
        error_manager(OTRO);
        return false;
    }

    maquina->estado = VOTACION;
    maquina->ciclo = ciclo;
    maquina->votando_cargo = 1;

    // TODO: Mostrar menu de votacion
    return true;
}

/*
 Almacenar en la pila de votacion el partido votado.
 Validar que exista ciclo de votacion.
*/
void comando_votar_idPartido(char* id) {
    printf("Comando votar idPartido ejecutado \n");
}

/* Retroceder la pila de votacion */
void comando_votar_deshacer() {
    printf("Comando votar deshacer ejecutado \n");
}


/* Cerrar ciclo de votacion y procesar resultados */
void comando_votar_fin() {
    printf("Comando votar fin ejecutado \n");
}

/* Procesar todos los votos y volcal resultados */
void comando_cerrar() {
    printf("Comando cerrar ejecutado \n");
}

/* Formatear y mostrar menu de votacion */
void mostrar_listado(maquina_votacion_t* maquina) {
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

        if( strcmp(cmd_ingresado, cmd_abrir)==0 )
            comando_abrir(maquina, cmd_param1, cmd_param2);

        else if( strcmp(cmd_ingresado, cmd_ingresar)==0 )
            comando_ingresar(maquina, cmd_param1, cmd_param2);

        else if( strcmp(cmd_ingresado, cmd_votar)==0 )
        {
            if(!cmd_param1) continue;

            if( strcmp(cmd_param1,"inicio")== 0 )
                comando_votar_inicio(maquina);

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
    if(!maquina) return 1;

    cola_t* cola = cola_crear();
    if(!cola) { free(maquina); return NULL; }

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
