/* Tomado de https://gist.github.com/sowmyagowri/f4bde25b3ebba1e6e9930afbdf31a25b */


/*
 Este programa toma la solucion de un Sudoku y determina si es una solucion valida o no.
 Esta validacion se hace utilizando un unico hilo y usando 27 hilos. 

 Los 27 hilos son los siguientes:
  - 1 por cada region 3x3 (total 9 hilos)
  - 1 por cada columna (total 9 hilos)
  - 1 por cada fila (total 9 hilos)

Cada hilo devuelve un valor entero de 1 que indica que la region correspondiente a ese hilo
es valida. El programa espera a que todos los hilos terminen su ejecucion y si todos retornan
1 la solucion es valida, de lo contrario no lo es. Tambien retorna el time que le tomo al programa
hacer la validacion.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>   //Libreria para escribir programas multihilo
#include <iostream>
#include <chrono>      //Nos permite medir el tiempo que tarda el codigo en hacer algo

#define num_threads 27

using namespace std;
using namespace std::chrono;

/*
Esta estructura guarda los parametros que se le pasan a un hilo.
Especificando en que parte del Sudoku deberia empezar a verificar.
 */
typedef struct {
    int row; 
    int col;
    int (* board)[9];
} parameters;


int result[num_threads] = {0};          //Arreglo de ceros en donde se actualizaran a 1 si la region correspondiente es valida
void *check_grid(void *params);         //Funcion que revisa una region 3x3
void *check_rows(void *params);         //Funcion que revisa una fila
void *check_cols(void *params);         //Funcion que revisa una columna
int sudoku_checker(int sudoku[9][9]);   //Funcion que revisa todo el Sudoku con un unico hilo


int main(void) {
    int sudoku[9][9] =
    {
            {6, 2, 4, 5, 3, 9, 1, 8, 7},
            {5, 1, 9, 7, 2, 8, 6, 3, 4},
            {8, 3, 7, 6, 1, 4, 2, 9, 5},
            {1, 4, 3, 8, 6, 5, 7, 2, 9},
            {9, 5, 8, 2, 4, 7, 3, 6, 1},
            {7, 6, 2, 3, 9, 1, 4, 5, 8},
            {3, 7, 1, 9, 5, 6, 8, 4, 2},
            {4, 9, 6, 1, 8, 2, 5, 7, 3},
            {2, 8, 5, 4, 7, 3, 9, 1, 6}
     };

    //Inicia a contabilizar el tiempo que tarda en verificar el Sudoku con un hilo
    steady_clock::time_point start_time_single_thread = steady_clock::now();

    if(sudoku_checker(sudoku))
        printf("Con un solo hilo: Solucion INCORRECTA del sudoku\n");
    else
        printf("Con un solo hilo: Solucion CORRECTA del sudoku\n");

    // Termina de contabilizar el tiempo que tarda en verificar el Sudoku con un hilo 
    steady_clock::time_point end_time_single_thread = steady_clock::now();
    duration<double> elapsed_time_single_thread = duration_cast<duration<double>>(end_time_single_thread - start_time_single_thread);

    cout << endl << "Tiempo total usando un solo hilo: " << elapsed_time_single_thread.count() << " segundos" << endl << endl;

    //Inicia a contabilizar el tiempo que tarda en verificar el Sudoku con los 27 hilos
    steady_clock::time_point start_time_threads = steady_clock::now();

    pthread_t threads[num_threads];   //En este arreglo se van a guardar los 27 identificadores para cada hilo
    int threadIndex = 0;

    // ====== Creacion de los 27 hilos ======
    //La funcion pthread_create nos permite crear un hilo con las siguientes caracteristicas:
    //1 parametro: identificador del hilo, 2 parametro: atributos del parametro, 3 parametro: Funcion que el hilo va a ejecutar y 4 parametro: parametros de la funcion 

    for (int i = 0; i < 9; i++) {       //filas
        for (int j = 0; j < 9; j++) {   //columnas
            //Creacion de hilos para las regiones 3x3
            if (i%3 == 0 && j%3 == 0) {
                parameters *gridData = (parameters *) malloc(sizeof(parameters));
                gridData->row = i;
                gridData->col = j;
                gridData->board = sudoku;
                pthread_create(&threads[threadIndex++], NULL, check_grid, gridData);
            }
            //Creacion de hilos para las filas
            if (j == 0) {
                parameters *rowData = (parameters *) malloc(sizeof(parameters));
                rowData->row = i;
                rowData->col = j;
                rowData->board = sudoku;
                pthread_create(&threads[threadIndex++], NULL, check_rows, rowData);
            }
            //Creacion de hilos para las columnas
            if (i == 0) {
                parameters *columnData = (parameters *) malloc(sizeof(parameters));
                columnData->row = i;
                columnData->col = j;
                columnData->board = sudoku;
                pthread_create(&threads[threadIndex++], NULL, check_cols, columnData);
            }
        }
    }

    // ======= Espera a que todos los hilos terminen sus tareas =======
    //Los parametros son el identificador del hilo y el valor que retorna la funcion ejecutada por el hilo
    for (int i = 0; i < num_threads; i++)
        pthread_join(threads[i], NULL);


    //Si alguno de los resultados de la validacion es 0, significa que la solucion es invalida
    for (int i = 0; i < num_threads; i++) {
        if (result[i] == 0) {
            cout << "Con varios hilos: Solucion INCORRECTA del sudoku" << endl;

            // Termina de contabilizar el tiempo que tarda en verificar el Sudoku con los 27 hilos
            steady_clock::time_point end_time_threads = steady_clock::now();
            duration<double> elapsed_time_threads = duration_cast<duration<double>>(end_time_threads - start_time_threads);
            cout << endl << "Tiempo total utilizando 27 hilos: " << elapsed_time_threads.count() << " segundos" << endl;
            return 1;
        }
    }
    cout << "Con varios hilos: Solucion CORRECTA del sudoku" << endl;
    // Termina de contabilizar el tiempo que tarda en verificar el Sudoku con los 27 hilos
    steady_clock::time_point end_time_threads = steady_clock::now();
    duration<double> elapsed_time_threads = duration_cast<duration<double>>(end_time_threads - start_time_threads);
    cout << endl << "Tiempo total utilizando 27 hilos: " << elapsed_time_threads.count() << " segundos" << endl;
}


//Funcion que revisa que en una region 3x3 esten los numeros del 1-9 sin repetirse
//En el arreglo validarray se iran actualizando con un 1 cada vez que lea un numero de la region 3x3
void *check_grid(void * params) {
    parameters *data = (parameters *) params;
    int startRow = data->row;
    int startCol = data->col;
    int validarray[10] = {0};
    for (int i = startRow; i < startRow + 3; ++i) {
        for (int j = startCol; j < startCol + 3; ++j) {
            int val = data->board[i][j];
            if (validarray[val] != 0) {
				// printf("[*check_grid] Error en la cuadricula que inicia en la fila %d, columna %d, \n",startRow,startCol);
                pthread_exit(NULL);
            } else {
                validarray[val] = 1;
			}
        }
    }
    //Si se alcanza este punto significa que la region es valida y se actualiza a 1 en el arreglo result
    result[startRow + startCol/3] = 1; 
    pthread_exit(NULL);
}


//Funcion que revisa que en una fila esten los numeros del 1-9 sin repetirse
void *check_rows(void *params) {
    parameters *data = (parameters *) params;
    int row = data->row;
    int validarray[10] = {0};
    for (int j = 0; j < 9; j++) {
        int val = data->board[row][j];
        if (validarray[val] != 0) {
			// printf("[*check_rows] Error en la fila %d\n",row);
            pthread_exit(NULL);
		} else {
            validarray[val] = 1;
		}
    }
    //Si se alcanza este punto significa que la fila es valida y se actualiza a 1 en el arreglo result
    result[9 + row] = 1; 
    pthread_exit(NULL);
}


//Funcion que revisa una columna esten los numeros del 1-9 sin repetirse
void *check_cols(void *params) {
    parameters *data = (parameters *) params;
    int col = data->col;
    int validarray[10] = {0};
    for (int i = 0; i < 9; i++) {
        int val = data->board[i][col];
        if (validarray[val] != 0) {
			// printf("[*check_cols] Error en la columna %d, \n",col);
            pthread_exit(NULL);
		} else {
            validarray[val] = 1;
		}
    }
    //Si se alcanza este punto significa que la columna es valida y se actualiza a 1 en el arreglo result
    result[18 + col] = 1; 
    pthread_exit(NULL);
 }



//Revisa que cada columna y fila tengan los numeros del 1-9 sin repetirse (funcion usada para la la validacion con un solo hilo)
int check_line(int input[9]) {
    int validarray[10] = {0};
    for (int i = 0; i < 9; i++) {
        int val = input[i];
        if (validarray[val] != 0)
            return 1;
        else
            validarray[val] = 1;
    }
    return 0;
}


//Checks each 3*3 grid if it contains all digits 1-9
//Revisa que cada region 3x3 tenga los numeros del 1-9 sin repetirse (funcion usada para la la validacion con un solo hilo)
int check_grid(int sudoku[9][9]) {
    int temp_row, temp_col;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            temp_row = 3 * i;
            temp_col = 3 * j;
            int validarray[10] = {0};
            for(int p=temp_row; p < temp_row+3; p++) {
                for(int q=temp_col; q < temp_col+3; q++) {
                    int val = sudoku[p][q];
                    if (validarray[val] != 0)
                        return 1;
                    else
                        validarray[val] = 1;
                }
            }
        }
    }
    return 0;
}


//Revisa si la solucion del Sudoku es valida o no con un solo hilo
int sudoku_checker(int sudoku[9][9]) {
    for (int i=0; i<9; i++) {
        if(check_line(sudoku[i]))
            return 1;
        int check_col[9];
        for (int j=0; j<9; j++)
            check_col[j] = sudoku[i][j];
        if(check_line(check_col))
            return 1;
        if(check_grid(sudoku))
            return 1;
    }
    return 0;
}
