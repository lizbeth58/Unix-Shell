#include <stdio.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>

#define COMMAND_BUFFER_SIZE 1024
#define TOKEN_BUFFER_SIZE 100

/**
 * shell program with i/o redirection
 * -Elizabeth Ortiz
 */

static int parse( char* input, char* args[],int size); 			/* parse commands into cstring array */

static int needs_out_redir(char* tokens[], int size); 			/* check if output redirection is needed */

static int needs_in_redir(char* tokens[], int size);  			/* check if input redirection is needed */

static int needs_pipe(char* tokens[], int size);				/* check if pipe is needed */

static void redirect_output(char* tokens[], int output);		/* redirect the output(stdout) */

static void redirect_input(char* tokens[], int input);			/* redirect the input(stdin) */

static void left_pipe(int pipefd[]);							/* redirect program 1 output to pipe input */

static void right_pipe(int pipefd[]);							/* redirect program 2 input to pipe output */

static void create_pipe(char* tokens[],int output, int input, int isPipe);  /* create pipe from program 1 to program 2 and run*/

int main(){ 													/* shell loop */	

	char command[COMMAND_BUFFER_SIZE];
	char *tokens[TOKEN_BUFFER_SIZE];

	int token_Size;

	int pid;
	int child_pid;
	int status;

	int flag;
	int check;

	int isPipe;
	int output;
	int input;

	signal(SIGINT,SIG_IGN);

	while(1){

		printf("shell@shell:~$ ");
		
		fgets(command,COMMAND_BUFFER_SIZE,stdin);

		token_Size = 0;

		token_Size = parse(command,tokens,token_Size);

		if( strcmp(tokens[0],"exit") == 0 ){
			exit(1);
		}

		if( strcmp(tokens[0],"cd") == 0 ){
			if( (check = chdir(tokens[1])) == -1 ){
				perror("");
			}
			continue;
		}

		if( (child_pid = fork()) == -1 ){
			perror("Fork:");
			exit(1);
		}

		if(child_pid == 0){

			output =  needs_out_redir(tokens,token_Size);
			input = needs_in_redir(tokens,token_Size);
			isPipe = needs_pipe(tokens,token_Size);
			
			flag = 0;

			if( output != -1 ){
				redirect_output(tokens,output);
				tokens[output] = NULL;
				flag = 1;
			}

			if( input != -1 ){
				redirect_input(tokens,input);
				tokens[input] = NULL;
				flag = 1;
			}

			if( isPipe != -1 ){	
				create_pipe(tokens,output,input,isPipe);
			}


			if(flag || isPipe == -1){
				execvp(tokens[0],tokens);
				perror("Unkown Command:");
				exit(1);
			}

		}else{
			pid = wait(&status);
		}

	}

	return 0;
}

static int parse( char* input, char* args[],int size){
  int i = 0;

  input[strlen(input)-1] = '\0'; 

  args[i] = strtok( input, " " );

  while( args[++i] = strtok(NULL, " ") ){
  	size++;
  }

  return size;
}

static int needs_out_redir(char* tokens[], int size){

	int position;
	int i;

	position = -1;
	for(i=0; i <= size; i++){
		if(strcmp(tokens[i],">") == 0 ){
			position = i;
			break;
		}
	}

	return position;
}

static int needs_in_redir(char* tokens[], int size){

	int position;
	int i;

	position = -1;
	for(i=0; i <= size; i++){
		if(strcmp(tokens[i],"<") == 0 ){
			position = i;
			break;
		}
	}

	return position;
}

static int needs_pipe(char* tokens[], int size){

	int position;
	int i;

	position = -1;
	for(i=0; i <= size; i++){

		if(strcmp(tokens[i],"|") == 0 ){
			position = i;
			break;
		}
	}

	return position;
}

static void redirect_output(char* tokens[], int output){
	int fd;

	close(1);

	if( (fd = open(tokens[output+1],O_WRONLY|O_CREAT,0777)) == -1 ){
		perror("open:");
		exit(1);
	}
}

static void redirect_input(char* tokens[], int input){
	int fd;

	close(0);

	if( (fd = open(tokens[input+1],O_RDONLY)) == -1 ){
		perror("open");
		exit(1);
	}
}

static void left_pipe(int pipefd[]){
	int check;

	close(pipefd[0]);

	if( ( check = dup2(pipefd[1],1)) == -1 ){
		perror("dup:");
		exit(1);
	}

	close(pipefd[1]);
}

static void right_pipe(int pipefd[]){
	int check;

	close(pipefd[1]);

	if( (check = dup2(pipefd[0],0)) == -1 ){
		perror("dup");
		exit(1);
	}

	close(pipefd[0]);
}

static void create_pipe(char* tokens[],int output, int input, int isPipe){
	int pipefd[2];
	int child_pid;

	if( (pipe(pipefd)) == -1 ){
		perror("pipe:");
		exit(1);
	}

	if(	(child_pid = fork()) == -1 ){
		perror("fork:");
		exit(1);
	}

	if(child_pid == 0){

		left_pipe(pipefd);

		if(output != -1 && output <= isPipe){
			fprintf(stderr,"cannot redirect output\n");
			exit(1);
		}

		if(input != -1 && input <= isPipe){
			redirect_input(tokens,input);
			tokens[input] = NULL;
		}

		tokens[isPipe] = NULL;

		execvp(tokens[0],tokens);
		perror("execvpG");
		exit(1);

	}else{

		right_pipe(pipefd);

		if(input != -1 && input >= isPipe){
			fprintf(stderr,"cannot redirect input\n");
			exit(1);
		}

		if(output != -1 && output >= isPipe){
			redirect_output(tokens,output);
			tokens[output] = NULL;
		}

		execvp(tokens[isPipe+1],&tokens[isPipe]+1);
		perror("execvpC:");
		exit(1);
	}
}
















