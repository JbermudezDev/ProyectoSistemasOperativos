#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <math.h>
#include <stdbool.h>
#include <pthread.h>


//estructura empleada en almacenar el resultado del llamado de los hilos
struct Resultado{
    int distintos_de_cero;
    char nombre[50];
};

//variables globales
int fil = -1;
int col = -1;
int n_hilos = -1;
int** matriz;

//PROTOTIPOS
int** leerMatrizDesdeArchivo(const char* nombre_archivo, int* filas, int* columnas);
void imprimirMatriz();
void* funcionHilo(void * argumentos);
int distintosDeCero(int fila_inicio, int filas_hilo);
void escribirValorEnArchivo(const char* nombre_archivo, int valor);
int leerValorDeArchivo(const char* nombre_archivo);
bool esMatrizDispersa(int distintos_de_cero, int porcentaje);


int main(int argc, char** argv){
  
  int opt;
  char *archivo = NULL;  //-a
  int porcentaje = -1;   //-p
  int filas_antes;
  int columnas_antes;  
  int n_hilos_main = -1;
    
  int total_distintos_de_cero = 0;
  
  //manejo de errores - ingresa menos de 11 argumentos
  if(argc!=11){
    perror("Error, ingrese argumentos de la forma ./hdispersa -f M –c N –a archivo –n n_hilos –p porcentaje\n");
    return 1;
  }
  
  
  while ((opt = getopt(argc, argv, "f:c:a:n:p:")) != -1) { 
    switch (opt) {
      case 'f':
        fil = atoi(optarg);
        break;
      case 'c':
        col = atoi(optarg);
        break;
     case 'a':
        archivo = optarg;
        printf("archivo : %s\n", archivo);
        break;
     case 'n':
        n_hilos = atoi(optarg);
        break;
     case 'p':
        porcentaje = atoi(optarg);
        break;
     default:
        //manejo de errores - ingresa un identificador NO valido
       perror("Uso: ./hdispersa -f M –c N –a archivo –n n_hilos –p porcentaje\n");
       return 1;
      }
    }
  
  //manejo de errores - se olvida el usuario de ingresar algun valor
  if (fil == -1 || col == -1 || archivo == NULL || n_hilos == -1 || porcentaje == -1) {
      perror("Faltan argumentos obligatorios.\n");
      return 1;
  }
  
  
  pthread_t hilos[n_hilos];   //arreglo de identificadores de hilos
  struct Resultado* resultados[n_hilos];  //arreglo punteros de n_hilos a la estructura resultado
                                          //empleado para almacenar los resultados de las operaciones POR HILO

  filas_antes = fil;
  columnas_antes = col;  
  matriz = leerMatrizDesdeArchivo(archivo, &fil, &col);
  
  //manejo de errores - las columnas y las filas que ingresa el usuario deben ser las mismas qeu tiene el archivo
  if(columnas_antes != col || filas_antes != fil){
    perror("las filas y columnas ingresadas no coinciden con las de el archivo ");
    return 1;
  }
  
  //manejo de errores - El número de procesos debe ser menor o igual al número de filas
  if(n_hilos > fil){
    perror("El número de procesos debe ser menor o igual al número de filas\n");
    exit(1);
  }
  
  n_hilos_main = n_hilos;
  
  //entra en un bucle for que crea n_hilos_main hilos y los inicia
  //se reserva memoria para un entero hilo usando malloc y SE IDENTIFICA CADA HILO CON UN INDICE i 
  for(int i=0 ; i < n_hilos_main ; i++){        
    int* hilo = (int *)malloc(sizeof(int));
    *hilo = i;
    //Se crea un hilo con pthread_create utilizando la función funcionHilo como función que el hilo ejecutará, y se le pasa el valor de hilo
    pthread_create(&hilos[i], NULL, funcionHilo, (void*) hilo);
  }
  
  
  
  //bucle for que espera a que todos los hilos finalicen mediante pthread_join y recopila los resultados
  for(int i=0 ; i < n_hilos_main ; i++){  
    resultados[i] = (struct Resultado*) malloc(sizeof(struct Resultado));
    pthread_join(hilos[i], (void**)&resultados[i]);
        
    if (resultados[i]->distintos_de_cero == 255) {      
      total_distintos_de_cero += leerValorDeArchivo(resultados[i]->nombre);
    }else{
      total_distintos_de_cero+=resultados[i]->distintos_de_cero;
    }
    
  }
  
  //prints con información 
  printf("Matriz %dx%d \n",fil,col);
  printf("Porcentaje de ceros = %d%% \n",((fil*col)-total_distintos_de_cero)*100/(fil*col));
  printf("Porcentaje distintos de cero = %d%% \n",(total_distintos_de_cero)*100/(fil*col));
  
  printf("/////////////////////////////////\n");
  printf("///////// R E S U L T A D O//////\n");
  printf("/////////////////////////////////\n");
  //determinar si la matriz es dispersa o no
  if(esMatrizDispersa( total_distintos_de_cero, porcentaje)){
    printf("Es matriz dispersa: El total de ceros en la matriz es %d más del %d%%.\n", (fil*col)-total_distintos_de_cero, porcentaje);
  }else{
    printf("No es matriz dispersa: El total de ceros en la matriz es %d menos del %d%%.\n", (fil*col)-total_distintos_de_cero, porcentaje);
  }
  printf("/////////////////////////////////\n");
 
}


//función que lee la matriz desde el archivo
int** leerMatrizDesdeArchivo(const char* nombre_archivo, int* filas, int* columnas) {
    FILE* archivo = fopen(nombre_archivo, "r");
    if (archivo == NULL) {
        perror("Error al abrir el archivo");
        return NULL;
    }
  
    int numFilas = 0;
    int numColumnas = 0;

    char caracter;
  
    //cuenta el numero de columnas
    while ((caracter = fgetc(archivo)) != EOF && caracter != '\n') {
        if (caracter != ' ') {
            numColumnas++;
            while ((caracter = fgetc(archivo)) != EOF && caracter != ' ' && caracter != '\n') {
                // Avanzamos hasta el siguiente espacio o fin de línea
            }
        }
    }

    rewind(archivo);//reinicia el archivo para leerlo nuevamente

    //cuenta el numero de filas
    while ((caracter = fgetc(archivo)) != EOF) {
        if (caracter == '\n') {
            numFilas++;
        }
    }

    rewind(archivo);

    //crea memoria para la matriz
    int** matriz = (int**)malloc(numFilas * sizeof(int*));
    for (int i = 0; i < numFilas; i++) {
        matriz[i] = (int*)malloc(numColumnas * sizeof(int));
    }

    //agrega la matriz del archivo a la variable int** de la función a ser retornada
    for (int i = 0; i < numFilas; i++) {
        for (int j = 0; j < numColumnas; j++) {
            if (fscanf(archivo, "%d", &matriz[i][j]) != 1) {
                perror("Error al leer la matriz desde el archivo");
                fclose(archivo);
                return NULL;
            }
        }
    }

    //cierra el archivo
    fclose(archivo);
    //reasigna
    *filas = numFilas;
    *columnas = numColumnas;

    return matriz;
}

//funcion que imprime la matriz
void imprimirMatriz() {
    for (int i = 0; i < fil; i++) {
        for (int j = 0; j < col; j++) {
            printf("%d ", matriz[i][j]);
        }
        printf("\n");
    }
}

//funcion que hace el manejo de los hilos
void* funcionHilo(void * argumentos){
  int hilo = *(int *)argumentos;
  struct Resultado* resultado = (struct Resultado*) malloc(sizeof(struct Resultado));

  //printf(".../ \n acceso exitoso al hilo %d\n.../",hilo+1);
  
  int retorno;//retorno del entero 
  
  int filas_restantes = fil;
  
  //entra a un for que itera n_hilos veces
  for(int i=0; i<n_hilos; i++){
    int fila_comienzo = fil - filas_restantes; //calcula la fila que será procesada por el hilo actual
    int procesos_restantes = n_hilos - i;      //calcula los procesos restantes
    int filas_hilo_actual = ceil(filas_restantes/procesos_restantes);  //número de filas que se le asignara al hilo actual
    
    // Si son iguales, significa que el hilo actual es el que debe realizar un cálculo en particular
    if(hilo == i){
      printf("hilo %d evalúa [%d] filas desde la fila %d hasta la fila %d \n",hilo,filas_hilo_actual,fila_comienzo,fila_comienzo+filas_hilo_actual-1);
      int retorno = distintosDeCero(fila_comienzo,filas_hilo_actual);
      //si es mayor a 255 se escribe la info en un archivo
      if(retorno>254){
        sprintf(resultado->nombre, "resultado_desde_%d_hasta_%d.txt", fila_comienzo, fila_comienzo+filas_hilo_actual-1);
        escribirValorEnArchivo(resultado->nombre, retorno);
        resultado->distintos_de_cero = 255;    
      }else{
        resultado->distintos_de_cero = retorno;
      }  
    }
    
    
    filas_restantes -= filas_hilo_actual;
  }
  
  return resultado;
  //return &resultado;
}

//funcion que cuenta los digitos distintos de 0
int distintosDeCero(int fila_inicio, int filas_hilo){
  int distintos_de_cero = 0;
  for(int i=fila_inicio; i < (fila_inicio + filas_hilo); i++){
    for(int j = 0; j < col; j++){
      if(matriz[i][j]!=0){
        distintos_de_cero++;
      }      
    }
  }
  return distintos_de_cero;
}

//función que escribe los valores en el archivo
void escribirValorEnArchivo(const char* nombre_archivo, int valor) {
    FILE* archivo = fopen(nombre_archivo, "w");
    if (archivo == NULL) {
        perror("Error al crear el archivo");
        return;
    }

    fprintf(archivo, "%d", valor);

    fclose(archivo);
}

//función que lee los valores en el archivo
int leerValorDeArchivo(const char* nombre_archivo) {
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


//función que permite determinar si la matriz es dispersa o no comparando los distintos de 0 con el corcentaje que el usu ingresa
bool esMatrizDispersa(int distintos_de_cero, int porcentaje){
  int total_de_elementos = fil*col;
  int porcentaje_de_ceros = (total_de_elementos-distintos_de_cero)*100/total_de_elementos;
  if(porcentaje<=porcentaje_de_ceros){
    return true;
  }else{
    return false;
  }
}