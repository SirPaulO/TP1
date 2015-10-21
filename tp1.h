#ifndef _LISTA_H
#define _LISTA_H

#include <stdbool.h>
#include <stdlib.h>



//Imprime en pantalla el error que se produjo
void error_manager(error_code code); 


// Abre el archivo de listas y crea una lista_t por cada lista de candidatos (cada linea del archivo es una lista).
// Carga cada lista_t creada dentro de la lista de maquina->listas
// Pre: maquina_votacion_t creada. 
// Post: Devuelve false en caso de no haber modificado maquina->listas.

bool cargar_listas(maquina_votacion_t* maquina, char* listas);
/*
 Abre el archivo de padron y crea un votante_t por cada votante (cada linea del archivo es un votante).
 Carga cada votante_t creado dentro del hash de maquina->padron
 Post: Devuelve false en caso de no haber modificado maquina->padron.
*/
bool cargar_padron(maquina_votacion_t* maquina, char* padron);
/*
 Abrir archivos .csv y almacenar la informacion en TDAs
 Lista para las listas.
 Hash para el padron.
*/
bool comando_abrir(maquina_votacion_t* maquina, char* listas, char* padron);

/* Crear y encolar struct votante_t. */
bool comando_ingresar(maquina_votacion_t* maquina, char* documento_tipo, char* documento_numero);

/*
 Desencolar y realizar validacion del Documento tipo/numero del votante y no haber votado
 Crear pila para ciclo de votacion actual
*/
bool comando_votar_inicio(maquina_votacion_t* maquina);
/*
 Almacenar en la pila de votacion el partido votado.
 Validar que exista ciclo de votacion.
*/
void comando_votar_idPartido(char* id);

/* Retroceder la pila de votacion */
void comando_votar_deshacer();

/* Cerrar ciclo de votacion y procesar resultados */
void comando_votar_fin();

/* Procesar todos los votos y volcal resultados */
void comando_cerrar();

/* Formatear y mostrar menu de votacion */
void mostrar_listado(maquina_votacion_t* maquina);

/* Leer entrada e intentar formatear comandos */
void leer_entrada(maquina_votacion_t* maquina);

/*
 Crear maquina de votacion con respectivos TDAs
 Procesar comandos de entrada.
*/
int main();

#endif // _lista_H







