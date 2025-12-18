#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "huffman.h"
#include "cola.h"

typedef struct 
{
    unsigned char buffer;
    int cont;
    FILE *dest;
} BitWriter;

// Prototipos locales
void procesaArgs(int argc, char** argv);
const char* getExtension(const char* filename);
const char* deleteExtension(char* filename);
void encode(const char *root, BitWriter *bw, char **codigos);
void encodeRec(const char *root, BitWriter *bw, char **codigos);
void decode(FILE *source, ColaPrioridad *cola, uint64_t *frequencias);
void frecuenciasGlobal(const char *ruta, uint64_t *frecuencias);
void frecuenciasLocal(const char *ruta, uint64_t *frecuencias);
void crearDirect(const char *ruta);
void escribirBit(BitWriter *bw, char bit);

void procesaArgs(int argc, char** argv) 
{
    if(argc != 3) 
    {
        fprintf(stderr, "[ERROR] Numero de argumentos incorrecto.\n");
        fprintf(stderr, "Uso: %s, <file> <encode/decode>\n", argv[0]);
        exit(1);
    }
    char* filename = argv[1];
    if (strcmp(argv[2], "decode") == 0) 
    {
        const char* ext = getExtension(filename);
        if(strcmp(ext, "cheras") != 0)
        {
            fprintf(stderr, "[ERROR] No es posible decodificar el archivo especificado.\n");
            fprintf(stderr, "CherasZIP esta diseñado para manejar archivos .cheras\n");
            fprintf(stderr, "Si no ha comprimido el archivo con CherasZIP, pruebe con el software original.\n");
            exit(1);
        } 
    } else if (strcmp(argv[2], "encode") != 0) {
        fprintf(stderr, "[ERROR] Modo de uso incorrecto.\n");
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
        fprintf(stderr, "[ERROR] No se pudo asignar memoria para el nuevo nombre de archivo.\n");
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

// Codifica el archivo especificado utilizando los códigos huffman
// previamente calculados. Archivo unico.
void encode(const char *root, BitWriter *bw, char **codigos) 
{
    struct stat info;
    stat(root, &info);

    fputc('F', bw->dest);
    uint16_t lenNombre = strlen(root);
    fwrite(&lenNombre, sizeof(uint16_t), 1, bw->dest);
    fwrite(root, sizeof(char), lenNombre, bw->dest);
    uint64_t tamArchivo = info.st_size;
    fwrite(&tamArchivo, sizeof(uint64_t), 1, bw->dest);

    FILE *archivo = fopen(root, "rb");
    if (archivo == NULL) 
    {
        fprintf(stderr, "[ERROR] Al abrir el archivo %s para lectura.\n", root);
        return;
    }

    int byte;
    printf("[i] Comprimiendo archivo: [%s]\n", root);
    while ((byte = fgetc(archivo)) != EOF) 
    {
        char* codHuffman = codigos[(unsigned char) byte];
        for(int i = 0; codHuffman[i] != '\0'; i++) 
        {
            escribirBit(bw, codHuffman[i]);
        }    
    }

    if (bw->cont > 0) 
    {
        bw->buffer <<= (8 - bw->cont);
        fputc(bw->buffer, bw->dest);
        bw->buffer = 0;
        bw->cont = 0;
    }

    fclose(archivo);
}
 
// Codifica todos los archivos en el directorio especificado y sus subdirectorios
// utilizando los códigos huffman previamente calculados. Directorio completo.
void encodeRec(const char *root, BitWriter *bw, char **codigos)
{
    DIR *dir = opendir(root);
    if (!dir ) 
    {
        fprintf(stderr, "[ERROR] Al intentar abrir el directorio: %s\n", root);
        return;
    }

    struct dirent *entrada;
    struct stat info;

    while ((entrada = readdir(dir)) != NULL) 
    {
        if (strcmp(entrada->d_name, ".") == 0 || strcmp(entrada->d_name, "..") == 0) 
        {
            continue;
        }

        char rutaCompleta[1024];
        snprintf(rutaCompleta, sizeof(rutaCompleta), "%s/%s", root, entrada->d_name);

        stat(rutaCompleta, &info);

        if (S_ISDIR(info.st_mode))
        {
            fputc('D', bw->dest); 
            uint16_t lenNombre = strlen(rutaCompleta);
            fwrite(&lenNombre, sizeof(uint16_t), 1, bw->dest);
            fwrite(rutaCompleta, sizeof(char), lenNombre, bw->dest);
            encodeRec(rutaCompleta, bw, codigos);
        } else
        {
            encode(rutaCompleta, bw, codigos);
        }
    }
    closedir(dir);
}

void decode(FILE *source, ColaPrioridad *cola, uint64_t *frequencias)
{
    size_t leidos = fread(frequencias, sizeof(uint64_t), 256, source);
    if (leidos != 256) 
    {
        fprintf(stderr, "[ERROR] Al leer la tabla de frecuencias del archivo comprimido.\n");
        exit(4);
    }

    Nodo* raiz = huffman(cola);
    int tag;
    while ((tag = fgetc(source)) != EOF)
    {
        uint16_t lenNombre;
        fread(&lenNombre, sizeof(uint16_t), 1, source);

        char nombreArchivo[1024];
        fread(nombreArchivo, sizeof(char), lenNombre, source);
        nombreArchivo[lenNombre] = '\0';

        if (tag == 'D')
        {
            printf("[i] Creando directorio: [%s]\n", nombreArchivo);
            crearDirect(nombreArchivo);
        } else if (tag == 'F')
        {
            uint64_t tamArchivo;
            fread(&tamArchivo, sizeof(uint64_t), 1, source);
            printf("[i] Descomprimiendo archivo: [%s] (Tamaño: %llu bytes)\n", 
                nombreArchivo, (unsigned long) tamArchivo);

            FILE *dest = fopen(nombreArchivo, "wb");
            if (!dest) 
            {
                fprintf(stderr, "[ERROR] Al crear el archivo %s para escritura.\n", nombreArchivo);
                continue;
            }

            Nodo *actual = raiz;
            uint64_t bytesEscritos = 0;
            int byteLeido;

            if (tamArchivo > 0)
            {
                while (bytesEscritos < tamArchivo && (byteLeido = fgetc(source)) != EOF)
                {
                    for (int i = 7; i >= 0; i--)
                    {
                        int bit = (byteLeido >> i) & 1;
                        if (bit == 0) actual = actual->izq;
                        else actual = actual->der;

                        if (actual->izq == NULL && actual->der == NULL)
                        {
                            fputc(actual->c, dest);
                            bytesEscritos++;
                            actual = raiz;

                            if (bytesEscritos >= tamArchivo) break;
                        }
                    }
                }
            }
            fclose(dest);
        } else
        {
            fprintf(stderr, "[ERROR] Etiqueta desconocida en el archivo comprimido.\n");
            exit(5);
        }
    }
}

void crearDirect(const char *ruta) 
{
    #ifdef _WIN32 // Windows
        _mkdir(ruta);
    #else // Unix - Linux
        mkdir(ruta, 0755);
    #endif
}

// Construye el histograma de frecuencias de los bytes que componen
// el archivo fuente (unico) especificado.
void frecuenciasLocal(const char *ruta, uint64_t *frecuencias) 
{
    FILE *archivo = fopen(ruta, "rb");
    if(archivo == NULL) 
    {
        fprintf(stderr, "[ERROR] Al abrir el archivo %s para lectura.\n", ruta);
        exit(3);
    }

    int byte;
    while((byte = fgetc(archivo)) != EOF) 
    {
        frecuencias[(unsigned char) byte]++;
    }  
    fclose(archivo); 
}

// Construye el histograma de frecuencias de los bytes que componen
// todos los archivos en el directorio especificado (y subdirectorios).
void frecuenciasGlobal(const char *ruta, uint64_t *frecuencias)
{
    DIR *dir = opendir(ruta);
    if (!dir ) 
    {
        fprintf(stderr, "[ERROR] Al intentar abrir el directorio: %s\n", ruta);
        return;
    }

    struct dirent *entrada;
    struct stat info;

    while ((entrada = readdir(dir)) != NULL)
    {
        if (strcmp(entrada->d_name, ".") == 0 || strcmp(entrada->d_name, "..") == 0) 
        {
            continue;
        }

        char rutaCompleta[1024];
        snprintf(rutaCompleta, sizeof(rutaCompleta), "%s/%s", ruta, entrada->d_name);
        stat(rutaCompleta, &info);

        if (!S_ISDIR(info.st_mode)) 
        {
            FILE *archivo = fopen(rutaCompleta, "rb");
            if (archivo == NULL) 
            {
                fprintf(stderr, "[ERROR] Al abrir el archivo %s para lectura.\n", rutaCompleta);
                continue;
            }
    
            int byte;
            while ((byte = fgetc(archivo)) != EOF) 
            {
                frecuencias[(unsigned char) byte]++;
            }
            fclose(archivo);
        } else 
        {
            frecuenciasGlobal(rutaCompleta, frecuencias);
        }
    }
}

void escribirBit(BitWriter *bw, char bit) 
{
    bw->buffer <<= 1;
    if (bit == '1') 
    {
        bw->buffer |= 1;
    }
    bw->cont++;

    if (bw->cont == 8) 
    {
        fputc(bw->buffer, bw->dest);
        bw->buffer = 0;
        bw->cont = 0;
    }
}

int main(int argc, char** argv) 
{
    char *filename, *uso;
    char destname[256];
    char bufferHuffman[256];
    char *codigos[256] = {NULL};
    uint64_t frecuencias[256] = {0};     
    
    procesaArgs(argc, argv);
    filename = argv[1];
    uso = argv[2];

    ColaPrioridad *cola;
    cola = (ColaPrioridad*) malloc(sizeof(ColaPrioridad));
    if(cola == NULL) 
    {
        fprintf(stderr, "[ERROR] Al intentar asignar memoria para la cola de prioridad.\n");
        exit(2);
    }
    init(cola);    
    
    if(strcmp(uso, "encode") == 0) 
    {
        struct stat info;
        stat(filename, &info);

        if (S_ISDIR(info.st_mode)) 
        {
            printf("[i] Construyendo tabla de frecuencias para el directorio [%s]...\n", filename);
            frecuenciasGlobal(filename, frecuencias);
        } else 
        {
            printf("[i] Construyendo tabla de frecuencias para el archivo [%s]...\n", filename);
            frecuenciasLocal(filename, frecuencias);
        }

        for (int i = 0; i < 256; i++) 
        {
            if(frecuencias[i] > 0) 
            {
                Nodo* nodo = creaNodo((char) i, frecuencias[i]);
                inserta(cola, nodo);
            }
        }

        Nodo* raiz = huffman(cola);
        codigosHuffman(raiz, 0, codigos, bufferHuffman);

        sprintf(destname, "%s.cheras", filename);
        FILE *dest = fopen(destname, "wb");
        if(dest == NULL)
        {
            fprintf(stderr, "[ERROR] Al intentar crear y abrir el archivo %s.\n", destname);
            exit(3);
        }

        fwrite(&frecuencias, sizeof(uint64_t), 256, dest);
        BitWriter bw = {0, 0, dest};

        
        if (S_ISDIR(info.st_mode)) 
        {
            printf("[i] Comprimiendo archivos de [%s] en [%s]\n", filename, destname);
            encodeRec(filename, &bw, codigos);
        } else 
        {
            encode(filename, &bw, codigos);
        }

        if (bw.cont > 0) 
        {
            bw.buffer <<= (8 - bw.cont);
            fputc(bw.buffer, bw.dest);
        }

        fclose(dest);
        liberarCola(cola);
        printf("[i] Archivo comprimido correctamente. ¡Gracias por usar CherasZIP!\n");  
    } else {
        FILE* source = fopen(filename, "rb");
        if (source == NULL) 
        {
            fprintf(stderr, "[ERROR] Al intentar abrir el archivo %s para lectura.\n", filename);
            exit(3);
        }

        decode(source, cola, frecuencias);

        fclose(source);
        liberarCola(cola);
        printf("[i] Archivo descomprimido correctamente. ¡Gracias por usar CherasZIP!\n");
    }

    return 0; 
}
