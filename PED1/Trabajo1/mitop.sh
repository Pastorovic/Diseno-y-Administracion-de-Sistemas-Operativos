#!/bin/bash

# ---------------------
# Funciones y variables
# ---------------------

# Funcion que se crea para indicar que un numero mayor a 999999 es igual a ese numero dividido entre 1000 y con una m al final
function setInMiles() {
 if [ $1 -gt 99999 ]; then
  var=$(expr $1 / 1000)
  echo $var"m"
 else
  echo $1
 fi
}

# Funcion para imprimir RT (real time) cuando la prioridad es -100
function setPriority() {
 if [ $1 == -100 ]; then
  echo "RT"
 else
  echo $1
 fi
}

# Funcion para pasar segundos a formato mm:ss.cc. Como se le pasara como parametro tics de reloj, se pasara a segundos
function convertSec() {
 ss=$(expr "scale=2;$1/`getconf CLK_TCK`" | bc)
 mm=$(expr "$ss / 60" | bc)
 ss=$(expr "scale=2;($ss-($mm*60.0))" | bc)
 LANG=C printf "%2d:%05.02f" $mm $ss
}

# Funcion para eliminar los parentesis del campo command. Se utiliza el comando sed
function removeBrackets() {
  echo $(echo $(echo $1 | sed 's/(//g') | sed 's/)//g')
}

# Variable que guardara el numero de procesos. Ademas se crea una variable para cada uno de los posibles estados y llevar su cuenta
ntask=0
sleeping=0
running=0
stopped=0
zombie=0

# Variable que guarda el uso total de la cpu
cpuUsage=0

# ------------------
# Obtencion de datos
# ------------------

# Se obtienen los pids y usuarios de todos los procesos y se guardan en una variable
pids=$(ls -l /proc | awk '$9 ~ /[0-9]/  {print $9}')

# Se mide el tiempo actual de uso de la cpu. Se obtiene mediante la suma de todos los estados de la cpu (tuser+tnice+tsystem+tidle+tiowait+tirq+tsoftirq+tsteal+tguest)
tcpu1=$(cat "/proc/stat" | awk '$1 == "cpu" {print ($2+$3+$4+$5+$6+$7+$8+$9+$10)}')

# Se obtienen de /proc/[PID]/stat los tiempos de uso de la cpu en modo usuario y nucleo y se suman
for pid in $pids; do
 if [ -f "/proc/$pid/stat" ]; then
  tpcpu1[$pid]=$(cat "/proc/$pid/stat" | awk '{print ($14+$15)}')
 fi
done

# ----------------------
# Calculo porcentaje cpu
# ----------------------

# Para calcular el uso total de cpu se realiza la misma suma que para tcpu1 pero sin el tiempo de idle
cpuUsage1=$(cat "/proc/stat" | awk '$1 == "cpu" {print ($2+$3+$4+$6+$7+$8+$9+$10)}')

# Se duerme durante 1 segundo
sleep 1

# Se obtienen el usuario de cada proceso, numero de procesos, sus estados y numero de tipos
for pid in $pids; do
 if [ -f "/proc/$pid/stat" ]; then
  user[$pid]=$(ls -l /proc | awk '$9 == '$pid'  {print $3}')
  let ntask=ntask+1
  state[$pid]=$(cat "/proc/$pid/stat" | awk '{print $3}')
  if [ ${state[$pid]} = 'R' ]; then
   let running=$running+1
  elif [ ${state[$pid]} = 'S' ]; then
   let sleeping=$sleeping+1
  elif [ ${state[$pid]} = 'T' ]; then
   let stopped=$stopped+1
  elif [ ${state[$pid]} = 'Z' ]; then
   let zombie=$zombie+1
  fi
 fi
done

# Se obtienen los tiempos de uso de la cpu en modo usuario y nucleo y se suman de nuevo
for pid in $pids; do
 if [ -f "/proc/$pid/stat" ]; then
  tpcpu2[$pid]=$(cat "/proc/$pid/stat" | awk '{print ($14+$15)}') # Valor que tendra el campo TIME+
 fi
done

# Se mide el tiempo actual de nuevo. Se le resta el tiempo medido anteriormente para poder asi hallar el tiempo de uso de la cpu.
tcpu2=$(cat "/proc/stat" | awk '$1 == "cpu" {print ($2+$3+$4+$5+$6+$7+$8+$9+$10)}')
tcpu=$(expr $tcpu2 - $tcpu1)

# Se obtiene la segunda muestra y se calcula el  porcentaje de uso de la cpu
cpuUsage2=$(cat "/proc/stat" | awk '$1 == "cpu" {print ($2+$3+$4+$6+$7+$8+$9+$10)}')
cpuUsage=$(echo "scale = 4;(($cpuUsage2-$cpuUsage1)/$tcpu)*100" | bc)

# Se obtiene el porcentaje de cpu
for pid in $pids; do
 if [ -f "/proc/$pid/stat" ]; then
  pcpu[$pid]=$(expr "scale=4;((${tpcpu2[$pid]}-${tpcpu1[$pid]})/$tcpu)*100" | bc)
 fi
done

# Se ordena el array donde se encuentran los porcentajes de cpu mediante su pid
sortedpid=($(for k in "${!pcpu[@]}"; do
              echo $k ${pcpu["$k"]}
             done | sort -rnk2 -nk1))

# ------------------
# Calculo de memoria
# ------------------
# Se obtienen la memoria total, libre y usada
memTotal=$(cat /proc/meminfo | awk '/MemTotal/ {print $2}')
memFree=$(cat /proc/meminfo | awk '/MemFree/ {print $2}')
memUsed=$(expr "$memTotal - $memFree" | bc)

# Para el calculo del porcentaje de memoria por proceso primero se calcula el tamano de pagina
mapped=$(cat /proc/meminfo | awk '/Mapped/ {print $2}')
nrMapped=$(cat /proc/vmstat | awk '/nr_mapped/ {print $2}')
pageSize=$(expr "$mapped / $nrMapped" | bc)

# Para calcular el porcentaje de memoria usada se divide el numero de paginas que usa el proceso en memoria por el tamano de la pagina dividido entre la memoria total. Ademas se calculan el numero total de procesos junto con el array que indica sus estados y el numero de estados de estos
for pid in $pids; do
 if [ -f "/proc/$pid/stat" ]; then
  rss=$(cat "/proc/$pid/stat" | awk '{print $24}') # Numero de paginas del proceso
  pmem[$pid]=$(expr "scale=4;(($rss*$pageSize)/$memTotal)*100" | bc)
 fi
done

# ---------
# Impresion
# ---------
# Numero de procesos, uso total de la cpu, memoria total, utilizada y libre.
LANG=C printf "Task: %5d, %3d running, %3d sleeping, %3d stopped, %3d zombie\nCPU(s): %-.1f%%\nMem:    %dk total,   %dk used,   %dk free\n" $ntask $running $sleeping $stopped $zombie $cpuUsage $memTotal $memUsed $memFree

# Cabecera con fondo blanco y letras negras
echo -e '\e[47;30m  PID USER        PR   VIRT  S  %CPU  %MEM     TIME+   COMMAND\x1B[K\e[0m'

# Se recorre desde 0 a 19 de dos en dos y se saca el pid del array ordenado anteriormente. En las posiciones pares se encuentran los pids y en las impares los porcentajes de uso de cpu de estos
aux=${#sortedpid[@]}-2
for ((i=0; i<19; i=$i+2));do
 pid=${sortedpid[$i]}
 # Para parecerse mas a top, si el porcentaje de cpu es 0, se imprimiran de menor a mayor por pid. En el array sortedpid estan ordenados de mayor a menor
 if [ ${pcpu[$pid]} == 0 ]; then
  pid=${sortedpid[$aux]}
  let aux=aux-2
 fi
 if [ -f "/proc/$pid/stat" ]; then
  # Se obtienen los datos restantes de proc/[pid]/stat. Se introducen en un array en el cual se introduce en la posicion 0 el pid otra vez para que asi al obtener los datos despues de dicho array esten en las mismas posiciones que en stat al acceder con awk
  statPid=($pid $(cat "/proc/$pid/stat"))
  LANG=C printf "%5s %-10s%4s %6s  %s %5.1f %5.1f  %9s  %s\n" $pid ${user[$pid]} `setPriority ${statPid[18]}` `setInMiles $(expr ${statPid[23]} / 1024)` ${state[$pid]} ${pcpu[$pid]} ${pmem[$pid]} `convertSec ${tpcpu2[$pid]}` `removeBrackets ${statPid[2]}`
 fi
done
