/*
 * This code implements a simple shell program
 * It supports the internal shell command "exit", 
 * backgrounding processes with "&", input redirection
 * with "<" and output redirection with ">".
 * However, this is not complete.
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

extern char **getaline();

pid_t child_id;

/*
 * Handle exit signals from child processes
 */
void sig_handler(int signal) {
  int status;
  //int result = wait(&status);
  int result = waitpid(-1, &status, WNOHANG);

  printf("Wait returned %d\n", result);
}

/*
 * Signal handler for SIGINT
 */
void sig_handler_int(int signal){
	// We send a SIGTERM signal to the child process
	if (kill(child_id,SIGTERM) == 0){
		printf("\nProcess %d received a SIGINT signal\n",child_id);
	}else{
		printf("\n");
	}
}

/*
 * signal handler for SIGCHLD
 */
void sig_handler_child(int signal){
	/* Wait for all dead processes.
	 * We use a non-blocking call (WNOHANG) to be sure this signal handler will not
	 * block if a child was cleaned up in another part of the program. */
	/* while (waitpid(-1, NULL, WNOHANG) > 0) {
	}
	printf("\n"); */
	int status;
	while(waitpid(-1, &status, WNOHANG) > 0){
		
	}
	printf("\n");
}

void sigttou_handler(int signal){
	printf("Hit SIGTTOU\n");
}

/*
 * The main shell function
 */ 
main() {
  int i;
  char **args; 
  int result;
  int block;
  int output;
  int input;
  int pipe;
  char *output_filename;
  char *input_filename;

  pid_t shell_pid = getpid();
  
  system("stty tostop");
  
  // Set up the signal handlers
  struct sigaction child_action;
  struct sigaction sigint_action;

  child_action.sa_handler = sig_handler_child;
  sigint_action.sa_handler = sig_handler_int;
  
  //sigaction(SIGCHLD, &child_action, 0);
  sigaction(SIGINT, &sigint_action, 0);
  sigset(SIGTTOU, sigttou_handler);
  
  //put shell process as new process group leader
  setpgid(shell_pid, shell_pid);
  //get control of the terminal 
  tcsetpgrp(STDIN_FILENO, shell_pid);
  

  // Loop forever
  while(1) {

    // Print out the prompt and get the input
    printf("->");
    args = getaline();
    int i = 0;
    // No input, continue
    if(args[0] == NULL)
      continue;
	
	int semi = check_all(args);
	//printf("got here semi is %i \n", semi);
	if(semi != -1) {
		while(semi != -1){
		//result = doThis(args);
		char **beforesemi = (char**)malloc(semi*sizeof(char*));
		int c = 0;
		int argsSize = 0;
		for(c = 0; args[c] != NULL; c++) {
				argsSize++;
		}
		//printf("semi is %i and argsSize is %i \n", semi, argsSize);
		int send = argsSize-semi-1;
		char **afterSemi = (char**)malloc((argsSize-semi)*sizeof(char*));
		char *inTheWay;
		//printf("size after %i  \n", send);
		int sub= 0;
		for(c = 0; args[c] != NULL; c++) {
				if(c < semi) {
					beforesemi[c] = args[c];
					//printf("got here \n");
					//printf("beforesemi is %s \n", beforesemi[c]);
				}else if(c > semi) {
					afterSemi[sub] = args[c];
					sub++;
				}else {
					inTheWay = args[c];
				}
		}
		//printf("inTheWay is %s\n", inTheWay);
		
		int newBack = doThis(beforesemi);
		semi = check_all(afterSemi);
		if(inTheWay[0] == ';') {
			if(semi == -1) {
				doThis(afterSemi);
			}else {
				args = afterSemi;	
			}
		}else if(inTheWay[0] == '|') {
			if(newBack == -1) {
				//printf("newBack is %i\n", newBack);
				//printf("|| work \n");
				if(semi == -1) {
					doThis(afterSemi);
				}else {
					args = afterSemi;	
				}
			}
		}else if(inTheWay[0] == '&') {
			if(newBack != -1) {
				//printf("newBack is %i\n", newBack);
				//printf("&& work \n");
				if(semi == -1) {
					doThis(afterSemi);
				}else {
					args=afterSemi;
				}
			}
		}
		//int oldBack = doThis(afterSemi);
		//printf("oldBack is %i\n", oldBack);
		}
	}
   else{
    // Check for internal shell commands, such as exit
    if(internal_command(args))
      continue;


    // Check for an ampersand
    block = (ampersand(args) == 0);
    // Check for redirected input
    input = redirect_input(args, &input_filename);
	// Check for piping
	pipe = check_pipes(args);
	/*int j = 0;
	for(j; args[j] != NULL; j++) {
			printf("args after pipe %s \n", args[j]);
	}*/
    switch(input) {
    case -1:
      printf("Syntax error!\n");
      continue;
      break;
    case 0:
      break;
    case 1:
      printf("Redirecting input from: %s\n", input_filename);
      break;
    }

    // Check for redirected output
	
    output = redirect_output(args);

    switch(output) {
    case -1:
      printf("Syntax error!\n");
      continue;
      break;
    case 0:
      break;
    case 1:
      printf("Redirecting output to: %s\n", output_filename);
      break;
    }
    // Do the command
    int back = do_command(args, block, 
	       input, input_filename, 
	       output, output_filename, pipe);
  }
  }
}


int doThis(char** args) {
	int i;
  int result;
  int block;
  int output;
  int input;
  int pipe;
  char *output_filename;
  char *input_filename;
block = (ampersand(args) == 0);
    // Check for redirected input
    input = redirect_input(args, &input_filename);
	// Check for piping
	pipe = check_pipes(args);
	/*int j = 0;
	for(j; args[j] != NULL; j++) {
			printf("args after pipe %s \n", args[j]);
	}*/
	
    switch(input) {
    case -1:
      printf("Syntax error!\n");
      break;
    case 0:
      break;
    case 1:
      printf("Redirecting input from: %s\n", input_filename);
      break;
    }

    // Check for redirected output
	
    output = redirect_output(args);

	
    switch(output) {
    case -1:
      printf("Syntax error!\n");
      break;
    case 0:
      break;
    case 1:
      printf("Redirecting output to: %s\n", output_filename);
      break;
    }	
	int back = do_command(args, block, 
	       input, input_filename, 
	       output, output_filename, pipe);
}


/*
 * Check for ampersand as the last argument
 */
 int check_all(char **args) {
  int i;
/*
  for(i = 0; args[i] != NULL; i++) {
	 printf("ARGS{I} is %s \n", args[i]); 
	  
  }*/
  for(i = 0; args[i] != NULL; i++) {
	  //printf("args[i] is %s \n", args[i]);
  if(args[i][0] == ';') {
	  //printf("found it \n");
    //free(args[i-1]);
    //args[i-1] = NULL;
    return i;
  }
  else if(args[i][0] == '&') {
	  if(args[i][1] == '&'){
		  //printf("found it \n");
		  return i;
	  }
  }else if(args[i][0] == '|') {
	  if(args[i][1] == '|'){
		  //printf("found it \n");
		  return i;
	  }
  }
}
return -1;
}
 
 
int ampersand(char **args) {
  int i;

  for(i = 1; args[i] != NULL; i++) ;

  if(args[i-1][0] == '&') {
    free(args[i-1]);
    args[i-1] = NULL;
    return 1;
  } else {
    return 0;
  }
  
  return 0;
}

/* 
 * Check for internal commands
 * Returns true if there is more to do, false otherwise 
 */
int internal_command(char **args) {
  if(strcmp(args[0], "exit") == 0) {
    exit(0);
  }

  return 0;
}

void f_pipe(char * args[]){
	// File descriptors
	int pfd[2]; 
	int pfd2[2];
	char **commandsIn;
	pid_t pid;
	int err = -1;
	int breakLoop = 0;
	int i = 0;
	int j = 0;
	int k = 0;
	int runThrough = 0;
	int pipe_count = 0;
	for(runThrough; args[runThrough] != NULL; runThrough++) {
		if(args[runThrough][0] == '|') {	
			pipe_count++;
		}
	}
	pipe_count++;
	while (args[j] != NULL && breakLoop != 1){
		k = 0;
		while (args[j][0] != '|'){
			commandsIn[k] = args[j];
			j++;	
			if (args[j] == NULL){
				breakLoop = 1;
				k++;
				break;
			}
			k++;
		}
		commandsIn[k] = NULL;
		j++;
		if (i % 2 != 0){
			pipe(pfd);
		}else{
			pipe(pfd2);
		}
		pid=fork();
		if(pid==-1){			
			if (i != pipe_count - 1){
				if (i % 2 != 0){
					close(pfd[1]);
				}else{
					close(pfd2[1]);
				} 
			}			
			printf("Child process could not be created\n");
			return;
		}
		if(pid==0){
			if (i == 0){
				dup2(pfd2[1], 1);
			}
			else if (i == pipe_count - 1){
				if (pipe_count % 2 != 0){
					dup2(pfd[0],0);
				}else{
					dup2(pfd2[0],0);
				}
			}else{ // for odd i
				if (i % 2 != 0){
					dup2(pfd2[0],0); 
					dup2(pfd[1],1);
				}else{ // for even i
					dup2(pfd[0],0); 
					dup2(pfd2[1],1);					
				} 
			}
			if (execvp(commandsIn[0],commandsIn)== -runThrough){
				printf("ERROR IN PIPE COMMAND\n");
				exit(0);
			}		
		}
		if (i == 0){
			close(pfd2[1]);
		}
		else if (i == pipe_count - 1){
			if (pipe_count % 2 != 0){					
				close(pfd[0]);
			}else{					
				close(pfd2[0]);
			}
		}else{
			if (i % 2 != 0){					
				close(pfd2[0]);
				close(pfd[1]);
			}else{					
				close(pfd[0]);
				close(pfd2[1]);
			}
		}
		waitpid(pid,NULL,0);		
		i++;	
	}
}


/* 
 * Do the command
 */
int do_command(char **args, int block,
	       int input, char *input_filename,
	       int output, char *output_filename, int actpipe) {
  
  int result;
  int status;

  // Fork the child process
  /*struct sigaction action;
	action.sa_handler = &alt_sig_handler;
	sigaction(SIGTTOU, &action, NULL);
	
	*/
	
	if(actpipe == 1){
		f_pipe(args);
	}else {
	
	 // Fork the child process
		 child_id = fork();
		  

	  // Check for errors in fork()
	  switch(child_id) {
	  case EAGAIN:
		perror("Error EAGAIN: ");
		return -1;
	  case ENOMEM:
		perror("Error ENOMEM: ");
		return -2; 
	  }
	  if(!block){
		  setpgid(0, 0);
	  }
		
		if(child_id == 0) {
			//have the child process ignore SIGINT signal so the parent will handle it with sig_handler_int
			signal(SIGINT, SIG_IGN);
	
			// Set up redirection in the child process
			if(input){
			  freopen(input_filename, "r", stdin);
			}
			if(output == 1){
			  freopen(output_filename, "w+", stdout);
			}
			else if(output == 2)
				freopen(output_filename, "a+", stdout);
			// Execute the command
				result = execvp(args[0], args);
				//printf("result is %i\n", result);
				//printf("result is %i\n", result);
				if(result == -1) {
					exit(-1);	
				}
				return result;
				//return result;
			exit(-1);
		}

	  // Wait for the child process to complete, if necessary
	  if(block) {
		printf("Waiting for child, pid = %d\n", child_id);
		result = waitpid(child_id, &status, 0);
	  }else {
		  // The SIGCHILD handler will take care of the returning values of the childs.
		printf("Not waiting for child, pid = %d\n", child_id);
		result = waitpid(-1, &status, WNOHANG);
	  }
	}
}

/*
* Check for pipes
*/
int check_pipes(char **args) {
  int i;
  int ret = 0;

  for(i = 0; args[i] != NULL; i++) {

    // Look for the <
    if(args[i][0] == '|') {
      ret = 1;
    }
  }

  return ret;
}

/*
* Write
*/



/*
 * Check for input redirection
 */
int redirect_input(char **args, char **input_filename) {
  int i;
  int j;

  for(i = 0; args[i] != NULL; i++) {

    // Look for the <
    if(args[i][0] == '<') {
      free(args[i]);

      // Read the filename
      if(args[i+1] != NULL) {
	*input_filename = args[i+1];
      } else {
	return -1;
      }

      // Adjust the rest of the arguments in the array
      for(j = i; args[j-1] != NULL; j++) {
	args[j] = args[j+2];
      }

      return 1;
    }
  }

  return 0;
}

/*
 * Check for output redirection
 */
int redirect_output(char **args, char **output_filename) {
  int i;
  int j;
  int ret = 1;

  for(i = 0; args[i] != NULL; i++) {

    // Look for the >
    if(args[i][0] == '>') {
		if(args[i][1] == '>') {
			ret = 2;
		}
		
      free(args[i]);

      // Get the filename 
      if(args[i+1] != NULL) {
	*output_filename = args[i+1];
      } else {
	return -1;
      }

      // Adjust the rest of the arguments in the array
      for(j = i; args[j-1] != NULL; j++) {
	args[j] = args[j+2];
      }

      return ret;
    }
  }

  return 0;
}


