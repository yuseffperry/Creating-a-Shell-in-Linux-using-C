#include <setjmp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>

static int quit_flag = 0;			//set from quit internal command
static char ** child_argv = NULL;	//arguments
static int child_argc = 0;			//arguments count
static int num_pipes = 0;			//count of pipes in command

//index of th PATH environment
#define ENV_PATH 0
static char * my_env[2] = {NULL, NULL};	//similar to environ(7)


static char * search_path(char * prog){
	if((prog[0] == '.') && (prog[1] == '/'))
		return strdup(prog);

	if(prog[0] == '/')
		return strdup(prog);

	if(my_env[0] == NULL)
		return NULL;

	char * env_path = strdup(my_env[0]);	//PATH from our environment

	char buf[100] = {0};

	char * save_path;
	char * path = strtok_r(env_path, ":", &save_path);
	while(path != NULL){
		snprintf(buf, sizeof(buf), "%s/%s", path, prog);
		if(access(buf, R_OK|X_OK) == 0){ //check file can be read/executed
			free(env_path);
			return strdup(buf);
		}

		path = strtok_r(NULL, ":", &save_path);
	}
	free(env_path);
	return NULL;
}

static int path_add(const char * path){
	size_t np_len;

	char * vp = my_env[0];	//variable PATH

	if(vp == NULL){
		np_len =  1 + strlen(path) + 1;	//new PATH len
		vp = (char*)malloc(sizeof(char)*np_len);
		if(vp == NULL){
			perror("malloc");
			return 1;
		}
		snprintf(vp, np_len, "%s", path);
	}else{
		size_t vp_len = strlen(vp);
		np_len = vp_len + 1 + strlen(path) + 1;
		char * temp = (char*)realloc(vp, sizeof(char)*np_len);
		if(temp == NULL){
			perror("realloc");
			return 1;
		}
		vp = temp;
		snprintf(&vp[vp_len], np_len, ":%s", path);
	}
	my_env[0] = vp;

	return 0;
}

static int path_remove(const char * path){

	char * vp = my_env[0];	//variable PATH

	if(vp == NULL)
		return 0;

	my_env[0] = NULL;

	char * token = strtok(vp, ":");
	while(token){
		if(strcmp(token, path) != 0)	//if its not the removed path
			path_add(token);
		token = strtok(NULL, ":");
	}

	return 0;
}

static int split_args(char * cmd){
	//clear variables from previous execution
	child_argc = num_pipes = 0;

	if(strlen(cmd) == 0)	//if we have empty line
		return 0;

	//tokenize command
	char * token = strtok(cmd, " \t");
	while(token){	//while we have a token
		child_argc++;	//increase argument counter

		//reallocate the array for arguments
		char ** temp = realloc(child_argv, sizeof(char*)*(child_argc+1));
		if(temp == NULL){		//if realloc failed
			perror("realloc");	//show why
			return -1;
		}
		child_argv = temp;		//set new array

		if(token[0] == '|'){	//if we have a pipe
			num_pipes++;		//save pipe index
			token = NULL;		//clear token value
		}
		child_argv[child_argc-1] = token;	//save token

		token = strtok(NULL, " \t");	//get next token
	}
	child_argv[child_argc] = NULL;	//arguments end with NULL

	return child_argc;
}

//Run an external command and process any redirection
static int my_exec(int io[2], const int close_me){
	int status,i;

	char * path = search_path(child_argv[0]);
	if(path == NULL){
		fprintf(stderr, "Error: %s is not found in PATH\n", child_argv[0]);
		return 1;
	}

	pid_t child = fork();	//create a child process
	if(child == -1){	//check the fork return value
		perror("fork");	//show why fork failed
		return -1;
	}

	if(child == 0){
		if(close_me != -1){	//if we have to close unused end
			//close unused end of pipe
			close(io[close_me]);
			io[close_me] = -1;
		}

		//check for redirection
		for(i=0; i < 2; i++){	//loop twice, since we can have 2 redirections max
			if(child_argc >= 3){	//if we have at least 3 arguments ( 1 command and 2 for redirection and filename)
				if(	child_argv[child_argc-2][0] == '<'){	//input redirection
					io[0] = open(child_argv[child_argc-1], 0, O_RDONLY);	//open file for input
					if(io[0] == -1){	//if open failed
						perror("open");	//show why
						exit(1);
					}

					child_argc -= 2;				//reduce argument count
					child_argv[child_argc] = NULL;	//update end of array
				}else if(	child_argv[child_argc-2][0] == '>'){	//output redirection
					//open/create file for writing only.
					io[1] = open(child_argv[child_argc-1], O_CREAT|O_TRUNC|O_WRONLY, 0666);
					if(io[1] == -1){
						perror("open");
						exit(1);
					}
					child_argc -= 2;
					child_argv[child_argc] = NULL;
				}

			}
		}

		//duplicate input, if we have redirection
		if(	(io[0] != -1) &&
			(dup2(io[0], STDIN_FILENO) != STDIN_FILENO)){
			perror("dup2");
			exit(1);
		}

		if(	(io[1] != -1) &&
			(dup2(io[1], STDOUT_FILENO) != STDOUT_FILENO)){
			perror("dup2");
			exit(1);
		}

		execv(path, child_argv);
		//we get here only if execvp/e fails
		perror(child_argv[0]);	//show why we failed
		exit(1);	//exit shell
	}

	free(path);

	//close pipe
	if(io[0] != -1)	close(io[0]);	//if we have an opened pipe end, close it
	if(io[1] != -1)	close(io[1]);

	//only parent gets up to here
	if(num_pipes > 0)	//if we don't have a pipe or last command in pipeline
		return 0;

	waitpid(child, &status, 0);	//wait for child to finish

	return (WEXITSTATUS(status) == 1) ? 1 : 0;
}

static int count_args(char ** argv){
	int i=0;
	while(argv[i] != NULL){ i++;};
	return i;
}

static int process_args(){

	if(child_argc == 0)	//line with no arguments
		return 0;

	if(strcmp(child_argv[0], "cd") == 0){
		//use HOME environment, if no arguments is given
		char * dirname = (child_argc == 1) ? getenv("HOME") : child_argv[1];

		if(dirname == NULL){	//if we don't have an argument and HOME is not set
			fprintf(stderr, "Error: HOME is not set");
		}else if(chdir(dirname) == -1){	//if changing directory failed
			perror("chdir");
		}

	}else if(strcmp(child_argv[0], "path") == 0){

		if(child_argc == 1){	//no arguments
			if(my_env[0] == NULL){
				puts("empty");
			}else
				printf("%s\n", my_env[0]);
		}else if(child_argc == 3){	//set command needs exactly 4 tokens, 1 for command + 3 arguments
			switch(child_argv[1][0]){
				case '+':	path_add(child_argv[2]);	break;
				case '-':	path_remove(child_argv[2]);	break;
				default:
					fprintf(stderr, "Error: path [+-] PATH\n");
					break;
			}
		}else{
			fprintf(stderr, "Error: path [+-] PATH\n");
		}
	}else if(strcmp(child_argv[0], "quit") == 0){
		quit_flag = 1;
	}else{

		int fd[2] = {-1,-1};	//pipe file descriptors

		int total_argc = child_argc;//save number of arguments
		int close_me = -1;

		int i;
		for(i=0; i <= num_pipes; i++){

			int io[2] = {-1,-1};	//cmd    io descriptors

			if(fd[1] != -1){	//if previous command had a pipe
				io[0] = fd[0];	//command input is from pipe output
				//close_me = -1;
			}

			if((i+1) <= num_pipes){	//if we have next command
				if(pipe(fd) == -1){
					perror("pipe");
					break;
				}
				io[1] = fd[1]; 		//cmd output will go to pipe input
			}

			if((num_pipes > 0) && (i == num_pipes))	//if its last command
				num_pipes = 0;

			child_argc = count_args(child_argv);	//update first command arguments count
			my_exec(io, close_me);					//run first command in pipe

			//shift arguments to front of argv
			int k,j=0;
			for(k=child_argc+1; k <= total_argc; k++,j++)
				child_argv[j]=child_argv[k];
			total_argc -= child_argc;

			//close_me = (close_me == 0) ? 1 : 0;
		}//end while
	}
	return 0;
}

int main(const int argc, char * const argv[]) {

	while (quit_flag == 0) {
		char *s = readline("$ ");
		if(s == NULL)
			break;
		//add_history(s); /* adds the line to the readline history buffer */

		if(split_args(s) > 0)
			process_args();

		free(s);		/* clean up! */
	}

	if(child_argv)	free(child_argv);
	if(my_env[0])	free(my_env[0]);

	return 0;
}
