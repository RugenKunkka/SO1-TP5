#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#define MAXW 1000
#define MAXCOM 100

#define TOK_BUFSIZE 64
#define TOK_DELIM " \t\r|\n\a:"

#define MAXCOMMANDLIST 4

#define REDDELIM "<>"



int contadorDeProcesosEnElBackground=0;
int contador=0;
int estado_;
int child_status;

typedef struct{
    int flag;
    pid_t pid;
}job;

job jobs[10];

int init_procesos(char **args);
void clearBackgroundProcess(int wait);

void loopShell(char** argv);
char *readCmdLine(void);
char **getLinesSeparando(char *linea);
int executeCmds(char **args);
int cantfunc();

void trimleadingandTrailing(char *s);

//funciones de cmd
int cmdCd(char **args);
int cmdEcho(char **args);
int cmdClr(char **args);
int cmdHelp(char **args);
int cmdQuit(char **args);

int executeRedirectionCmds(char* linea, char** lineas);
int contador_red(char* linea, char** lineas);

//tp 5-------------
int pipeCounter();
int executePiped(char *linea, int cantPipes);
void bg_clr(int wait);
void handlerC(int sig);
void handlerZ(int sig);

char *arrayCmds[] = {
  "cd",
  "echo",
  "clr",
  "help",
  "quit",
  "exit",
  "clear"
};

void batchFile(char **argv, int *estado);

int main(int argc, char **argv)
{

    int estado=0;

    signal (SIGINT,handlerC);
    signal (SIGTSTP,handlerZ);
    //signal (SIGQUIT,handler);

    for (int i = 0; i < argc; i++){
        printf("argv[%d] = %s\n", i, argv[i]);
    }

    FILE *fd;
    char buffer[40];
    char **args;
    //ejecuto el batchfile
    if (argc > 1){
      fd = fopen(argv[1], "r");
        if(fd == NULL){
            printf("Error en la apertura del archivo");
            exit(1);
        }
        while(!feof(fd)){
          fgets(buffer,40, fd);
          args = getLinesSeparando(buffer);
          estado = executeCmds(args);
        }
      fclose(fd);
    }

    printf("El codigo de estado es: %d \n", estado);
    loopShell(argv);
}

//ok
void loopShell(char **argv){
    char *cmdReadedLines;
    char **args;
    int estado;
    char * lineReaded;

    int redirectionQuantity= 0;
    char *redirectionLinesCmd[3];

    int cantPipes;
    char *pipeLines[3];

    char hostname[150];
    gethostname(hostname, sizeof(hostname));

    char *username = getenv("USER");

    printf("\n");
    printf("##########################################################################################\n");
    printf("###Bienvenido a MyShell. Introduzca el comando 'help' para ver los comandos disponibles###\n");
    printf("##########################################################################################\n\n");

    do {
      char cwd[1024];
      getcwd(cwd,sizeof(cwd)); 
      printf("%s@%s:%s:~$ ",username,hostname,cwd); 
      cmdReadedLines = readCmdLine();
      cantPipes = pipeCounter(strdup(cmdReadedLines),pipeLines);
      
      redirectionQuantity = contador_red(strdup(cmdReadedLines),redirectionLinesCmd);

      if(cantPipes != 0){
        estado = executePiped(lineReaded, cantPipes);
      }
      else if (redirectionQuantity){
        estado = executeRedirectionCmds(cmdReadedLines, redirectionLinesCmd);
      }
      else{
        args = getLinesSeparando(cmdReadedLines);
        estado = executeCmds(args);
      }
      cantPipes = 0;

      if(contadorDeProcesosEnElBackground){
        clearBackgroundProcess(0);
      }
    } while (estado);
    clearBackgroundProcess(1);
}

//ok
char *readCmdLine(void)
{
  int bufferSize = 2048;
  char * bufferBuildedLine = malloc(sizeof(char) * bufferSize);

  for (int i=0; ; i++){
    int characterReaded = getchar();     //frena el programa y espero un char que viene en ascii

    if (characterReaded == EOF || characterReaded == '\n') {
      bufferBuildedLine[i] = '\0';
      return bufferBuildedLine;
    } else {
      bufferBuildedLine[i] = characterReaded;
    }
  }
}


//Segmenta la linea para obtener los argumentos y paths
//ok
char **getLinesSeparando(char *linea)
{
  int bufsize = 64;
  int position = 0;

  char **tokens = malloc(bufsize * sizeof(char*));
  char *token;

  token = strtok(linea, TOK_DELIM);
  //tokens
  while (token != NULL) {
    //strtok mete espacios en blanco de manera aleatorea, esta funcion la saque de stack y resuelve el problema
    trimleadingandTrailing(token);
    tokens[position] = token;
    position++;
    token = strtok(NULL,TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

//Funcion que ejecuta los comandos ingresados
//ok
int executeCmds(char **args)
{
  int i;

  if (args[0] == NULL) {
    return 1;
  }

  for (i = 0; i < cantfunc(); i++) {
    if (strcmp(args[0], arrayCmds[i]) == 0) { //compara si el argumento ingresado coincide con alguno permitido
      switch (i){
        case 0:
          return cmdCd(args);
          break;
        case 1:
          return cmdEcho(args);
          break;
        case 2:
          return cmdClr(args);
          break;
        case 3:
          return cmdHelp(args);
          break;
        case 4:
          return cmdQuit(args);
          break;
        case 5:
          return cmdQuit(args);
          break;
        case 6:
          return cmdClr(args);
          break;
      }
    }
  }
  return init_procesos(args);
}

//strtok me generaba espacios en blanco y me rompia el programa y con esto lo pude solucionar. aguante stack overflow XD
void trimleadingandTrailing(char *s)
{
    int  i,j;

    for(i=0;s[i]==' '||s[i]=='\t';i++);

    for(j=0;s[i];i++)
    {
        s[j++]=s[i];
    }
    s[j]='\0';
    for(i=0;s[i]!='\0';i++)
    {
        if(s[i]!=' '&& s[i]!='\t'&& s[i]!='$'&& s[i]!='\n')
                j=i;
    }
    s[j+1]='\0';
}

int cantfunc() {
  return sizeof(arrayCmds) / sizeof(char *);
}

//ok
int init_procesos(char **args){
    int i=0;
    int flag_bg=0;
    pid_t pid;

    while(args[i]!=NULL){
      i++;
    }

  //Chequeo si el ultimo argumento es un & para saber si es un bg process
  if(strcmp(args[i-1],"&")==0){
    flag_bg=1;
    signal(SIGINT,SIG_IGN);
    signal(SIGTSTP,SIG_IGN);
    signal(SIGQUIT,SIG_IGN);
    args[i-1]=NULL;
  }

  pid = fork ();

  //proceso padre
  if (pid != 0) {
    //ejecuta el comando pero como no espera nada, le da play hasta el final el padre
    if(flag_bg){
        contadorDeProcesosEnElBackground++;
        printf("Parent PID: %d PID: %d\n", getppid(), pid);
    }

    else{
        estado_ = child_status;
        waitpid(pid,&child_status,0);

        if (WIFEXITED (child_status))
            printf ("El proceso hijo finalizo normalmente, con exit code %d\n",WEXITSTATUS (child_status));
        else
            printf ("El proceso hijo finalizo de forma anormal\n");
    }
  }
  else {
    char *path;
    char **paths;
    paths = getLinesSeparando(getenv("PATH"));
    int i=0;

    while(paths[i]!=NULL){
      path=strdup(paths[i]);
      strncat(path,"/",20);
      strncat(path,args[0],20);
      if(access(path, F_OK ) == 0 ) {
        execv(path,args);
        break;
      }
      i++;
    }
    fprintf (stderr, "Ocurrio un error en execv\n");
    abort ();
  }
  return 1;
}


//Funcion de Redireccion
//
int executeRedirectionCmds(char* linea, char** lineas){

  int flag = 0;
  int i=0;
  char **arg0;
  char **arg1;
  char *c = linea;
  char *comando;
  char *archivo;

  pid_t p;
  while(!flag){
    if(c[i] == '<'){
      flag = 1;
    } 
    if(c[i] == '>'){
      flag = 2;
    }
    i++;
  }

  switch(flag){
    case 1:
      comando = strtok(linea, "<");
      archivo = strtok(NULL, "\n");
      arg1 = getLinesSeparando(archivo);
      arg0 = getLinesSeparando(comando);

      p = fork();
      if (p < 0) {
          printf("\nCould not fork");
          return 1;
        }

      if (p == 0){
        int file = open(arg1[0],O_RDONLY | O_CREAT, 0777);
        if(file == -1){
          return 2;
        }
        dup2(file, STDIN_FILENO);
        close(file);

        char *path;
        char **paths;

        paths = getLinesSeparando(getenv("PATH"));
        int i=0;

        while(paths[i]!=NULL){

          path=strdup(paths[i]);
          strncat(path,"/",20);
          strncat(path,arg0[0],20);
          
          if(access(path, F_OK ) == 0 ) {
            execv(path,arg0);
            break;
          }
          i++;
        }
        fprintf (stderr, "Ocurrio un error en execv\n");

      }
      waitpid(p,NULL,WUNTRACED);
      break;
    case 2:
      comando = strtok(linea, ">"); 
      archivo = strtok(NULL, "\n"); 
      arg1 = getLinesSeparando(archivo);
      arg0 = getLinesSeparando(comando);

      p = fork();
      if (p < 0) {
          printf("\nCould not fork");
          return 1;
        }

      if (p == 0){
        int file = open(arg1[0],O_WRONLY | O_CREAT, 0777);
        if(file == -1){
          return 2;
        }
        dup2(file, STDOUT_FILENO);
        close(file);

        char *path;
        char **paths;
        paths = getLinesSeparando(getenv("PATH"));
        int i=0;

        while(paths[i]!=NULL){

          path=strdup(paths[i]);
          strncat(path,"/",20);
          strncat(path,arg0[0],20);

          if(access(path, F_OK ) == 0 ) {
            execv(path,arg0);
            break;
          }
          i++;
        }
        fprintf (stderr, "Ocurrio un error en execv\n");
      }
      waitpid(p,NULL,WUNTRACED);
      break;
    default:
      break;
  }

  return 1;
}

int contador_red(char* linea, char** lineas){
  int i=0;
  for(;;){
    lineas[i] = strsep(&linea, REDDELIM);//
    if(lineas[i] == NULL){
      break;
    }
    i++;
  }
  return i-1;
}


//---------------------------------------------------------
//funciones del cmd
//---------------------------------------------------------
//cambiar de directorio
//
int cmdCd(char **args)
{

  char cwd[1024];
  getcwd(cwd,sizeof(cwd));

  printf("argumento1: %s \n",args[0]);
  printf("argumento2: %s \n",args[1]);
  printf("argumento3: %s \n",args[2]);

  if (args[1] != NULL && strcmp(args[1],"")) {

    if ((strcmp(args[1],"-")==0)){
      args[1] = getenv("OLDPWD");
    }
    if(!chdir(args[1])){
      setenv("OLDPWD",cwd,1);
      getcwd(cwd,sizeof(cwd));
      setenv("PWD",cwd,1);
    }
    else if(chdir(args[1]) != 0){
      printf("error: el directorio <%s> no existe \n",args[1]);
    }

  }
  else {
    printf("especifica un argumento \"cd\"\n");
  }
  return 1;
}

//ok
int cmdEcho(char **args)
{
  if (args[1] == NULL) {
    printf("\nespecifica un argumento para el echo\n");
  }

  //impresion convariable de entorno $
  int j=1;
    while(args[j]!=NULL){

      if(args[j][0] == '$'){
        args[j]++;
        printf("%s ", getenv(args[j]));
      } else{
        printf("%s ",args[j]);
      }

      j++;
    }

    printf("\n");
    return 1;
}

//ok
 int cmdClr(char **args)
{
  for(int i=0; i<1000;i++){
    printf("\033[3J\033c");//es un codigo para clerear la consola
  }
  return 1;
}

//ok
int cmdHelp(char **args)
{
  int i;
  printf("Los comandos disponibles son.\n");
  for (i = 0; i < cantfunc(); i++) {
    printf("  %s\n", arrayCmds[i]);
  }
  return 1;
}

//ok
//Funcion quit
int cmdQuit(char **args)
{
  return 0;
}

void clearBackgroundProcess(int wait){
  pid_t pid;
  int   estado;

  if(wait)
    pid = waitpid(-1, &estado, 0);
  else
    pid = waitpid(-1, &estado, WNOHANG);
  if(pid>0){
    int indice=0;
    while(jobs[indice].pid != pid){
      indice++;
    }
    jobs[indice].flag=0;
    jobs[indice].pid=0;

    printf("El proceso hijo con jobid %d y pid %d finalizo\n",indice+1,pid);
    contador-=1;
  }
  return;
}


//tp5----------------------
//Funcion que devuelve la cantidad de pipes ingresados
int pipeCounter(char* linea, char** lineas){
  int i=0;
  while(1){
    lineas[i] = strsep(&linea, "|");
    if(lineas[i] == NULL){
      break;
    }
    i++;
  }
  return i-1;
}

// Funcion que ejecuta los comandos ingresados con pipe
int executePiped(char *linea, int cantPipes){
  if(cantPipes >= 3){
    printf("Cantidad maxima de pipes permitidos: 2\n");
    return 1;
  }
  pid_t p1,p2,p3;
  char *primerComando = strtok(linea, "|");
  char *comandosRestantes = strtok(NULL,"\n");

  char **args1;
  char **args2;
  char **args3;

  args1 = getLinesSeparando(primerComando);

  int pipe_fd[2];
  pipe(pipe_fd);

  if (pipe(pipe_fd) < 0) {
    printf("\nError al inicializar el pipe\n");
    return 1;
  }

  p1 = fork();
  if (p1 < 0) {
    printf("\nCould not fork");
    return 1;
  }

  if (p1 == 0) {
      dup2(pipe_fd[1], STDOUT_FILENO);
      close(pipe_fd[0]);
      close(pipe_fd[1]);

      char *path;
      char **paths;
      paths = getLinesSeparando(getenv("PATH"));
      int i=0;

          while(paths[i]!=NULL){

            path=strdup(paths[i]);
            strncat(path,"/",20);
            strncat(path,args1[0],20);

            if(access(path, F_OK ) == 0 ) {
              execv(path,args1);
              break;
            }
            i++;
          }
      fprintf (stderr, "Ocurrio un error en execv\n");
      abort ();
  }
  char *segundoComando = strtok(comandosRestantes, "|");
  comandosRestantes = strtok(NULL,"\n");
  args2 = getLinesSeparando(segundoComando);

  char *tercerComando = strtok(comandosRestantes, "|");
  comandosRestantes = strtok(NULL,"\n");
  args3 = getLinesSeparando(tercerComando);

  int pipe_fd1[2];
  if (pipe(pipe_fd1) < 0) {
    printf("\nError al inicializar el pipe\n");
    return 1;
  }

  p2 = fork();
  if (p2 < 0) {
    printf("\nCould not fork");
    return 1;
  }

  if (p2 == 0) {
    dup2(pipe_fd[0], STDIN_FILENO);
    if(cantPipes == 2){
      dup2(pipe_fd1[1], STDOUT_FILENO);
    }

    close(pipe_fd[1]);
    close(pipe_fd[0]);
    close(pipe_fd1[1]);
    close(pipe_fd1[0]);

    char *path;
    char **paths;
    paths = getLinesSeparando(getenv("PATH"));
    int i=0;

    while(paths[i]!=NULL){

      path=strdup(paths[i]);
      strncat(path,"/",20);
      strncat(path,args2[0],20);

      if(access(path, F_OK ) == 0 ) {
        execv(path,args2);
        break;
      }
      i++;
    }
    fprintf (stderr, "Ocurrio un error en execv\n");
    abort ();
  }

  if(cantPipes == 2){
    p3 = fork();
    if (p3 < 0) {
      printf("\nCould not fork");
      return 1;
    }

    if (p3 == 0) {

    dup2(pipe_fd1[0], STDIN_FILENO);
    close(pipe_fd[1]);
    close(pipe_fd[0]);
    close(pipe_fd1[1]);
    close(pipe_fd1[0]);
    char *path;
    char **paths;
    
    paths = getLinesSeparando(getenv("PATH"));
    int i=0;

    while(paths[i]!=NULL){

      path=strdup(paths[i]);
      strncat(path,"/",20);
      strncat(path,args3[0],20);

      if(access(path, F_OK ) == 0 ) {
        execv(path,args3);
        break;
      }
      i++;
    }
    fprintf (stderr, "Ocurrio un error en execv\n");
    abort ();
    }
  }

  close(pipe_fd[0]);
  close(pipe_fd[1]);
  close(pipe_fd1[0]);
  close(pipe_fd1[1]);

  waitpid(p1,NULL,WUNTRACED);
  waitpid(p2,NULL,WUNTRACED);
  waitpid(-1,NULL,WUNTRACED);
  return 1;
}

void handlerC(int sig){
  printf("\nEntro a ctrl+C \n");
  fflush(stdout)
}
void handlerZ(int sig){
  printf("\nEntro a ctrl+Z \n");
  return;
}




