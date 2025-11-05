#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "huffman.h"
#include "cola.h"


// Inicializa la cola
void init(ColaPrioridad *cola) 
{
    cola->nodos = NULL;
    cola->tam = 0;
}

// Inserta un nuevo nodo en la cola. Reorganiza.
void inserta(ColaPrioridad *cola, Nodo *nodo) 
{
    // Reasignación de memoria para contener al nuevo nodo
    cola->nodos = realloc(cola->nodos, (cola->tam + 1) * sizeof(Nodo*));
    if (cola->nodos == NULL) {
        fprintf(stderr, "Error al realizar la asignacion de memoria\n");
        exit(1);
    } 
    cola->tam++;  
    cola->nodos[cola->tam - 1] = nodo;  // Insercion nuevo nodo
    sift_up(cola, cola->tam - 1);    
}

// Extrae el primer nodo de la cola (nodo con menor frecuencia). Reorganiza.
Nodo* extrae(ColaPrioridad *cola) 
{
    if (cola->tam == 0) return NULL; // Cola vacia
    Nodo *nodo = cola->nodos[0]; // Guardamos el nodo a extraer
    cola->nodos[0] = cola->nodos[cola->tam - 1]; // Reemplazamos la raiz por el ultimo nodo
    cola->tam--; // Disminuimos el tamaño de la cola
    sift_down(cola, 0); // Reorganizamos la cola
    return nodo; // Liberamos memoria del nodo extraido 
}

// Reorganiza la cola de minima prioridad tras insercion. 
// Parte del final de la cola (tam-1) donde se ha insertado el nuevo nodo.
// Compara iterativamente con el padre para reubicar pertinentemente.
// Elementos organizados por frecuencia. Menores frecuencias a la cabeza. Facilitamos Huffman.
void sift_up(ColaPrioridad *cola, int pos) 
{
    int posPadre = (pos - 1) / 2; // Posicion del padre
    if ((*cola->nodos[pos]).frecuencia < (*cola->nodos[posPadre]).frecuencia) 
    {
        Nodo *temp = cola->nodos[pos];
        cola->nodos[pos] = cola->nodos[posPadre];
        cola->nodos[posPadre] = temp;
        sift_up(cola, posPadre); // Llamada recursiva para seguir subiendo el nodo
    }
    else if (pos == 0) return; // Si el nodo es la raiz, no hay padre.
} 

// Reorganiza la cola de minima prioridad tras extraccion. 
// Parte de la cabeza (0) donde se ha reubicado ultimo nodo.
// Compara iterativamente con los hijos para reubicar pertinentemente.
// Elementos organizados por frecuencia. Menores frecuencias a la cabeza. Facilitamos Huffman.
void sift_down(ColaPrioridad *cola, int pos) 
{
    int hijo_izq, hijo_der, min;
    hijo_izq = 2 * pos + 1;
    hijo_der = 2 * pos + 2;
    min = pos;
    // Comparación con hijo izquierdo
    if ((hijo_izq < cola->tam) && ((*cola->nodos[hijo_izq]).frecuencia < (*cola->nodos[min]).frecuencia)) 
    {
        min = hijo_izq;
    }
    // Comparación con hijo derecho
    if ((hijo_der < cola->tam) && ((*cola->nodos[hijo_der]).frecuencia < (*cola->nodos[min]).frecuencia)) 
    {
        min = hijo_der;
    }
    if(min != pos) {
        Nodo *temp = cola->nodos[pos];
        cola->nodos[pos] = cola->nodos[min];
        cola->nodos[min] = temp;
        sift_down(cola, min); // Llamada recursiva para seguir bajando el nodo
    } 
    else return; // Si el nodo es la hoja, no hay hijos.  
}

bool colaVacia(ColaPrioridad *cola) 
{
    return (cola->tam == 0);
}

void liberarCola(ColaPrioridad *cola) 
{
    int i;
    for(i = 0; i < cola->tam; i++) {
        free(cola->nodos[i]);
    }
    free(cola->nodos);
}