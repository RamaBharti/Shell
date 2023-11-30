#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h> // For pid_t
#include <sys/wait.h>  // For waitpid
#include <signal.h>
#include <stdbool.h>

int MAX = 128;
char **cmd_history;
int count=0, j=0;

#define clear() printf("\033[H\033[J")
#define MAX_SIZE 2048
#define MAX_ARGS 64


void update_history(char* cmd){
    count++;
    if(count>5) count=5;

    int j;
    for(j=0; j<5; j++){
        if(cmd_history[j]==NULL){
            cmd_history[j]=malloc(MAX*sizeof(char));
            strncpy(cmd_history[j], cmd, MAX);
            break;
        }
    }
    if(j==5){
        free(cmd_history[0]);
        cmd_history[0] = malloc(MAX*sizeof(char));
        strncpy(cmd_history[0], cmd_history[1], MAX);
        strncpy(cmd_history[1], cmd_history[2], MAX);
        strncpy(cmd_history[2], cmd_history[3], MAX);
        strncpy(cmd_history[3], cmd_history[4], MAX);
        strncpy(cmd_history[4], cmd, MAX);
    }
} 

int status_history[1000];

void add_status_history(int new){
    status_history[j]=new;
    j++;
}

int str_parser_pipe(char* s, char** s_piped){
    int i;
    for(i=0; i<2; i++){
        s_piped[i]=strsep(&s, "|");
        if(s_piped[i]==NULL){
            break;
        }
    }
    if(s_piped[1]==NULL) return 0;
    else return 1;
}

void str_parser(char* s, char** parsed){
    for(int i=0; i<MAX_SIZE; i++){
        parsed[i]=strsep(&s, " ");
        if(parsed[i]==NULL){
            break;
        }
        if(strlen(parsed[i])==0){
            i--;
        }
    }
}

void exit_handler(){
    free(getcwd(NULL, 0));
    printf("\n");
    exit(1);
}

void change_directory(char **args){
    if(args[1]==NULL){
        printf("cd: Missing argument\n");
    }else{
        if(chdir(args[1])!=0){
            perror("cd");
        }
    }
}
// void change_directory(char **args) {
//     if (args[1] == NULL) {
//         printf("cd: Missing argument\n");
//     } else {
//         char *home_directory = getenv("HOME");
//         if (home_directory == NULL) {
//             perror("getenv");
//             return;
//         }
        
//         char target_directory[MAX_SIZE];
//         snprintf(target_directory, MAX_SIZE, "%s/%s", home_directory, args[1]);

//         if (chdir(target_directory) != 0) {
//             perror("cd");
//         }
//     }
// }

void temp_func(bool b, int b2){
    int pid=fork();
    if(pid<0){
        printf("Forking failed\n");
        return;
    }
    else if(pid>0){
        add_status_history(pid);
        if(!b){
            waitpid(pid, NULL, 0);
        }
    }
    if(pid==0){
        int x=10;
        for(int i=0; i<x; i++){
            sleep(i);
            printf("%d\n", i);
        }
        exit(0);
    }
}

void history(bool b, int b2){
    if(b2>0){
        fflush(stdout);
        for(int j=count-1; j>=0; j--){
            if(cmd_history[j]!=NULL){
                printf("%s\n", cmd_history[j]);
            }
            else break;
        }
        fflush(stdout);
        return;
    }
    int pid=fork();
    if(pid<0){
        printf("Forking failed\n");
        return;
    }
    else if(pid>0){
        add_status_history(pid);
        if(!b) waitpid(pid, NULL, 0);
    }
    else if(pid==0){
        fflush(stdout);
        for(int j=count-1; j>=0; j--){
            if(cmd_history[j]!=NULL){
                printf("%s\n", cmd_history[j]);
            }
            else break;
        }
        fflush(stdout);
        exit(0);
    }
}

void ps_history(bool b1, bool b2){
	pid_t pid;

	pid=fork();

	if (pid < 0) {
		printf("\nError forking child");
	} else if (pid == 0){
		int i=0;
		while (i<j){
			int process_id = status_history[i];
			int status;
			printf("%d ", process_id);
			pid_t return_pid = kill(process_id, 0);
			if (return_pid == 0) {
				printf("%s\n", "RUNNING");
			} else {
				printf("%s\n", "STOPPED");
			}
			i++;
		}
		exit(0);
	} else {
		add_status_history(pid);
		wait(NULL);
	}
}

void custom_echo(char **args, bool b, bool b2){
	pid_t pid;

	pid=fork();

	if (pid < 0) {
		printf("\nError forking child");
	} else if (pid == 0){
		int i=1;
		while (args[i]){
			if (args[i][0]=='$'){
				char *temp=args[i]+1;
				char *value = getenv(temp);
				printf("%s ", value);
			}else{
				printf("%s ", args[i]);
			}
			i++;
		}
		printf("\n");
		exit(0);
	}else {
		add_status_history(pid);
	}
} 

int CustomCmd(char** parsed, bool b, int from_pipe)
{
    int numCmd = 5, i, x = -1;
    char* cmd[numCmd];

    cmd[0] = "exit";
    cmd[1] = "history";
    cmd[2] = "ps_history";
    cmd[3] = "temp_func";
    cmd[4] = "echo";
  
    for (i = 0; i < numCmd; i++) {
        if (strcmp(parsed[0], cmd[i]) == 0) {
            x = i;
            break;
        }
    }
  
    if(x==0){
        printf("\n");
        exit(0);
    }
    else if (x==1){
        history(b, from_pipe);
        return 1;
    }
    else if (x==2){
        ps_history(b, from_pipe);
        return 1;
    }
    else if (x==3){
        temp_func(b, from_pipe);
        return 1;
    }
    else if (x==4){
        custom_echo(parsed, b, from_pipe);
        return 1;
    }else if(strcmp(parsed[0], "cd")==0){
        change_directory(parsed);
        return 1;
    }
    return 0;
}

void exec(char** parsed, bool b)
{
    int pid = fork(); 

    if (pid == -1) {
        printf("Could not fork the child\n");
        return;
    } 
    else if (pid == 0) {
        if (execvp(parsed[0], parsed) < 0) {
            printf("Failed to execute command\n");
            exit(1);
        }
        exit(0);
    } 
    else if(pid>0){
        add_status_history(pid);
        if(!b)waitpid(pid, NULL, 0);
    }
}

void execPipe(char ** argv1, char ** argv2) {
    int fds[2], r, status;
    pipe(fds);
    int child1= fork();
    if (child1 == -1) {
        printf("Could not fork the child\n");
        return;
    }
    if(child1==0){
        dup2(fds[1], STDOUT_FILENO);
        close(fds[0]);
        close(fds[1]);
        if(CustomCmd(argv1, 0, 1)==0){
            if (execvp(argv1[0], argv1) < 0) {
                printf("Failed to execute command\n");
            }
        }
        exit(0);
    }
    if(child1<=0){
        exit(0);
    }
    add_status_history(child1);
    int child2= fork();
    if (child2 == -1) {
        printf("Could not fork the child\n");
        return;
    }
    if(child2==0){
        dup2(fds[0], STDIN_FILENO);
        close(fds[0]);
        close(fds[1]);
        if(CustomCmd(argv2, 0, 2)==0) {
            if (execvp(argv2[0], argv2) < 0){
                printf("Failed to execute command\n");
            }
        }
        exit(0);
    }
    if(child2<=0)exit(0);
    add_status_history(child2);

    close(fds[0]);
    close(fds[1]);
    
    r = waitpid(child1, &status, 0);
    r = waitpid(child2, &status, 0);
}

int processString(char* str, char** parsed, char** parsedpipe, bool b)
{
  
    char* s_piped[2];
    int is_piped = 0;
    
    is_piped = str_parser_pipe(str, s_piped);
    if (is_piped) {
        str_parser(s_piped[0], parsed);
        str_parser(s_piped[1], parsedpipe);
  
    } else {
        str_parser(str, parsed);
        if (CustomCmd(parsed, b, 0)!=0){
            return 0;
        }
    }
    return 1+is_piped; 
}

char *read_line(void)
{
  int bufsize = MAX_SIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "Allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    c = getchar();

    if (c == EOF || c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;

    if (position >= bufsize) {
      bufsize += MAX_SIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "Allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}


void loop(void)
{
  char *line;
  char *args[MAX_SIZE];
  char *parsedArgsPiped[MAX_SIZE];
  int status;

  while(1) {
    signal(SIGINT, exit_handler);
    char cwd[1000];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
      printf("\nMTL458 >");
    } else {
      perror("getcwd() error");
    }
    line = read_line();
    if(strcmp(line, "") ==0){
        continue;
	}
    int n=0;
    while (line[n]){
        n++;
    }
    char*line2=malloc(strlen(line) + 1);
    strcpy(line2, line);

    int y=1;
    int i;
    for (i=0;i<n;i++){
        if (line[i]=='='){
            y=0;
            break;
        }
    }

    if (y==0){
        char name[i+1];
        memcpy(name, &line[0], i);
        name[i]='\0';
        char value[n-i-1];
        memcpy(value, &line[i+1], n-i-1);
        setenv(name, value, 1);
        update_history(line2);
        free(line);
        continue;
    }
    
    bool b=false;
    if(line[0]=='&'){
        line=line+1;
        b=true;
    }
    status= processString(line, args, parsedArgsPiped, b);

    if (status == 1){
        exec(args, b);
    }

    if (status == 2){
        execPipe(args, parsedArgsPiped);
    }
    if(b)line=line-1;
    update_history(line2);
    free(line);
  } 
}


int main(int argc, char **argv)
{
    cmd_history = (char**) malloc(5 * sizeof(char*));
    loop();
    return EXIT_SUCCESS;
}


//for parsing help taken from geeksforgeeks.com
//took help from online resources for some other concepts as well