#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_JOB 6 //max jobs 5 
#define MAX_ARG 80 //max number of arguments provided
#define MAX_LINE 80 //max length of a line

#define END 1
#define STOP 2

#define FOREGROUND 0
#define BACKGROUND 1
#define STOPPED 2

#define IN_REDIRECT 4
#define OUT_REDIRECT 8

char* buffer;
int SIGTSTP_CALL;
int FG_END;
int SIGCONT_CALL;
struct command_text;
struct process;

struct command_text** cmd_list;
struct process** pcs_list;

struct command_text{
    int flag;
    int input;
    int output;
    int end_of_param;
    char* in_file;
    char* out_file;
    char* command;
    char** params;
    char** argv;
};

struct process{
    int jid;
    pid_t pid;
    int flag;
};

//destructors
void* _command_text(struct command_text*);
void* _process(struct process*);

int get_pid(int jid);
int is_builtin(struct command_text*);
int exec_builtin(struct command_text*);

int _kill(pid_t);
int _bg(pid_t);
int _fg(pid_t);
int _jobs();

void parsing_command(char*, struct command_text*);
void signal_handler(int SIGNAL);

void initialize(); //initialize
void loop(); //mainloop
void clear(); //clear up
int add_command();

// functions
int main(int argc, char** argv){
    initialize();
    signal(SIGTSTP,signal_handler);
    signal(SIGCHLD,signal_handler);
    loop();
    clear();
    return END;
}

void initialize(){
    cmd_list = (struct command_text**)malloc(sizeof(struct command_text*)*MAX_JOB);
    pcs_list = (struct process**)malloc(sizeof(struct process*)*MAX_JOB);

    buffer = (char*)malloc(sizeof(char)*MAX_LINE);
    SIGTSTP_CALL = 0;
    FG_END = 0;
    SIGCONT_CALL = 0;
}

void signal_handler(int SIGNAL){
    int status,i;
    pid_t pid;
    if(SIGNAL == SIGTSTP){
        for(i=0;i<MAX_JOB;i++){
            if(pcs_list[i] && pcs_list[i]->flag == BACKGROUND){
                pcs_list[i]->flag = STOPPED;
                _bg(pcs_list[i]->pid);
            }
        }
        //Hang up foreground
        for(i=0;i<MAX_JOB;i++){
            if(pcs_list[i] && pcs_list[i]->flag == FOREGROUND){
                SIGTSTP_CALL = 1;
                pid = pcs_list[i]->pid;
                kill(pid,SIGTSTP);
                pcs_list[i]->flag = STOPPED;
                return;
            }
        }
    }

    if(SIGNAL == SIGCHLD){
        if(SIGCONT_CALL){
            SIGCONT_CALL = 0;
            return;
        }
        //Some process ended, reap it
        for(i=0;i<MAX_JOB;i++){
            if(pcs_list[i]){
                if(pcs_list[i]->flag == FOREGROUND || pcs_list[i]->flag == BACKGROUND) {
                    if (pcs_list[i]->flag == FOREGROUND) { FG_END = 1; usleep(100);}
                    waitpid(pcs_list[i]->pid, &status, WNOHANG);
                    return;
                }
            }
        }
    }
}

int add_command(){
    int i;
    for(i=0;i<MAX_JOB;i++){
        if(!cmd_list[i] && !pcs_list[i]){
            return i;
        }
    }
    _kill(pcs_list[MAX_JOB-1]->pid);
    return MAX_JOB-1;
}


void loop(){
    int RUNTIME;
    int cur;
    int pcs_stat;
    size_t size;
    pid_t pid;
    int in_fd;
    int out_fd;
    mode_t mode;

    struct command_text* t_cmd;

    RUNTIME = 1;
    pid = getpid();
    mode = S_IRWXU | S_IRWXG | S_IRWXO;
    in_fd = 0;
    out_fd = 0;
    while(RUNTIME){
        printf("prompt> ");
        size = (size_t)fgets(buffer,MAX_LINE,stdin);
        if(!size){
            goto END_LOOP;
        }

        t_cmd = (struct command_text*)malloc(sizeof(struct command_text));
        parsing_command(buffer,t_cmd);
        if(is_builtin(t_cmd)){
            RUNTIME = exec_builtin(t_cmd); // Check for quit command
            _command_text(t_cmd);
            t_cmd = NULL;
            goto END_LOOP;
        }

        //start process
        cur = add_command();
        cmd_list[cur] = t_cmd;
        pcs_list[cur] = (struct process*)malloc(sizeof(struct process));
        pcs_list[cur]->jid = cur;
        pcs_list[cur]->flag = t_cmd->flag;
        pcs_list[cur]->pid = getpid();

        //execute process
        pid = fork();
        if(pid == 0){
            if(t_cmd->input == IN_REDIRECT){
                in_fd = open(t_cmd->in_file,O_RDONLY,mode);
                dup2(in_fd,STDIN_FILENO);
            }
            if(t_cmd->output == OUT_REDIRECT){
                out_fd = open(t_cmd->out_file, O_CREAT|O_WRONLY|O_TRUNC,mode);
                dup2(out_fd,STDOUT_FILENO);
            }
            if(access(t_cmd->command,F_OK) == 0){
                execv(t_cmd->command,t_cmd->argv);
            }else{
                execvp(t_cmd->command,t_cmd->argv);
            }
            if(t_cmd->input == IN_REDIRECT){
                close(in_fd);
            }
            if(t_cmd->input == OUT_REDIRECT){
                close(out_fd);
            }
            exit(END);
        }else{
            pcs_list[cur]->pid = pid;
            if(pcs_list[cur]->flag == FOREGROUND){
                //Clear up foreground process when finished/ hang up process when interrupted
                while(1){
                    if(SIGTSTP_CALL){
                        pcs_stat = STOP;
                        break;
                    }
                    if(FG_END){
                        FG_END = 0;
                        pcs_stat = END;
                        break;
                    }
                }
                if(pcs_stat == END){
                    pcs_list[cur] = _process(pcs_list[cur]);
                    cmd_list[cur] = _command_text(cmd_list[cur]);
                }else{
                    SIGTSTP_CALL = 0;
                }
            }
        }

        END_LOOP:
        memset(buffer,0,sizeof(char)*MAX_LINE);
    }
}

void clear(){
    int i;
    if(buffer) free(buffer);
   
    for(i=0;i<MAX_JOB;i++){
        if(!pcs_list[i] || !cmd_list[i]){
            continue;
        }
        _kill(pcs_list[i]->pid);
        _command_text(cmd_list[i]);
        _process(pcs_list[i]);
    }
    free(cmd_list);
    free(pcs_list);
}

int get_pid(int jid){
    int i;
    for(i=0;i<MAX_JOB;i++){
        if(pcs_list[i] && pcs_list[i]->jid == jid){
            return pcs_list[i]->pid;
        }
    }
    return 0;
}

int is_builtin(struct command_text* cmd){
    if(!strcmp(cmd->command,"quit")) return 1;
    if(!strcmp(cmd->command,"jobs")) return 1;
    if(!strcmp(cmd->command,"fg")) return 1;
    if(!strcmp(cmd->command,"bg")) return 1;
    if(!strcmp(cmd->command,"kill")) return 1;
    return 0;
}

int exec_builtin(struct command_text* cmd){
    char* p;
    int jid;
    pid_t pid;
    if(!strcmp(cmd->command,"quit")){
        // 0 means end loop
        return 0;
    }
    if(!strcmp(cmd->command,"jobs")){
        // 0 means end loop
        return _jobs();
    }
    if(!strcmp(cmd->command,"kill")){
        // 0 means end loop
        if(!cmd->params[1]) return 1;
        p = cmd->params[1];
        if(p[0] == '%'){
            jid = atoi(p+sizeof(char));
            pid = get_pid(jid);
            if(pid > 0)
                return _kill(pid);
            else
                return 1;
        }else{
            pid = (pid_t)atoi(p);
            return _kill(pid);
        }
    }
    if(!strcmp(cmd->command,"bg")){
        // 0 means end loop
        if(!cmd->params[1]) return 1;
        p = cmd->params[1];
        if(p[0] == '%'){
            jid = atoi(p+sizeof(char));
            pid = get_pid(jid);
            if(pid > 0)
                return _bg(pid);
            else
                return 1;
        }else{
            pid = (pid_t)atoi(p);
            return _bg(pid);
        }
    }
    if(!strcmp(cmd->command,"fg")){
        // 0 means end loop
        if(!cmd->params[1]) return 1;
        p = cmd->params[1];
        if(p[0] == '%'){
            jid = atoi(p+sizeof(char));
            pid = get_pid(jid);
            if(pid > 0)
                return _fg(pid);
            else
                return 1;
        }else{
            pid = (pid_t)atoi(p);
            return _fg(pid);
        }
    }
}

int _jobs(){
    int i,j;
    int new_jid;
    new_jid = 1;
    for(i=0;i<MAX_JOB;i++){
        if(pcs_list[i]){
            if(pcs_list[i]->flag == BACKGROUND){
                printf("[%d] (%d) Running ",new_jid,pcs_list[i]->pid);
                pcs_list[i]->jid = new_jid++;
            }
            if(pcs_list[i]->flag == STOPPED){
                printf("[%d] (%d) Stopped ",new_jid,pcs_list[i]->pid);
                pcs_list[i]->jid = new_jid++;
            }
            for(j=0;j<MAX_ARG;j++){
                if(!cmd_list[i]->params[j]) break;
                printf("%s ",cmd_list[i]->params[j]);
            }
            printf("\n");
        }
    }
    return 1;
}


int _bg(pid_t pid){
    int i;
    for(i=0;i<MAX_JOB;i++){
        if(pcs_list[i] && pcs_list[i]->flag == STOPPED && pcs_list[i]->pid == pid){
            kill(pcs_list[i]->pid,SIGCONT);
            SIGCONT_CALL = 1;
            pcs_list[i]->flag = BACKGROUND;
        }
    }
    return 1;
}

int _fg(pid_t pid){
    int i,pcs_stat;
    for(i=0;i<MAX_JOB;i++){
        if(pcs_list[i] && pcs_list[i]->pid == pid){
            pcs_list[i]->flag = FOREGROUND;
            kill(pcs_list[i]->pid,SIGCONT);
            SIGCONT_CALL = 1;
            FG_END = 0;
            SIGTSTP_CALL = 0;
            while(1){
                if(SIGTSTP_CALL){
                    pcs_stat = STOP;
                    break;
                }
                if(FG_END){
                    FG_END = 0;
                    pcs_stat = END;
                    break;
                }
            }
            if(pcs_stat == END){
                pcs_list[i] = _process(pcs_list[i]);
                cmd_list[i] = _command_text(cmd_list[i]);
            }else{
                SIGTSTP_CALL = 0;
            }
        }
    }
    return 1;
}

int _kill(pid_t pid){
    int i;
    kill(pid,SIGKILL);
    while(pid!=waitpid(pid,&i,WNOHANG)){}
    for(i=0;i<MAX_JOB;i++){
        if(pcs_list[i] && pcs_list[i]->pid == pid){
            pcs_list[i] = _process(pcs_list[i]);
            cmd_list[i] = _command_text(cmd_list[i]);
            pcs_list[i] = NULL;
            cmd_list[i] = NULL;
        }
    }
    return 1;
}


void* _command_text(struct command_text* cmd){
    int i;
    if(!cmd) return NULL;
    if(cmd->in_file){
        free(cmd->in_file);
    }
    if(cmd->out_file){
        free(cmd->out_file);
    }
    if(cmd->command){
        free(cmd->command);
    }
    if(cmd->end_of_param > 0){
        i = 0;
        while(cmd->params[i]){
            free(cmd->params[i++]);
        }
        i = 0;
        while(cmd->argv[i]){
            free(cmd->argv[i++]);
        }
    }else{
        i = 0;
        while(cmd->params[i]){
            free(cmd->params[i++]);
        }
    }
    free(cmd->params);
    if(cmd->end_of_param > 0) free(cmd->argv);
    free(cmd);
    return NULL;
}

void* _process(struct process* pcs){
    if(!pcs) return NULL;
    free(pcs);
    return NULL;
}

void parsing_command(char* buffer, struct command_text* cmd){
    char** params;
    char* p;
    int i;
    params = (char**)malloc(sizeof(char*)*MAX_ARG);
    for(i=0;i<MAX_ARG;i++){
        params[i] = (char*)malloc(sizeof(char)*MAX_LINE);
        memset(params[i],0,sizeof(char)*MAX_LINE);
    }

    p = strtok(buffer," \t\n");
    cmd->command = (char*)malloc(sizeof(char)*MAX_LINE);
    cmd->params = (char**)malloc(sizeof(char**)*MAX_ARG);
    memset(cmd->command,0,sizeof(char)*MAX_LINE);
    memset(cmd->params,0,sizeof(char)*MAX_LINE);
    strcpy(cmd->command,p);
    cmd->flag = FOREGROUND;
    cmd->input = STDIN_FILENO;
    cmd->output = STDOUT_FILENO;
    cmd->end_of_param = -1;
    i = 0;
    cmd->params[i] = (char*)malloc(sizeof(char)*MAX_LINE);
    strcpy(cmd->params[i++],cmd->command);
    while(1){
        p = strtok(NULL, " \t\n");
        if(!p) break;
        cmd->params[i] = (char*)malloc(sizeof(char)*MAX_LINE);
        strcpy(cmd->params[i],p);

        if(strlen(p) == 1 && ispunct(p[0])){
            if(cmd->end_of_param < 0) {cmd->end_of_param = i;}
            switch(p[0]){
                case '<':
                    cmd->input = IN_REDIRECT + i+1;
                    break;
                case '>':
                    cmd->output = OUT_REDIRECT + i+1;
                    break;
                case '&':
                    cmd->flag = BACKGROUND;
                    break;
                default:
                    cmd->end_of_param = -1;
                    break;
            }
        }

        i++;
    }
    cmd->in_file = NULL;
    cmd->out_file = NULL;

    if(cmd->input != STDIN_FILENO){
        cmd->in_file = (char*)malloc(sizeof(char)*MAX_LINE);
        strcpy(cmd->in_file, cmd->params[cmd->input-IN_REDIRECT]);
        cmd->input = IN_REDIRECT;
    }
    if(cmd->output != STDOUT_FILENO){
        cmd->out_file = (char*)malloc(sizeof(char)*MAX_LINE);
        strcpy(cmd->out_file, cmd->params[cmd->output-OUT_REDIRECT]);
        cmd->output = OUT_REDIRECT;
    }

    if(cmd->end_of_param > 0){
        cmd->argv = (char**)malloc(sizeof(char*)*cmd->end_of_param);
        memset(cmd->argv,0,sizeof(char*)*cmd->end_of_param);
        for(i=0;i<cmd->end_of_param;i++){
            cmd->argv[i] = (char*)malloc(sizeof(char)*strlen(cmd->params[i]));
            strcpy(cmd->argv[i],cmd->params[i]);
        }
    }else{
        cmd->argv = cmd->params;
    }
}
