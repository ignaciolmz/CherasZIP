# CherasZIP
## Descripción
Compresor de archivos simple basado en algoritmo de compresión de Huffman. 
Permite codificar y descodificar archivos empleando un método de compresión 
eficiente y sin pérdidas, cuya idea principal es asignar códigos binarios más 
cortos a los caracteres aparecidos con mayor frecuencia.<br>

Proyecto personal con fines puramente académicos.<br>

## Funcionamiento

Obtener el ejecutable CherasZIP.exe del directorio build/.<br>
Actualmente, su uso se encuentra limitado a plataformas Windows mediante CMD.<br>
<pre>
  Modo de uso (compresión) - Obtención del archivo comprimido <i>file.extension.cheras</i>
  >> CherasZIP.exe file.extension encode
  
  Modo de uso (descompresión) - específicamente diseñado para archivos .cheras, propios de CherasZIP.
  >> CherasZIP.exe file.extension.cheras decode
</pre>

## Características
- Compresión de archivos mediante algoritmo de Huffman.
- Descompresión precisa y sin pérdida.
- Interfaz simple por línea de comandos.
- Implementado en C.

Actualmente no permite su uso con directorios. Esta implementación se encuentra en desarollo como primera ampliación.<br>
Utiliza exclusivamente Huffman, si bien se plantea como ampliación posterior emplear Deflate (LZ77 + Huffman).<br>

### Estructura del proyecto
```text
CherasZIP/
├── build/
│   └── CherasZIP.exe
├── include/
│   ├── cola.h
│   └── huffman.h
├── src/
│   ├── cola.c
│   ├── huffman.c
│   └── main.c
└── Makefile
```
