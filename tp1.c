#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "lectura/lectura.h"
#include "lectura_csv/parser.h"

#include "tda/cola.h"
#include "tda/lista.h"
#include "tda/pila.h"
#include "tda/vector_dinamico.h"

#define VECTOR_TAM_INICIAL 256
#define CARGOS_A_VOTAR 5 // Cantidad de puestos a votar + 2 (Por ID y Nombre de la lista)

#define DEBUG 0

typedef enum {
    PRESIDENTE,
    GOBERNADOR,
    INTENDENTE,
    FIN
} cargo_t;

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
    // Ciclo donde se guardan los datos mientras un votante este votando
    pila_t* ciclo;
    // Cargo que se esta votando actualmente (de estar votandose)
    cargo_t votando_cargo;
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

/* Copia la clave en memoria */
char* copiar_clave(const char *clave) {
    char* clave_copiada = malloc(sizeof(char) * strlen(clave)+1);
    strcpy(clave_copiada, clave);
    return clave_copiada;
}

/*
 Abre el archivo de listas y crea una lista_t por cada lista de candidatos (cada linea del archivo es una lista).
 Carga cada lista_t creada dentro de la lista de maquina->listas
 Post: Devuelve false en caso de no haber modificado maquina->listas.
*/
bool cargar_listas(maquina_votacion_t* maquina, const char* listas) {
    lista_t* lista_candidatos = lista_crear();
    if(!lista_candidatos)
        return error_manager(OTRO);

    FILE* f = fopen(listas,"r");
	if(!f)
    {
        lista_destruir(lista_candidatos, NULL);
        fclose(f);
        return error_manager(LECTURA);
    }

	char* linea = leer_linea(f);
	free(linea);
    // Saltear primer linea.
    linea = leer_linea(f);
    size_t cantidad_partidos = 0;

	while(linea)
	{
        cantidad_partidos++;
        partido_politico_t* partido = malloc(sizeof(partido_politico_t));
		fila_csv_t* fila = parsear_linea_csv(linea, CARGOS_A_VOTAR, false);

		free(linea); // No se usa mas, se libera.

        if(!fila || !partido)
        {
            if(partido) free(partido);
            if(fila) destruir_fila_csv(fila, true);
            //free(linea);
            fclose(f);
            lista_destruir(lista_candidatos, free);
            return error_manager(OTRO);
        }

        // EXPERIMENTAL: Evito copiar las claves, pero tampoco las elimino despues de usarlas

        partido->idPartido = obtener_columna(fila, 0);
        partido->nombre_partido = obtener_columna(fila, 1);
        partido->presidente = obtener_columna(fila, 2);
        partido->gobernador = obtener_columna(fila, 3);
        partido->intendente = obtener_columna(fila, 4);
        partido->votos_presidente = 0;
        partido->votos_gobernador = 0;
        partido->votos_intendente = 0;

        destruir_fila_csv(fila, false); // No se usa mas, se libera.

        if(DEBUG) printf("Listas: %s, %s, %s\n", partido->idPartido, partido->nombre_partido, partido->presidente);

        bool insertar = lista_insertar_ultimo(lista_candidatos, partido);
        if(!insertar)
        {
            free(partido);
            //free(linea);
            fclose(f);
            lista_destruir(lista_candidatos, free);
            return error_manager(OTRO);
        }

        // Leer proxima linea
        linea = leer_linea(f);
	}
    free(linea); // Free de NULL?
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
bool cargar_padron(maquina_votacion_t* maquina, const char* padron) {
    if(maquina->estado >= ABIERTA) return error_manager(MESA_ABIERTA);

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

        free(linea); // No se usa mas, se libera.

		if(!votante || !fila)
        {
            if(fila) free(fila);
            if(votante) free(votante);
            fclose(f);
            lista_destruir(lista_candidatos, free);
            return error_manager(OTRO);
        }

        char* documento_tipo = obtener_columna(fila, 0);
        char* documento_numero = obtener_columna(fila, 1);

        destruir_fila_csv(fila, false); // No se usa mas, se libera.

        votante->documento_numero = documento_numero;
        votante->documento_tipo = documento_tipo;
        votante->voto_realizado = false;

        if(DEBUG) printf("Padron: %s, %s\n", votante->documento_tipo, votante->documento_numero);


        bool guardado = lista_insertar_ultimo(lista_candidatos, votante);
        if(!guardado)
        {
            free(votante);
            lista_destruir(lista_candidatos, free);
            fclose(f);
            return error_manager(OTRO);
        }

        // Leer proxima linea
        linea = leer_linea(f);
	}
	free(linea); // Free de NULL?
	fclose(f);
    maquina->padron = lista_candidatos;

	return true;
}

void destruir_padron(maquina_votacion_t* maquina) {
    if(!maquina->padron) return;

    lista_iter_t* iter = lista_iter_crear(maquina->padron);
    if(!iter) return;

    while(!lista_iter_al_final(iter))
    {
        votante_t* votante = lista_iter_ver_actual(iter);
        if(!votante) continue;

        // Liberar memoria
        free(votante->documento_tipo);
        free(votante->documento_numero);
        free(votante);

        lista_iter_avanzar(iter);
    }
    lista_iter_destruir(iter);
    lista_destruir(maquina->padron, NULL);
    maquina->padron = NULL;
}

void destruir_listas(maquina_votacion_t* maquina) {
    if(!maquina->listas) return;

    lista_iter_t* iter = lista_iter_crear(maquina->listas);
    if(!iter) return;

    while(!lista_iter_al_final(iter))
    {
        partido_politico_t* partido = lista_iter_ver_actual(iter);
        if(!partido) continue;
        // Liberar memoria
        free(partido->idPartido);
        free(partido->nombre_partido);
        free(partido->presidente);
        free(partido->gobernador);
        free(partido->intendente);
        free(partido);

        lista_iter_avanzar(iter);
    }
    lista_iter_destruir(iter);
    lista_destruir(maquina->listas, NULL);
    maquina->listas = NULL;
}

void destruir_ciclo(maquina_votacion_t* maquina) {
    if(!maquina->ciclo) return;
    while(!pila_esta_vacia(maquina->ciclo))
    {
        char* id = pila_desapilar(maquina->ciclo);
        if(!id) continue;
        // Liberar memoria
        free(id);
    }
    pila_destruir(maquina->ciclo);
    maquina->ciclo = NULL;
}

void destruir_cola(maquina_votacion_t* maquina) {
    if(!maquina->cola) return;

    while(!cola_esta_vacia(maquina->cola))
    {
        votante_t* votante = cola_desencolar(maquina->cola);
        if(!votante) continue;
        // Liberar memoria
        free(votante->documento_numero);
        free(votante->documento_tipo);
        free(votante);
    }
    cola_destruir(maquina->cola, free);
    maquina->cola = NULL;
}

void cerrar_maquina(maquina_votacion_t* maquina) {
    if(DEBUG) printf("Cerrar maquinas por las pruebas de mierda.\n");

    destruir_padron(maquina);
    destruir_listas(maquina);
    destruir_cola(maquina);
    destruir_ciclo(maquina);
}

/*
 Abrir archivos .csv y almacenar la informacion en TDAs
 Lista para las listas.
 Hash para el padron.
*/
bool comando_abrir(maquina_votacion_t* maquina, const char* listas, const char* padron) {
    if(DEBUG) printf("Comando abrir ejecutado.");
    if(!listas || !padron)
        return error_manager(LECTURA);

    if(maquina->estado >= ABIERTA)
        return error_manager(MESA_ABIERTA);

    cargar_listas(maquina, listas);
    cargar_padron(maquina, padron);

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
        if(ciclo_votacion) free(ciclo_votacion);
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
            free(ciclo_votacion);
            return enpadronado ? error_manager(VOTO_REALIZADO) : error_manager(NO_ENPADRONADO);
    }

    votante_padron->voto_realizado = true;
    maquina->estado = VOTACION;
    maquina->ciclo = ciclo_votacion;
    maquina->votando_cargo = PRESIDENTE;

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
        case FIN: lista_iter_destruir(iter); return; break; // Tecnicamente nunca deberia llegar aca.
    }

    while(!lista_iter_al_final(iter))
    {
        partido_politico_t* partido = lista_iter_ver_actual(iter);
        if(!partido) { error_manager(OTRO); lista_iter_destruir(iter); return; }

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
            case FIN: break; // -Werror=switch
        }
        lista_iter_avanzar(iter);
    }
    lista_iter_destruir(iter);
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

    char* idPartido_str = copiar_clave(id);

    pila_apilar(maquina->ciclo, idPartido_str);
    maquina->votando_cargo++;

    printf("OK\n");

    if(maquina->votando_cargo < FIN)
        mostrar_menu_votacion(maquina);

    return true;
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
        char* id = pila_desapilar(maquina->ciclo);
        // TODO if(!id)

        lista_iter_t* iter = lista_iter_crear(maquina->listas);
        if(!iter) { free(id); return error_manager(OTRO); }
        maquina->votando_cargo--;

        while(!lista_iter_al_final(iter))
        {
            partido_politico_t* partido = lista_iter_ver_actual(iter);
            if(!partido) { free(id); lista_iter_destruir(iter); return error_manager(OTRO); }

            if(strcmp(partido->idPartido, id) == 0)
            {
                if(DEBUG) printf("idPartido votado: %s, Cargo %d\n", id, maquina->votando_cargo);
                switch(maquina->votando_cargo)
                {
                    case PRESIDENTE: partido->votos_presidente++; break;
                    case GOBERNADOR: partido->votos_gobernador++; break;
                    case INTENDENTE: partido->votos_intendente++; break;
                    case FIN: break; // -Werror=switch
                }
            }
            lista_iter_avanzar(iter);
        }
        lista_iter_destruir(iter);
        free(id);
    }

    // Reset de variables.
    pila_destruir(maquina->ciclo);
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
    free(pila_desapilar(maquina->ciclo));

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
        printf("%s:\nPresidente: %d votos\nGobernador: %zu votos\nIntendente: %zu votos\n\n",
            partido->nombre_partido, (int)partido->votos_presidente, partido->votos_gobernador, partido->votos_intendente);
        // Liberar memoria
        free(partido->idPartido);
        free(partido->nombre_partido);
        free(partido->presidente);
        free(partido->gobernador);
        free(partido->intendente);
        free(partido);

        lista_iter_avanzar(iter);
    }
    lista_iter_destruir(iter);
    lista_destruir(maquina->listas, NULL);
    maquina->listas = NULL;

    return true;
}

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

/* Leer entrada e intentar formatear comandos */
void leer_entrada(maquina_votacion_t* maquina) {
	bool terminar = false;

	char* cmd_abrir       = "abrir";
    char* cmd_ingresar    = "ingresar";
    char* cmd_votar       = "votar";
    char* cmd_cerrar      = "cerrar";

	while(!terminar)
	{
		char* linea = leer_linea(stdin);
		if(!linea)
        {
            terminar = true;
            continue;
        }

        // 5 = cantidad minima de caracteres del comando mas corto valido.
		if(strlen(linea) < 5)
        {
            free(linea);
            continue;
        }

        size_t columnas = obtener_cantidad_columnas(linea, ' ');

        fila_csv_t* fila = parsear_linea_csv(linea, columnas, true);
        if(!fila)
        {
            free(linea);
            continue;

        }

        char* cmd_ingresado = obtener_columna(fila, 0) ? obtener_columna(fila, 0) : "";
        char* cmd_param1 = obtener_columna(fila, 1) ? obtener_columna(fila, 1) : "";
        char* cmd_param2 = obtener_columna(fila, 2);


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
            {
                comando_votar_inicio(maquina);
            }
            else if( strcmp(cmd_param1,"deshacer")== 0 ) {
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
        else if( strcmp(cmd_ingresado, cmd_cerrar)==0 )
            if( comando_cerrar(maquina) )
                terminar = true;

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

    cerrar_maquina(maquina);
    free(maquina);

	return 0;
}
