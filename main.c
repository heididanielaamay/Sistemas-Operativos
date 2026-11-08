

#include <stddef.h>			/* NULL */
#include <stdio.h>			/* setbuf, printf */
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>                      /*Para open, close ...*/
#include <sys/wait.h>
#include <sys/stat.h> // Añado esta librería para umask
#include <sys/resource.h> //Añado esta libreria para limit
#include <string.h>


extern int obtain_order();		/* See parser.y for description */

int main(void)
{
	char ***argvv = NULL;
	int argvc;
	/*char **argv = NULL;*/
	/*int argc;*/
	char *filev[3] = { NULL, NULL, NULL };
	int bg;
	int ret;
	setbuf(stdout, NULL);			/* Unbuffered */
	setbuf(stdin, NULL);
        //Ignorar las señales  SIGIT Y SIGQUIT para que no muere el minishell
        struct sigaction act;
        act.sa_handler = SIG_IGN;
        act.sa_flags = 0;
        sigemptyset(&act.sa_mask);//Creación de  la máscara para evitar interferencias entre señales
        sigaction(SIGINT,&act,NULL);
        sigaction(SIGQUIT,&act,NULL);




	while (1) {
      		fprintf(stderr, "%s", "msh> ");	/* Prompt */
		ret = obtain_order(&argvv, filev, &bg);
		if (ret == 0) break;		/* EOF */
		if (ret == -1) continue;	/* Syntax error */
		argvc = ret - 1;		/* Line */
		if (argvc == 0) continue;	/* Empty line */
      
            //Implementación de los pipes
                int i;
                int fd[2];
                int fd_in =0;
                pid_t pid2;
                pid_t ult_pid = -1;

                //Comprobación de un mandato interno
                if(argvc == 1 && !bg){
                int fd_mat[3]; // Guardar los descritores  stdin,stdout y stderr originales
                int interno = 1; //Suponemos que es interno 
                fd_mat[0]= dup(0);
                fd_mat[1]= dup(1);
                fd_mat[2]= dup(2);
                //Aplicación de las redirecciones para los mandatos internos
                 if (filev[0]){ //Redireccion de entrada 
                   int fd0 = open(filev[0],O_RDONLY);
                   if(fd0 < 0){
                    perror("open");
                    interno = 0;   
                   }else{
                    dup2(fd0,0);
                    close(fd0);
                   }
                  }
                  if( filev[1]){//Redireccion de salida 
                   int fd1 = creat(filev[1],0666);
                   if(fd1 < 0){
                    perror("creat");
                    interno = 0;
                   }else{
                    dup2(fd1,1);
                    close(fd1);
                   }
                  }
                  if( filev[2]){//Redireccion de error
                    int fd2 = creat(filev[2],0666);
                    if(fd2 < 0){
                     perror("creat");
                     interno = 0;
                    }else{
                     dup2(fd2,2);
                     close(fd2);
                    }
                  }

                  //Implementación de cada uno de los mandatos internos
                  if(interno && strcmp(argvv[0][0],"cd")==0){ // Utilizo la función strcmp para comparar cadenas

                    if(argvv[0][1]==NULL){//si cd no tiene argumentos
                     char *valor = getenv("HOME");
                     if(!valor || chdir(valor) < 0) 
                      perror("cd");

                    }else if(argvv[0][2]!=NULL){//Cuando tenga demasiados argumentos
                      fprintf(stderr,"cd  tiene demasiados argumentos\n");

                    }else{//Se produce  error en cd
                      if(chdir(argvv[0][1]) < 0) 
                       perror("cd");
                    }

                    char alm_ruta[1024];
                    if(getcwd(alm_ruta,sizeof(alm_ruta))) printf("%s\n",alm_ruta);

                  }else if(interno && strcmp(argvv[0][0], "umask") == 0){

                    if(argvv[0][1]==NULL){// si umask no tiene  argumentos
                     mode_t m= umask(0);//devuelve la mascara actual
                     umask(m);
                     printf("%o\n",m);

                    }else if(argvv[0][2]!=NULL){
                      fprintf(stderr,"umask  tiene argumentos erróneos\n");

                    }else{
                     char*fin;
                     mode_t m = strtol(argvv[0][1],&fin,8);//Covierte la parte inicial de una cadena en un número largo
                     if(*fin != '\0'){
                      fprintf(stderr, "umask presenta argumento invalido\n");
                     }else{
                      umask(m);//modifica la máscar si no hay ningún error
                     }
                    }


                  }else{
                   interno = 0;
                  }
                  //Restauramos los descriptores y  los cerramos
                  dup2(fd_mat[0],0);
                  dup2(fd_mat[1],1);
                  dup2(fd_mat[2],2);
                  close(fd_mat[0]);
                  close(fd_mat[1]);
                  close(fd_mat[2]); 

                  if(interno){
                   continue;
                  }

                }
                if(strcmp(argvv[0][0], "limit") == 0){

                  int fd_mat[3];
                  fd_mat[0] = dup(0);
                  fd_mat[1] = dup(1);
                  fd_mat[2] = dup(2);

                  if (filev[0]){ //Redireccion de entrada
                   int fd0 = open(filev[0],O_RDONLY);
                   if(fd0 < 0){
                    perror("open");
                   }else{
                    dup2(fd0,0);
                    close(fd0);
                   }
                  }
                  if( filev[1]){//Redireccion de salida
                   int fd1 = creat(filev[1],0666);
                   if(fd1 < 0){
                    perror("creat");
                   }else{
                    dup2(fd1,1);
                    close(fd1);
                   }
                  }
                  if( filev[2]){//Redireccion de error
                    int fd2 = creat(filev[2],0666);
                    if(fd2 < 0){
                     perror("creat");
                    }else{
                     dup2(fd2,2);
                     close(fd2);
                    }
                  }
                  //Ten en cuenta: RLIMIT_NOFILE =  nº máximo de descriptores abiertos
                   struct rlimit rl;
                   if (argvv[0][1] == NULL) {
                    getrlimit(RLIMIT_NOFILE, &rl);
                    printf("nofile %ld\n", rl.rlim_cur);

                   }else if(argvv[0][2] == NULL){
                     if (strcmp(argvv[0][1], "nofile") != 0) {
                       fprintf(stderr, "limit: recurso invalido\n");
                     }else{
                      getrlimit(RLIMIT_NOFILE, &rl);
                      printf("%ld\n", rl.rlim_cur);
                     }

                   }else if(argvv[0][3] != NULL){
                     fprintf(stderr, "limit: numero incorrecto de argumentos\n");
                   }else{
                     if (strcmp(argvv[0][1], "nofile") != 0) {
                      fprintf(stderr, "limit: recurso invalido\n");
                     }else{
                      long v = atol(argvv[0][2]);
                      getrlimit(RLIMIT_NOFILE, &rl);
                      rl.rlim_cur = v;
                      setrlimit(RLIMIT_NOFILE, &rl);
                     }
                   }

                   dup2(fd_mat[0],0);
                   dup2(fd_mat[1],1);
                   dup2(fd_mat[2],2);
                   close(fd_mat[0]);
                   close(fd_mat[1]);
                   close(fd_mat[2]);
                   continue;
                  }

                  if( strcmp(argvv[0][0], "set") == 0) {

                   int fd_mat[3];
                   fd_mat[0] = dup(0);
                   fd_mat[1] = dup(1);
                   fd_mat[2] = dup(2);

                   if (filev[0]){ //Redireccion de entrada
                    int fd0 = open(filev[0],O_RDONLY);
                    if(fd0 < 0){
                    perror("open");
                   }else{
                    dup2(fd0,0);
                    close(fd0);
                    }
                   }
                   if( filev[1]){//Redireccion de salida
                   int fd1 = creat(filev[1],0666);
                   if(fd1 < 0){
                    perror("creat");
                   }else{
                    dup2(fd1,1);
                    close(fd1);
                    }
                   }
                   if( filev[2]){//Redireccion de error
                    int fd2 = creat(filev[2],0666);
                    if(fd2 < 0){
                     perror("creat");
                    }else{
                     dup2(fd2,2);
                     close(fd2);
                    }
                  }
                   //variable que contiene todas las variables de entorno.
                   extern char **environ;

                   if (argvv[0][1] == NULL) {// Si  argumentos
                    for (char **e = environ; *e; e++)
                     printf("%s\n", *e);

                   }else if(argvv[0][2] == NULL){

                    char *v = getenv(argvv[0][1]);
                    if (v)
                      printf("%s=%s\n", argvv[0][1], v);

                   }else{
                    int len = 0;
                    for (int i = 2; argvv[0][i]; i++)
                     len += strlen(argvv[0][i]) + 1;//Suma la longitud de cada palabra + 1 por el espacio 

                    char *buf = malloc(strlen(argvv[0][1]) + len + 2);//Reservo memoria para copiar la cadena
                    if (buf) {
                     sprintf(buf, "%s=", argvv[0][1]);
                     for (int i = 2; argvv[0][i]; i++) {
                      strcat(buf, argvv[0][i]);
                      if (argvv[0][i+1]) strcat(buf, " ");
                     }
                     putenv(buf);
                    }
                   }
                   dup2(fd_mat[0],0);
                   dup2(fd_mat[1],1);
                   dup2(fd_mat[2],2);
                   close(fd_mat[0]);
                   close(fd_mat[1]);
                   close(fd_mat[2]);
                   continue;

                  }

               //Me creo un bucle para poder  crearme un proceso para cada comando.
                for (i = 0; i<argvc; i++){
                 int tiene_pipes = (i<argvc - 1);// Es un booleano que me ayuda a saber si tiene un pipe(1)o no tiene(0)
                 if(tiene_pipes){
                  if(pipe(fd)){
                   perror("pipe");
                   exit(1);
                  }
                 }
                 pid2= fork();//Hijo 
                 if(pid2 == 0){
                  struct sigaction ta;
                  ta.sa_flags = 0;
                  sigemptyset(&ta.sa_mask);
                  //Averiguo si el hjo fue lanazdo en blakground o no 
                  if(bg){
                   ta.sa_handler = SIG_IGN;
                  }else{
                    ta.sa_handler = SIG_DFL;
                  }
                   sigaction(SIGINT,&ta,NULL);
                   sigaction(SIGQUIT,&ta,NULL);

                  if(fd_in != 0){
                   dup2(fd_in,0);
                   close(fd_in);
                  }
                  //En el caso de que no fuera el último comando 
                  if(tiene_pipes){
                   dup2(fd[1],1);
                  }

                  if(i == 0 && filev[0]){ //Redireccion de entrada solo se puede hacer en el primer comando
                   int fd0 = open(filev[0],O_RDONLY);
                   if(fd0 < 0){
                    perror("open");
                    exit(1);
                   }
                   dup2(fd0,0);
                   close(fd0);
                  }
                  if(i == argvc -1 && filev[1]){//Redireccion de salida solo se puede hacer con el último comando
                   int fd1 = creat(filev[1],0666);
                   if(fd1 < 0){
                    perror("creat");
                    exit(1);
                   }
                   dup2(fd1,1);
                   close(fd1);
                  }
                  if( i == argvc -1 && filev[2]){//Redireccion de error solo se puede hacer con el útimo comando 
                    int fd2 = creat(filev[2],0666);
                    if(fd2 < 0){
                     perror("creat");
                     exit(1);
                    }
                    dup2(fd2,2);
                    close(fd2);
                  }
                   if(tiene_pipes){
                    close(fd[0]);
                    close(fd[1]);
                   }

                   execvp(argvv[i][0], argvv[i]);
                   perror("execvp");
                   exit(1);

                } else if(pid2 > 0){//Padre
                 if (fd_in != 0)
                  close(fd_in);

                 if (tiene_pipes) {
                  close(fd[1]);
                  fd_in = fd[0];
                 }
                 if(i== argvc - 1) ult_pid = pid2; 
                }else{
                  perror("fork");
                 }
                }
                if(!bg && ult_pid > 0){
                  if(bg){
                   printf("[%d]\n",ult_pid);
                  }else{
                   waitpid(ult_pid,NULL,0);
                  }
                }
	}
	exit(0);
	return 0;
}

