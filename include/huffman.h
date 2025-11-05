#ifndef HUFFMAN_H
#define HUFFMAN_H

#include "cola.h"

// Estructura del nodo del arbol de Huffman
typedef struct Nodo 
{
    char c;
    int frecuencia;
    struct Nodo* izq;
    struct Nodo* der;
} Nodo;

Nodo* huffman(ColaPrioridad *cola);
Nodo* creaNodo(char c, int frecuencia);
void codigosHuffman(Nodo* raiz, int nivel, char* codigos[256], char* buffer);
    
#endif