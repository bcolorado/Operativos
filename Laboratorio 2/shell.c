#include <sys/types.h>  /*Define los tipos de datos utilizados en el código fuente del sistema*/

#include <stdio.h>      /*Tiene la información necesaria para incluir las funciones relacionadas con la entrada/salida
                        de nuestro programa: print, remove etc*/
                        
#include <unistd.h>     /*Proporciona acceso a la API del sistema operativo*/

#include <string.h>     /*Contiene definiciones de macros, constantes y declaraciones de funciones y tipos de string
                        permite manejo de cadenas y de memoria*/
                        
#include <stdlib.h>     /*Libreria estandar. Incluye funciones relacionadas con la asignación de memoria, 
                        control de procesos, conversiones y otras*/
                        
#include <sys/wait.h>   /*Informa el estado de procesos*/

typedef int bool;       /*Crea un sinónimo (o alias), bool representa el tipo int*/

#define true 1                      /*Se define 1 como verdadero*/
#define false 0                     /*Se define 0 como falso*/


#define LSH_RL_BUFSIZE 1024         /*Tamaño de búfer para leer la entrada del usuario*/
#define LSH_TOK_BUFSIZE 64          /*Tamaño de búfer para dividir los argumentos*/
#define LSH_HIST_SIZE 10            /*Tamaño de búfer para el historial de comandos */
#define LSH_TOK_DELIM " \t\r\n\a"   /*Delimitadores para analizar los argumentos */



/*Variable global para verificar la concurrencia de procesos padre e hijo */
bool conc = false;

/*Variable global para apuntar al último comando ejecutado*/
int cur_pos = -1;

/*Variable global que almacena el historial de comandos ejecutados*/
char *history[LSH_HIST_SIZE];
/*Tamaño actual del buffer*/
int cur_bufsize = LSH_TOK_BUFSIZE;

/*Declaraciones de funciones para comandos de shell incorporados*/
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);

/*Lista de comandos integrados, seguidos de sus funciones correspondientes*/
char *builtin_str[] = {
    "cd",
    "help",
    "exit"
};

int (*builtin_func[]) (char **) = {
    &lsh_cd,
    &lsh_help,
    &lsh_exit
};

/*Retorna el # total de instrucciones*/
int lsh_num_builtins(){
    return sizeof(builtin_str) / sizeof(char *);
}

/*Implementación de cd (change directory)*/
int lsh_cd(char **args)
{
    if(args[1] == NULL){
        fprintf(stderr, "lsh: expected argument to \"cd\"\n");
    }else{
        if(chdir(args[1]) != 0){
            perror("lsh");
        }
    }

    return 1;
}

/*Implementación de help, muestra información acerca del programa*/
int lsh_help(char **args)
{
    int i;
    printf("Aman Dalmia's LSH\n");
    printf("Type program names and arguments, and press enter.\n");
    printf("Append \"&\" after the arguments for concurrency between parent-child process.\n");
    printf("The following are built in:\n");

    for(i = 0; i < lsh_num_builtins(); i++){
        printf(" %s\n", builtin_str[i]);
    }

    printf("Use the man command for information on other programs.\n");
    return 1;
}

/*Finalizar Programa*/
int lsh_exit(char **args)
{
    return 0;
}

/*Lanzar el programa*/
int lsh_launch(char **args)
{
    pid_t pid, wpid;        /*pid:process identifier*/
    int status;

    pid = fork();
    if(pid == 0){                                       /* Caso Proceso hijo*/
        if(execvp(args[0], args) == -1) perror("lsh");  /*Si execvp devuelve -1 lance error*/
        exit(EXIT_FAILURE);                             /*Finalice la ejecución con un error*/
    }else if(pid > 0){ /* Caso Proceso padre*/
        if(!conc){
            do{
                wpid =  waitpid(pid, &status, WUNTRACED);       /*Espere a que finalice el proceso*/
            }while(!WIFEXITED(status) && !WIFSIGNALED(status)); /*Comprueba si finalizo el proceso*/
        }
    }else{ /* Error realizando el fork */
        perror("lsh");
    }

    conc = false;
    return 1;
}

/*  Analiza la entrada para obtener los argumentos */
char **lsh_split_line(char *line){
    cur_bufsize = LSH_TOK_BUFSIZE;
    int position = 0;
    char **tokens = malloc(cur_bufsize * sizeof(char*));    /*Asigna memoria dinamicamente*/
    char *token;

    if(!tokens){        /*Si no existen tokens envie un error*/
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, LSH_TOK_DELIM);    /*Devuelve un puntero al token*/
    while(token != NULL){
        tokens[position] = token;       /*Almacena el token*/
        position++;

        if(position >= cur_bufsize){
            cur_bufsize += LSH_TOK_BUFSIZE; 
            tokens = realloc(tokens, cur_bufsize * sizeof(char*)); /*Cambia el tamaño de un bloque de memoria que se asignó previamente*/
            if(!tokens){
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, LSH_TOK_DELIM);    /*Se establece token como nulo*/
    }
    if(position > 0 && strcmp(tokens[position - 1], "&") == 0) {    /*Caso ejecutarse al mismo tiempo*/
        conc = true;
        tokens[position - 1] = NULL;
    }
    tokens[position] = NULL;
    return tokens;
}

/* Historial de comandos */
int lsh_history(char **args)
{
    if(cur_pos == -1 || history[cur_pos] == NULL){      /* Caso no se han realizado comandos */
        fprintf(stderr, "No commands in history\n");
        exit(EXIT_FAILURE);
    }

    if(strcmp(args[0], "history") == 0){                /* Caso se han realizado comandos */
        int last_pos = 0, position = cur_pos, count = 0;

        if(cur_pos != LSH_HIST_SIZE && history[cur_pos + 1] != NULL){
            last_pos = cur_pos + 1;
        }

        count = (cur_pos - last_pos + LSH_HIST_SIZE) % LSH_HIST_SIZE + 1;
        /*imprime el historial*/
        while(count > 0){
            char *command = history[position];
            printf("%d %s\n", count, command);
            position = position - 1;
            position = (position + LSH_HIST_SIZE) % LSH_HIST_SIZE;
            count --;
        }
    }else{                  /*Caso ejecutar comando anterior*/
        char **cmd_args;
        char *command;
        if(strcmp(args[0], "!!") == 0){
            command = malloc(sizeof(history[cur_pos]));
            strcat(command, history[cur_pos]);
            printf("%s\n", command);
            cmd_args = lsh_split_line(command); /*Obtiene los argumentos*/
            int i;
            for (i = 0; i < lsh_num_builtins(); i++){
                if(strcmp(cmd_args[0], builtin_str[i]) == 0){
                    return (*builtin_func[i])(cmd_args);        /*Devuelve el comando y los argumentos ingresados previamente*/
                }
            }
            return lsh_launch(cmd_args);    /*lance el comando*/
        }else if(args[0][0] == '!'){ /*caso comando inicia por !*/
            if(args[0][1] == '\0'){/*Caso error*/
                fprintf(stderr, "Expected arguments for \"!\"\n");
                exit(EXIT_FAILURE);
            }
            /*Posicion del comando a ejecutar */
            int offset = args[0][1] - '0';
            int next_pos = (cur_pos + 1) % LSH_HIST_SIZE;
            if(next_pos != 0 && history[cur_pos + 1] != NULL){
                offset = (cur_pos + offset) % LSH_HIST_SIZE;
            }else{
                offset--;
            }
            if(history[offset] == NULL){
                fprintf(stderr, "No such command in history\n");
                exit(EXIT_FAILURE);
            }
            command = malloc(sizeof(history[cur_pos]));
            strcat(command, history[offset]);
            cmd_args = lsh_split_line(command);
            int i;
            for (i = 0; i < lsh_num_builtins(); i++){
                if(strcmp(cmd_args[0], builtin_str[i]) == 0){
                    return (*builtin_func[i])(cmd_args); /*Devuelve el comando y los argumentos ingresados previamente*/
                }
            }
            return lsh_launch(cmd_args); /*lance el comando*/
        }else{
            perror("lsh");
        }
    }
}

/* Ejecutar los argumentos analizados */
int lsh_execute(char *line){
    int i;
    //printf("%s\n", line);

    char **args = lsh_split_line(line);

    if(args[0] == NULL){ /* Caso comando vacio */
        return 1;
    }else if(strcmp(args[0], "history") == 0 ||
             strcmp(args[0], "!!") == 0 || args[0][0] == '!'){
        return lsh_history(args);       /* Caso se inserto comando anterior*/
    }

    cur_pos = (cur_pos + 1) % LSH_HIST_SIZE;
    history[cur_pos] = malloc(cur_bufsize * sizeof(char));
    char **temp_args = args;
    int count=0;
    
    /*  Se concantena en historial el nuevo comando con sus argumentos*/
    while(*temp_args != NULL){
        strcat(history[cur_pos], *temp_args);
        strcat(history[cur_pos], " ");
        temp_args++;
    }
    //printf("%s\n", history[cur_pos]);
    //history[count] = '\0';
    if(cur_pos > 0)
    printf("Inserted %s\n", history[cur_pos-1]);    /*Muestra comando insertado*/

    for (i = 0; i < lsh_num_builtins(); i++){
        if(strcmp(args[0], builtin_str[i]) == 0){
            return (*builtin_func[i])(args);    /*Devuelve el comando y los argumentos ingresados previamente*/
        }
    }

    return lsh_launch(args);
}

/* Lee la entrada de stdin */
char *lsh_read_line(void)
{
    cur_bufsize = LSH_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) * cur_bufsize);
    int c;

    if(!buffer){
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while(1){
        /* Leer un caracter */
        c = getchar();

        if(c == EOF || c == '\n'){  /*Caso vacio o fin de la entrada*/
            buffer[position] = '\0';
            return buffer;
        }else{                      /*Caso hay un caracter en la entrada*/
            buffer[position] = c;
        }
        position++;

        /* Si se excedió el búfer, reasignar el búfer */
        if(position >= cur_bufsize){
            cur_bufsize += LSH_RL_BUFSIZE;
            buffer = realloc(buffer, cur_bufsize);
            if(!buffer){
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

/* Bucle para obtener entrada y ejecutarla */
void lsh_loop(void)
{
    char *line;
    int status;

    do {
        printf(">");
        line = lsh_read_line();
        status = lsh_execute(line);
        /*Libera memoria*/
        free(line);
    } while(status);
}

int main(void)
{
    lsh_loop();

    return EXIT_SUCCESS;
}