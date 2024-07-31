#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <math.h>
#include <stdbool.h>

//PROTOTIPOS:
int** leerMatrizDesdeArchivo(const char* nombreArchivo, int* filas, int* columnas);
void imprimirMatriz(int** matriz, int filas, int columnas);
int distintosDeCero(int **matriz, int columnas,int fila_inicio, int filas_proceso);
void escribir_valor_en_archivo(const char* nombre_archivo, int valor);
int leer_valor_de_archivo(const char* nombre_archivo);
bool esMatrizDispersa(int filas, int columnas, int distintos_de_cero, int porcentaje);



int main(int argc, char** argv){
  
  int opt;
  int filas = -1;        //-f
  int columnas = -1;     //-c
  char *archivo = NULL;  //-a
  int nprocesos = -1;    //-n
  int porcentaje = -1;   //-p
  int **matriz;
  int filas_antes;
  int columnas_antes;  
  int pid_hijo;    
  int filas_restantes;
  int fila_comienzo; 
  int filas_proceso_actual;  
  int total_distintos_de_cero = 0;
  
  //manejo de errores - ingresa menos de 11 argumentos
  if(argc!=11){
    perror("Error, ingrese argumentos de la forma ./pdispersa -f M –c N –a archivo –n nprocesos –p porcentaje");
    return 1;
  }
  
  //las opciones validas en "" (requieren un arguemnto) lo que no encuentre asigna -1
  //optarg almacena el argumento asociado con la ultima opcion procesada
  while ((opt = getopt(argc, argv, "f:c:a:n:p:")) != -1) { 
    switch (opt) {
      case 'f':
        filas = atoi(optarg);
        break;
      case 'c':
        columnas = atoi(optarg);
        break;
     case 'a':
        archivo = optarg;
        printf("archivo : %s\n", archivo);
        break;
     case 'n':
        nprocesos = atoi(optarg);
        break;
     case 'p':
        porcentaje = atoi(optarg);
        break;
     default:
        //si meten un selector que no va, manda alerta
        fprintf(stderr, "Uso: %s -f M -c N -a archivo -n nprocesos -p porcentaje\n", argv[0]);
        exit(EXIT_FAILURE);
      }
    }
  
    
  //si están los selectores que son pero no le asignaron nada como argumento da alerta
  if (filas == -1 || columnas == -1 || archivo == NULL || nprocesos == -1 || porcentaje == -1) {
      perror("Faltan argumentos obligatorios.\n");
      return 1;
  }

  //manejo de errores - las columnas y las filas que ingresa el usuario deben ser las mismas qeu tiene el archivo
  filas_antes = filas;
  columnas_antes = columnas;  
  matriz = leerMatrizDesdeArchivo(archivo, &filas, &columnas);
  if(columnas_antes != columnas || filas_antes != filas){
    perror("las filas y columnas ingresadas no coinciden con las de el archivo");
    return 1;
  }
  
  //manejo de errores - El número de procesos debe ser menor o igual al número de filas
  if(nprocesos > filas){
    perror("El número de procesos debe ser menor o igual al número de filas\n");
    exit(1);
  }
  
  filas_restantes = filas;
  
  for(int i=0 ; i < nprocesos ; i++){//crea los n procesos
    
    filas_proceso_actual = ceil(filas_restantes/(nprocesos - i)); //filas asignadas al proceso actual
    pid_hijo = fork();
    
    if(pid_hijo == 0){//Aquí entra el hijo
      
      int elementos_distintos_de_cero = distintosDeCero(matriz, columnas, filas - filas_restantes , filas_proceso_actual); 
      
      if(elementos_distintos_de_cero > 254){
        char nombre_archivo[50];
        sprintf(nombre_archivo, "resultado_proceso_%d.txt", getpid());         //Se crea el nombre del archivo
        escribir_valor_en_archivo(nombre_archivo, elementos_distintos_de_cero);//Crea el archivo y escribe el valor        
        exit(255);                                                             //retorna 255, indicando que debe leer el valor en un archivo
      }
      else{
        exit(elementos_distintos_de_cero);
      }
             
    }
    else{
      printf("El proceso %d evalúa [%d filas] desde la fila %d hasta la fila %d \n",i,filas_proceso_actual,filas - filas_restantes,(filas - filas_restantes-1)+filas_proceso_actual);
    }
    
    filas_restantes -= filas_proceso_actual;//filas que restan por asignar
    
  }
   
  
  for(int i=0; i<nprocesos; i++){                  //El padre espera el resultado de los n hijos
    int estado;                                     //Variable en la que se guardara el estado del proceso
    int proceso_terminado = waitpid(-1, &estado, 0);//variable en la que se guarda el pid del proceso terminado
    if(proceso_terminado>0){
        if (WIFEXITED(estado)) {
          int resultado_hijo = WEXITSTATUS(estado); //Variable en la que se guarda el retorno del hijo
          
          if(resultado_hijo == 255){                //Si el hijo retorna 255, el padre lee el resultado en un archivo
            char nombre_archivo[50];            
            sprintf(nombre_archivo, "resultado_proceso_%d.txt", proceso_terminado);//Nombre que debe tener el archivo
            int valorArchivo = leer_valor_de_archivo(nombre_archivo);
            total_distintos_de_cero += valorArchivo;          
          }else{
            total_distintos_de_cero +=  resultado_hijo;
          }
          
          
        }
        else{
          printf("El proceso hijo con PID %d terminó de forma anormal.\n", proceso_terminado);
          exit(1);
        }
      }
  }
  
  printf("Matriz %dx%d \n",filas,columnas);
  printf("Porcentaje de ceros = %d%% \n",((filas*columnas)-total_distintos_de_cero)*100/(filas*columnas));
  printf("Porcentaje distintos de cero = %d%% \n",(total_distintos_de_cero)*100/(filas*columnas));
  
  printf("/////////////////////////////////\n");
  printf("///////// R E S U L T A D O//////\n");
  printf("/////////////////////////////////\n");
  
  if(esMatrizDispersa( filas, columnas, total_distintos_de_cero, porcentaje)){
    printf("Es matriz dispersa: El total de ceros en la matriz es %d más del %d%%.\n", (filas*columnas)-total_distintos_de_cero, porcentaje);
  }else{
    printf("No es matriz dispersa: El total de ceros en la matriz es %d menos del %d%%.\n", (filas*columnas)-total_distintos_de_cero, porcentaje);
  }
  
  printf("/////////////////////////////////\n");
  printf("/////////////////////////////////\n");
  
}

//funcion que lee la matriz de un archivo y la retorna
int** leerMatrizDesdeArchivo(const char* nombreArchivo, int* filas, int* columnas) {
    FILE* archivo = fopen(nombreArchivo, "r");
    if (archivo == NULL) {
        perror("Error al abrir el archivo");
        return NULL;
    }

    // Variables para contar filas y columnas
    int numFilas = 0;
    int numColumnas = 0;

    // Contamos el número de columnas en la primera fila
    char caracter;
    //lee los caracteres de un archivo hasta que encuentre el final del archivo(EOF) o salto de linea \n
    while ((caracter = fgetc(archivo)) != EOF && caracter != '\n') {
        if (caracter != ' ') {
            numColumnas++;
            while ((caracter = fgetc(archivo)) != EOF && caracter != ' ' && caracter != '\n') {
                // Avanzamos hasta el siguiente espacio o fin de línea
            }
        }
    }

    // Regresamos al inicio del archivo
    rewind(archivo);

    // Contamos el número de filas
    while ((caracter = fgetc(archivo)) != EOF) {
        if (caracter == '\n') {
            numFilas++;
        }
    }

    // Regresamos al inicio del archivo nuevamente
    rewind(archivo);

    // Reservamos memoria para la matriz
    int** matriz = (int**)malloc(numFilas * sizeof(int*));
    for (int i = 0; i < numFilas; i++) {
        matriz[i] = (int*)malloc(numColumnas * sizeof(int));
    }

    // Leemos la matriz desde el archivo
    for (int i = 0; i < numFilas; i++) {
        for (int j = 0; j < numColumnas; j++) {
            if (fscanf(archivo, "%d", &matriz[i][j]) != 1) {
                perror("Error al leer la matriz desde el archivo");
                fclose(archivo);
                return NULL;
            }
        }
    }

    fclose(archivo);

    *filas = numFilas;
    *columnas = numColumnas;

    return matriz;
}

//funcion que imprime la matriz
void ImprimirMatriz(int** matriz, int filas, int columnas) {
    for (int i = 0; i < filas; i++) {
        for (int j = 0; j < columnas; j++) {
            printf("%d ", matriz[i][j]);
        }
        printf("\n");
    }
}

//funcion que cuenta los elementos distintos de 0
int distintosDeCero(int **matriz, int columnas,int fila_inicio, int filas_proceso){
  int distintos_de_cero = 0;
  for(int i=fila_inicio; i < (fila_inicio + filas_proceso); i++){
    for(int j = 0; j < columnas; j++){
      if(matriz[i][j]!=0){
        distintos_de_cero++;
      }      
    }
  }
  return distintos_de_cero;
}

//funcio que dado que los distintos de 0 fueron > a 255 los escribe en un archivo para que acceda el padre
void escribir_valor_en_archivo(const char* nombre_archivo, int valor) {
    FILE* archivo = fopen(nombre_archivo, "w");
    if (archivo == NULL) {
        perror("Error al crear el archivo");
        return;
    }

    fprintf(archivo, "%d", valor);

    fclose(archivo);
}

//funcion que lee el archivo de los distintos de 0
int leer_valor_de_archivo(const char* nombre_archivo) {
    FILE* archivo = fopen(nombre_archivo, "r");
    if (archivo == NULL) {
        perror("Error al abrir el archivo");
        exit(1);
    }

    int valor;
    if (fscanf(archivo, "%d", &valor) != 1) {
        perror("Error al leer el valor del archivo");
        exit(1);
    }

    fclose(archivo);
    return valor;
}

//funcion que determina si la matriz es o NO dispersa
bool esMatrizDispersa(int filas, int columnas, int distintos_de_cero, int porcentaje){
  int total_de_elementos = filas*columnas;
  int porcentaje_de_ceros = (total_de_elementos-distintos_de_cero)*100/total_de_elementos;
  if(porcentaje<=porcentaje_de_ceros){
    return true;
  }else{
    return false;
  }
}
