#!/bin/bash
#este archivo es un scrip que compila el trabajo 2 y luego ejecuta Ej2
#gcc esto y lo otro

gcc ./Trabajo2/fuente1.c -o ./Trabajo2/Ej1
gcc ./Trabajo2/fuente2.c -o ./Trabajo2/Ej2
gcc ./Trabajo2/fuente3.c -o ./Trabajo2/Ej3

./Trabajo2/Ej1

rm ./Trabajo2/Ej1
rm ./Trabajo2/Ej2
rm ./Trabajo2/Ej3

