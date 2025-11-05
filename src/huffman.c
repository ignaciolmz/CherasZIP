#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "huffman.h"
#include "cola.h"

// Construccion del arbol de Huffman a partir de la cola de prioridad, 
// mediante adicion de nodos de paso. En ultima instancia solo queda 
// un único nodo, la raiz del arbol de Huffman.
Nodo* huffman(ColaPrioridad *cola) 
{   
    Nodo *nodo1, *nodo2, *nuevo;
    
    while(cola->tam > 1) 
    {
        nodo1 = extrae(cola);
        nodo2 = extrae(cola);
    
        int frecNuevo = nodo1->frecuencia + nodo2 -> frecuencia;
        nuevo = creaNodo('\0', frecNuevo);
        nuevo->izq = nodo1;
        nuevo->der = nodo2;  
    
        inserta(cola, nuevo);
    }

    // Solo queda la raíz, con el árbol completamente construido.
    return extrae(cola); 
}

// Crea un nuevo nodo, con un caracrer y frecuencia determinados.
// Inicialmente, no define sus hijos.
Nodo* creaNodo(char c, int frec) 
{
    // Reserva dinamica para el nodo.
    Nodo *nodo = (Nodo*) malloc(sizeof(Nodo));
    if(nodo == NULL) 
    {
        fprintf(stderr, "Error al intentar realizar la asignacion de memoria\n");
        exit(1); 
    }
    // Especificación de valor inicial para el nodo.
    nodo->c = c;
    nodo->frecuencia = frec;
    nodo->izq = NULL;
    nodo->der = NULL;   

    return nodo;
}

// Recorrido del arbol de Huffman en preorden.
// Permite obtener los codigos asociados a cada caracter.
void codigosHuffman(Nodo *raiz, int nivel, char* codigos[256], char* buffer) 
{
    if(raiz != NULL) 
    {
        if(raiz->izq == NULL && raiz->der == NULL) 
        {
            buffer[nivel] = '\0'; // Es una hoja (caracter).
            codigos[(unsigned char) raiz->c] = strdup(buffer); // Codigo asociado al caracter.
        }
        // Bit 0 para el hijo izquierdo
        buffer[nivel] = '0';
        codigosHuffman(raiz->izq, nivel+1, codigos, buffer);
        // Bit 1 para el hijo derecho
        buffer[nivel] = '1';
        codigosHuffman(raiz->der, nivel+1, codigos, buffer);
    }
    else return; // Se ha llegado a todas las hojas. 
}
