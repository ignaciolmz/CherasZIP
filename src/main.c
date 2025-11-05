#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "huffman.h"
#include "cola.h"

// Prototipos locales
void procesaArgs(int argc, char** argv);
const char* getExtension(const char* filename);
const char* deleteExtension(char* filename);
void encode(FILE *source, FILE *dest, unsigned long *frecuencias, ColaPrioridad *cola, uint64_t *tamOriginal, uint8_t *bitsRelleno);
void decode(FILE *source, FILE *dest, unsigned long *frecuencias, ColaPrioridad *cola, uint64_t *tamOriginal, uint8_t *bitsRelleno);

void procesaArgs(int argc, char** argv) 
{
    if(argc != 3) 
    {
        fprintf(stderr, "Error: numero de argumentos incorrecto.\n");
        fprintf(stderr, "Uso: %s, <file> <encode/decode>\n", argv[0]);
        exit(1);
    }
    char* filename = argv[1];
    if (strcmp(argv[2], "decode") == 0) 
    {
        const char* ext = getExtension(filename);
        if(strcmp(ext, "cheras") != 0)
        {
            fprintf(stderr, "Error: no es posible decodificar el archivo expecificado.\n");
            fprintf(stderr, "CherasZIP esta diseñado para manejar archivos .cheras\n");
            fprintf(stderr, "Si no ha comprimido el archivo con CherasZIP, pruebe con el software original.\n");
            exit(1);
        } 
    } else if (strcmp(argv[2], "encode") != 0) {
        fprintf(stderr, "Error: modo de uso incorrecto.\n");
        fprintf(stderr, "Uso: %s, <file> <encode/decode>\n", argv[0]);
        exit(1);
    } 
}

const char* getExtension(const char* filename) 
{
    const char* ext = strrchr(filename, '.');
    return (ext != NULL) ? ext + 1 : "";
}

const char* deleteExtension(char* filename) 
{
    char* newFilename = (char*) malloc(strlen(filename) + 1);
    if (newFilename == NULL) 
    {
        fprintf(stderr, "Error: no se pudo asignar memoria para el nuevo nombre de archivo.\n");
        exit(1);
    }
    strcpy(newFilename, filename);
    char* ext = strrchr(newFilename, '.');
    if (ext != NULL) 
    {
        *ext = '\0'; // Elimina la extensión
    }
    return newFilename;
}
 
void encode(FILE *source, FILE *dest, unsigned long *frecuencias, ColaPrioridad *cola, uint64_t *tamOriginal, uint8_t *bitsRelleno) 
{
    /* Buffer para almacenar los códigos Huffman hasta completar un byte.
     * Cuando completa, se escribe en dest.
     * En el espacio que ocupa un único char, podran almacenarse varias
     * codificaciones de caracteres, dependiendo de la longitud de su codigo Huffman. */
    unsigned char bufferByte = 0;
    int contadorBits = 0; // Contador de bits escritos en el buffer.
    char* codigos[256] = {NULL}; // Vector de cadenas para almacenar codigo Huffman de cada char.
    char buffer[256]; // Buffer para almacenar temporalmente el codigo de Huffman, segun se va generando.
    int i, byte;

    // Construcción del histograma de frecuencias.
    while((byte = fgetc(source)) != EOF) 
    {
        frecuencias[(unsigned char) byte]++;
        (*tamOriginal)++;
    }  
    // Reinicio del puntero del archivo fuente para lectura posterior.
    rewind(source); 

    // Creacion de la cola de prioridad a partir de las frecuencias obtenidas.
    for(i = 0; i < 256; i++) 
    {
        if(frecuencias[i] > 0) 
        {
            Nodo* nodo = creaNodo((char) i, frecuencias[i]);
            inserta(cola, nodo);
        }
    }

    // Construcción del árbol de Huffman.
    Nodo* raiz = huffman(cola);
    // Generación de los códigos de Huffman.
    codigosHuffman(raiz, 0, codigos, buffer);

    // Escritura de la cabecera del archivo comprimido.
    // Tamaño del archivo original.
    fwrite(tamOriginal, sizeof(uint64_t), 1, dest);
    // Punto de control de bits para guardar los bits de relleno posteirormente.
    long pos = ftell(dest);
    uint8_t relleno = 0;
    fwrite(&relleno, sizeof(uint8_t), 1, dest);
    // Se escribe la tabla de frecuencias.
    size_t escritos = fwrite(frecuencias, sizeof(uint32_t), 256, dest);
    if(escritos != 256) 
    {
        fprintf(stderr, "Error: al escribir la tabla de frecuencias en el archivo comprimirdo.\n");
        exit(4);
    }

    // Reiteración sobre los bytes originales.
    // Sustitución por la representación Huffman correspondiente.
    while((byte = fgetc(source)) != EOF) 
    {
        // Se obtiene el código Huffman del byte leído.
        char* codHuffman = codigos[(unsigned char) byte];
        if (!codHuffman) 
        {
            fprintf(stderr, "Error: No hay código Huffman para el byte %d\n", byte);
            exit(5);
        }

        // Se escribe el código en el buffer.
        for(i = 0; codHuffman[i] != '\0'; i++) 
        {
            // Se añade un bit al buffer (por defecto, un 0).
            // Se desplaza el buffer a la izquierda, para añadir un nuevo bit.
            bufferByte <<= 1;
            if(codHuffman[i] == '1') 
            {
                // Se añade un 1 al buffer si corresponde (OR logico).
                bufferByte |= 1;
            }
            contadorBits++;
            // Completado un byte, se escribe en el archivo destino.
            if(contadorBits == 8) 
            {
                fputc(bufferByte, dest);
                bufferByte = 0;
                contadorBits = 0;
            }
        }    
    }    
    // Si el buffer no se ha completado, completa el byte restante.
    if(contadorBits > 0) 
    {
        bufferByte <<= (8 - contadorBits); 
        fputc(bufferByte, dest);

        // Guardado del número de bits de relleno.
        // Recupera el checkpoint previo de la cabecera.
        *bitsRelleno = 8 - contadorBits;
        fseek(dest, pos, SEEK_SET); 
        fwrite(&bitsRelleno, sizeof(uint8_t), 1, dest);
    }

    // Liberacion de memoria del vector que alberga los codigos de Huffman
    for (int i = 0; i < 256; ++i) {
        free(codigos[i]);
    }

    // Asegura escritura en destino de forma inmediata.
    fflush(dest); 
    fclose(dest); 
    fclose(source); 
}

void decode(FILE *source, FILE *dest, unsigned long *frecuencias, ColaPrioridad *cola, uint64_t *tamOriginal, uint8_t *bitsRelleno) 
{
    int i, byte, bit, bitsLeidos;
    uint64_t bitsTotales = 0;

    size_t leidos = fread(tamOriginal, sizeof(uint64_t), 1, source);
    if (leidos != 1) {
        perror("Error: al intentar leer tamaño del archivo original de los metadatos.\n");
        exit(6);
    }
    bitsTotales = (*tamOriginal) * 8;
    leidos = fread(bitsRelleno, sizeof(uint8_t), 1, source);
    if (leidos != 1) {
        perror("Error: al intentar leer tamaño del archivo original de los metadatos.\n");
        exit(6);
    }
    // Lectura de la tabla de frecuencias de los metadatos
    leidos = fread(frecuencias, sizeof(uint32_t), 256, source);
    if (leidos != 256) {
        fprintf(stderr, "Error: al leer la tabla de frecuencias del archivo comprimido.\n");
        exit(7);
    }

    // Recreación de la cola de prioridad a partir de las frecuencias leídas.
    for(i = 0; i < 256; i++) 
    {
        if(frecuencias[i] > 0) 
        {
            Nodo* nodo = creaNodo((char) i, frecuencias[i]);
            inserta(cola, nodo);
        }
    }

    // Reconstrucción del árbol de Huffman.
    // Parte de la raiz
    Nodo* raiz = huffman(cola);
    Nodo* actual = raiz;
    bitsLeidos = 0; // Contador auxiliar 
    // Lectura de los datos comprimidos
    while((byte = fgetc(source)) != EOF) 
    {   
        // Lectura bit a bit
        for(i = 7; i >= 0; i--) 
        {  
            // Aislamiento del bit i-eisimo 
            bit = (byte >> i) & 1;
            actual = (bit == 0) ? actual->izq : actual->der;

            if(actual ->izq == NULL && actual->der == NULL) 
            {
                // Llegados a una hoja -> Copiamos el caracter (original)
                fputc(actual->c, dest);
                // Vuelta a empezar
                actual = raiz;
                
                // Acabamos
                // Pueden haber bits de relleno al final como se especifico antes
                // No hay problema, cuando se detecte se rompe el bucle
                if(bitsLeidos >= (bitsTotales - *bitsRelleno))
                {
                    break;
                }
            }
        }
    }

    fflush(dest);
    fclose(dest);
    fclose(source);
}

int main(int argc, char** argv) 
{
    FILE *source, *dest;
    char *filename, *uso;
    char destname[256]; 
    unsigned long frecuencias[256] = {0};     
    uint64_t tamOriginal = 0;
    uint8_t bitsRelleno = 0;
    
    // Procesa los argumentos de la línea de comandos.
    procesaArgs(argc, argv);
    
    // Asigna los argumentos a las variables correspondientes.
    filename = argv[1];
    uso = argv[2];

    // Inicializa la cola de prioridad.
    ColaPrioridad *cola;
    cola = (ColaPrioridad*) malloc(sizeof(ColaPrioridad));
    if(cola == NULL) 
    {
        fprintf(stderr, "Error: al intentar asignar memoria para la cola de prioridad.\n");
        exit(2);
    }
    init(cola);    
    
    if(strcmp(uso, "encode") == 0) 
    {
        // Archivo origen (original)
        source = fopen(filename, "rb");
        if(source == NULL)
        {
            fprintf(stderr, "Error: al abrir el archivo %s para lectura.\n", filename);
            exit(3);
        }

        // Archivo destino (comprimido)
        sprintf(destname, "%s.cheras", filename);
        dest = fopen(destname, "wb");
        if(dest == NULL) 
        {
            fprintf(stderr, "Error: al abrir el archivo %s para escritura.\n", destname);
            exit(3);
        }

        // Codifica el archivo, comprimiendolo.
        encode(source, dest, frecuencias, cola, &tamOriginal, &bitsRelleno);

        // Libera la memoria de la cola de prioridad.
        liberarCola(cola); 
    } else {
        source = fopen(filename, "rb");
        if(source == NULL)
        {
            fprintf(stderr, "Error: al abrrir el archivo %s para lectura.\n", filename);
            exit(3);
        }

        // Archivo destino (descomprimido)
        sprintf(destname, "%s", deleteExtension(filename));
        dest = fopen(destname, "wb");

        // Descodifica el archivo cheras
        decode(source, dest, frecuencias, cola, &tamOriginal, &bitsRelleno);

        // Libera la memoria de la cola de prioridad.
        liberarCola(cola);
    }

    return 0; 
}
