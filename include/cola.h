#ifndef COLA_H
#define COLA_H

#include <stdbool.h>

struct Nodo;

typedef struct ColaPrioridad 
{
    struct Nodo** nodos; 
    int tam;
} ColaPrioridad;

void init(ColaPrioridad *cola);
struct Nodo* extrae(ColaPrioridad *cola);
void inserta(ColaPrioridad *cola, struct Nodo *nodo);
void sift_up(ColaPrioridad *cola, int pos);
void sift_down(ColaPrioridad *cola, int pos);
bool colaVacia(ColaPrioridad *cola);
void liberarCola(ColaPrioridad *cola);

#endif