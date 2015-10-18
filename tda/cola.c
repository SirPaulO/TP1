#include <stdlib.h>
#include "cola.h"
#include <stdbool.h>

typedef struct nodo {
	void* dato;
	struct nodo* siguiente;
} nodo_t;

struct cola {
	nodo_t* primero;
	nodo_t* ultimo;
	size_t largo;
};

// Crea una cola.
// Post: devuelve una nueva cola vac�a.

cola_t* cola_crear(void)
{
    cola_t* cola = malloc(sizeof(cola_t));

    if(!cola)
    	return NULL;

    cola->primero = NULL;
    cola->ultimo = NULL;
    cola->largo = 0;

    return cola;
}

// Destruye la cola. Si se recibe la funci�n destruir_dato por par�metro,
// para cada uno de los elementos de la cola llama a destruir_dato.
// Pre: la cola fue creada. destruir_dato es una funci�n capaz de destruir
// los datos de la cola, o NULL en caso de que no se la utilice.
// Post: se eliminaron todos los elementos de la cola.
void cola_destruir(cola_t *cola, void destruir_dato(void*))
{
	while(!cola_esta_vacia(cola))
    {
        if(destruir_dato!=NULL)
            destruir_dato(cola_desencolar(cola));
        else
            cola_desencolar(cola);
    }
	free(cola);
}

// Devuelve verdadero o falso, seg�n si la cola tiene o no elementos encolados.
// Pre: la cola fue creada.
bool cola_esta_vacia(const cola_t *cola)
{
	return (cola->primero == NULL) ? true : false;
}

// Agrega un nuevo elemento a la cola. Devuelve falso en caso de error.
// Pre: la cola fue creada.
// Post: se agreg� un nuevo elemento a la cola, valor se encuentra al final
// de la cola.
bool cola_encolar(cola_t *cola, void* valor)
{
	nodo_t* nodo = malloc(sizeof(nodo_t));

	if(nodo == NULL)
		return false;

	nodo->siguiente = NULL;
	nodo->dato = valor;

	cola->largo++;

	if(cola_esta_vacia(cola))
	{
		cola->primero = nodo;
		cola->ultimo = nodo;
		return true;
	}

	if(cola->primero == cola->ultimo)
	{
		cola->ultimo = nodo;
		cola->primero->siguiente = cola->ultimo;
		return true;
	}

	cola->ultimo->siguiente = nodo;
	cola->ultimo = nodo;
	return true;
}

// Obtiene el valor del primer elemento de la cola. Si la cola tiene
// elementos, se devuelve el valor del primero, si est� vac�a devuelve NULL.
// Pre: la cola fue creada.
// Post: se devolvi� el primer elemento de la cola, cuando no est� vac�a.
void* cola_ver_primero(const cola_t *cola)
{
	return (!cola_esta_vacia(cola)) ? (cola->primero)->dato : NULL;
}

// Saca el primer elemento de la cola. Si la cola tiene elementos, se quita el
// primero de la cola, y se devuelve su valor, si est� vac�a, devuelve NULL.
// Pre: la cola fue creada.
// Post: se devolvi� el valor del primer elemento anterior, la cola
// contiene un elemento menos, si la cola no estaba vac�a.
void* cola_desencolar(cola_t *cola)
{
	if(cola_esta_vacia(cola))
		return NULL;

	void* dato = cola_ver_primero(cola);

	if(dato != NULL)
	{
		nodo_t* nuevo_primero = cola->primero->siguiente;
		free(cola->primero);
		cola->primero = nuevo_primero;
		cola->largo--;
		return dato;
	}

	return NULL;
}
