    #include<stdio.h>
    #include<stdlib.h>
    #include<stdbool.h>  // Needed for true/false
    #include"helpers.h" //for parse() that is given.
    #include<string.h> //for strcmp
    #define BUFFER_SIZE 1024
    #include <unistd.h>
    #include <limits.h>  // for PATH_MAX
    #include <sys/types.h>
    #include <errno.h>
    #include <fcntl.h> // for open();

    #define MAX_BG_PROCESSES 100
    pid_t bg_processes[MAX_BG_PROCESSES];
    int bg_count =0;

    void pwd();
    void cd(const char* path);
    void help();
    void execute(char **tokens);
    char * resolve_path(const char* command);
    void handle_pipes(char **tokens);

    int main(int argc, char* argv[]){
    // char *line = NULL;// pointer to hold input
    // size_t len =0; //Buffer size
    // ssize_t read; // Number of character read
    puts("Welcome to my shell!");
    char line[BUFFER_SIZE]; 

    while(true){
    printf("prompt >");
    //read user input
    // read = getline(&line,&len,stdin);

    // if(read ==-1){
    //     printf("error couldn't get line.");
    //     break;
    // }

    if(fgets(line,BUFFER_SIZE,stdin)== NULL){
        printf("Error : coudln't get the line \n");
    }



    // printf("You entered : %s",line);

    char ** tokens = parse(line," \n");
    if(tokens == NULL){
        continue; //nothing is entered
    }

    if(tokens[0] != NULL){
        if(strcmp(tokens[0],"exit")==0){
            free(tokens);
            break;
        }
    else if(strcmp(tokens[0],"wait")==0){
        while(wait(NULL)>0);
    }

    else if(strcmp(tokens[0],"help")==0){
        help();
    }
    else if(strcmp(tokens[0],"pwd")==0){
        pwd();
    }
    else if(strcmp(tokens[0],"cd")==0){
        if(tokens[1]!=NULL){
            cd(tokens[1]);
        }
        else{
            printf("Error: no directory specified. \n");
        }
    }
    // else if(strcmp(tokens[0],"wait")==0){
    //     wait();
    // }
    else{
        // execute(tokens);
        // printf("shell: %s: command not found\n",tokens[0]);
        bool has_pipe = false;
        for(int i=0;tokens[i]!=NULL; i++){
            if(strcmp(tokens[i],"|")==0){
                has_pipe = true;
                break;
            }
        }

        if(has_pipe){
            handle_pipes(tokens);
        }else{
            execute(tokens);
        }
    }
    free(tokens);
    }
    }


    return 0;
    }

    void help(){
        printf("Available built-in commands:\n");
        printf("  help- Display this help message\n");
        printf("  exit- Exit the shell\n");
        printf("  pwd- Print the current working directory\n");
        printf("  cd- Change the current working directory\n");
        printf("  wait- Wait for background processes to finish\n");
    }

    void pwd(){
    char cwd[PATH_MAX];

    if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("Current working directory: %s\n", cwd);
        } else {
            perror("getcwd() error");
        }

        return;
    }

    void cd(const char* path){
        if(chdir(path)==0){
            printf("Directory changed to : %s\n",path);
        }else{
            perror("chdir() error");
        }
    }
    //try cd C:\Users\zhiga\Documents working directory 
    char * resolve_path(const char* command){

        // printf("resolve_path called for command : %s\n",command);
            char *path_env = getenv("PATH");
            if(path_env == NULL){
                printf(":Path variable not found");
                return NULL;
            }
            
            // printf("PATH = %s\n",path_env);


            char*path_copy = strdup(path_env); // duplicate path to aovid modifyting getenv
            char *token =strtok(path_copy, ":");

            while(token != NULL){
                char full_path[BUFFER_SIZE];
                snprintf(full_path,sizeof(full_path),"%s/%s",token,command);
                // printf("DEBUG: Checking path: %s\n", full_path);  

                if(access(full_path,X_OK)==0){
                    free(path_copy);
                    return strdup(full_path); //return the found full path.
                }
                token = strtok(NULL,":");
            }

            free(path_copy);
            printf("COmmand not found in path \n");
            return NULL; //command not found. 

    }

    void handle_pipes(char **tokens){
        int num_pipes =0;

        //count the number of pipes 
        
        for(int i=0;tokens[i]!=NULL;i++){
            if(strcmp(tokens[i],"|")==0){
                num_pipes++;
            }
        }
        //if no pipe, execute normally
            if(num_pipes ==0){
                execute(tokens);
                return;
            }

        int pipes[num_pipes][2];

        for(int i=0; i<num_pipes; i++){
            if(pipe(pipes[i])==-1){
                perror("Pipe Fauled");
                exit(1);
            }
        }

        int cmd_index=0;
        int pipecount =0;

        while(tokens[cmd_index]!=NULL){
            char * command_tokens[BUFFER_SIZE];
            int cmd_size =0;
            


            while(tokens[cmd_index]!= NULL && strcmp(tokens[cmd_index],"|")!=0){
                    command_tokens[cmd_size++]= tokens[cmd_index++];
            }
            command_tokens[cmd_size] = NULL;


            pid_t pid = fork();
            if(pid==0){//child process
                if(pipecount>0){
                    dup2(pipes[pipecount-1][0],STDIN_FILENO);
                }
                if(pipecount < num_pipes){
                    dup2(pipes[pipecount][1],STDOUT_FILENO);
                }

                for(int i=0; i<num_pipes;i++){
                    close(pipes[i][0]);
                    close(pipes[i][1]);
                }

                execute(command_tokens);
                exit(1);
            }

            pipecount++;
            cmd_index++;
        }
        
        for(int i=0; i<num_pipes;i++){
            close(pipes[i][0]);
            close(pipes[i][1]);
        }

        for(int i=0; i<=num_pipes; i++){
            wait(NULL);
        }

    }



    void execute(char **tokens){
        bool background = false;
        int i=0;

        while(tokens[i]!= NULL){
            if(strcmp(tokens[i],"&")== 0){
                background = true;
                //remove & 
                tokens[i]= NULL;
                break;
            }
            i++;
        }




        pid_t pid = fork();

        if(pid<0){
            perror("Fork failed");
            return;
        }

        if(pid ==0){// Fork did not fail, child process.

            int input_fd =-1;
            int output_fd =-1;
            char *command_path = tokens[0];

            char *clean_tokens[BUFFER_SIZE];
            int clean_index = 0;

        //check for input/output redirection
        for(int i=0;tokens[i]!=NULL; i++){
            //input redirection
            //< filename - reads input from a file instead of the keyboard
            if(strcmp(tokens[i],"<")==0){
            //check if < was found...
            //open file for reading, returns a file descriptor(fd)
            //tokens[i+1] is the file name after > or < in the command 
            input_fd = open(tokens[i+1],O_RDONLY);
            if(input_fd == -1){
                perror("Error opening the input file");
                exit(1);
            }
            dup2(input_fd,STDIN_FILENO); //Redirect stdin to file
            close(input_fd);
            i++;
            // tokens[i] =NULL; //Removes < and file name from tokens.
            // tokens[i+1] = NULL;
            }
        //output redirection
        //>fileanme writes output to a file instead of the screen.
        else if(strcmp(tokens[i],">")==0){
            output_fd = open(tokens[i + 1],O_WRONLY | O_CREAT| O_TRUNC,0644);
            if(output_fd==-1){
                perror("Error opening the output file");
                exit(1);
                //Redirect stdout to file.
            }
            dup2(output_fd,STDOUT_FILENO);
            close(output_fd);
            //remove > and file name from taokens.
            // tokens[i]=NULL;
            // tokens[i+1]=NULL;
            i++; //skip the file name
        }
        else{
            clean_tokens[clean_index++] = tokens[i];
        }

    }
    //ensure clean tokens is terminated
    clean_tokens[clean_index] = NULL;

    if(input_fd != -1){
        close(input_fd);
    }
    if(output_fd!=-1){
        close(output_fd);
    }

       

        if(command_path[0] != '/'&& command_path[0]!= '.'){
            char* resolved_path = resolve_path(command_path);
            if(resolved_path != NULL){
                command_path = resolved_path;
            }
        }

        // printf("Excuting command : %s\n",command_path);


        if(execv(command_path,clean_tokens)==-1){
            perror("Error cannot execute command");
            exit(127); //standard exit code for command not found.
        }

        if(command_path != tokens[0]){
            free(command_path);
        }
        }
        else{// Parent process
           if(!background){
            int status;
            waitpid(pid,&status,0);
           }
           else{
            printf("Background processes started with PID %d \n",pid);
           }
        }

    }
