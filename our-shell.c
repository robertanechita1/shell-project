#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h> // pt manipularea directoarelor
#include <sys/stat.h>  // pt stat()
#include <time.h> // pt conversia timestamp-ului
#include <pwd.h> // pt info despre utilizator
#include <grp.h> // pt info despre grup
#include <sys/wait.h>  // pt wait()
#include <ctype.h>

char *history_c[1024];  // Vector de siruri pentru comenzile introduse
int cmd_count = 1;
char temp_input[200][200];

int procesare_comanda(char *c);
void process_command(char *cmd);


//afisam numele shell-ului si path ul inainte de a scrie comanda
void show(){ 
    char cwd[1000];
    char rez[1000] = "OurShell: ";
    if(getcwd(cwd, sizeof(cwd)) == NULL) 
        perror("Eroare la afisarea path ului\n"); //in cwd va fi stocata calea absoluta a directorului curent sau NULL daca esueaza
    else{
        strcat(rez, cwd); 
        strcat(rez, "$ ");
        printf("%s", rez);
    }
}

//citirea comenzii
void readInput(char *input){
    show();

    if (fgets(input, 1024, stdin) == NULL) {
        perror("Eroare la citirea comenzii");
        exit(1); 
    }
    input[strcspn(input, "\n")] = 0; // elimin \n de la final (daca are)

    history_c[cmd_count] = strdup(input); //copiez numele comenzii in istoric
    cmd_count++; //cresc nr de comenzi apelate pana in momentul actual
}

int cmp(const void *s1, const void *s2) { //sortare in ordine lexicografica
    return strcmp(*(const char **)s1, *(const char **)s2);
}

int cmpRev(const void *s1, const void *s2) { //sortare in ordine invers lexicografica
    return strcmp(*(const char **)s2, *(const char **)s1);
}

void printPermissions(mode_t mode) {//afisarea permisiunilor unui fisier/director
    printf( (S_ISDIR(mode)) ? "d" : "-"); //este director
    printf( (mode & S_IRUSR) ? "r" : "-"); //proprietar: read
    printf( (mode & S_IWUSR) ? "w" : "-"); //proprietar: write
    printf( (mode & S_IXUSR) ? "x" : "-"); //proprietar: exec
    printf( (mode & S_IRGRP) ? "r" : "-"); //grup: read
    printf( (mode & S_IWGRP) ? "w" : "-"); //grup: write
    printf( (mode & S_IXGRP) ? "x" : "-"); //grup: exec
    printf( (mode & S_IROTH) ? "r" : "-"); //others: read
    printf( (mode & S_IWOTH) ? "w" : "-"); //others: write
    printf( (mode & S_IXOTH) ? "x" : "-"); //others: exec
    printf(" ");
}

void f_history(){
    for(int i = 1; i < cmd_count; i++)
        printf("%d %s\n", i, history_c[i]);
}

void f_ls(){
    struct dirent *file; // struct cu info despre fisier/dir: d_ino, d_off, d_reclen, d_type, d_name
    DIR *dir = opendir(".");

    if (dir == NULL) {
        perror("Eroare la deschiderea directorului");
        exit(1);
    }

    char *files[1024]; 
    int cnt = 0;

    //citesc si stochez toate fisierele/directoarele din dir curent in vectorul files
    while ((file = readdir(dir)) != NULL) {
        if (file->d_name[0] != '.') { // adica nu e ascuns
            files[cnt] = strdup(file->d_name); //copiaza numele de fisier, alocat dinamic
            cnt++;
        }
    }

    closedir(dir);

    qsort(files, cnt, sizeof(char *), cmp); //sortez in ordine lexicografica file-urile

    for (int i = 0; i < cnt; i++) {
        printf("%s  ", files[i]);
        free(files[i]); 
    }
    printf("\n");
}

void f_ls_l(){
    struct dirent *file; // struct cu info despre fisier/dir: d_ino, d_off, d_reclen, d_type, d_name
    DIR *dir = opendir(".");

    if (dir == NULL) {
        perror("Eroare la deschiderea directorului");
        exit(1);
    }

    char *files[1024]; 
    int cnt = 0;

    //citesc si stochez toate fisierele/directoarele din dir curent in vectorul files
    while ((file = readdir(dir)) != NULL) {
        if (file->d_name[0] != '.') { // adica nu e ascuns
            files[cnt] = strdup(file->d_name); //copiaza numele de fisier, alocat dinamic
            cnt++;
        }
    }

    closedir(dir);

    qsort(files, cnt, sizeof(char *), cmp); //sortez in ordine lexicografica file-urile

    for (int i = 0; i < cnt; i++) {
        struct stat fileStat; //struct cu info detaliate despre fisier/dir: st_div, st_ino, st_mode, st_nlink, st_mtime etc 
        if (stat(files[i], &fileStat) < 0) { //incerc sa obtin informatiile din fisier
            perror("Eroare la obtinerea informatiilor despre file");
            exit(1);
        }
        //else
        printPermissions(fileStat.st_mode); // afisez permisiunile

        printf("%2ld ", fileStat.st_nlink); // nr de legaturi

        struct passwd *pw = getpwuid(fileStat.st_uid); // afisez proprietarul
        if (pw != NULL) {
            printf("%s ", pw->pw_name);
        } 
        else {
            printf("%d ", fileStat.st_uid);
        }

        struct group *gr = getgrgid(fileStat.st_gid); //afisez grupul
        if (gr != NULL) {
            printf("%s ", gr->gr_name);
        } 
        else {
            printf("%d ", fileStat.st_gid);
        }

        printf("%5ld ", fileStat.st_size); //dimensiune fisier

        char timeStr[20]; //data ultimei modificari
        strftime(timeStr, sizeof(timeStr), "%b %d %H:%M", localtime(&fileStat.st_mtime)); //localtime transf nr de secunde in data, ora, minut, secunda etc si se muta in timeStr cu formatul %b %d %H:%M
        printf("%s ", timeStr);

        printf("%s\n", files[i]); //nume fisier
    }
}

void f_ls_a(){
    struct dirent *file; // struct cu info despre fisier/dir: d_ino, d_off, d_reclen, d_type, d_name
    DIR *dir = opendir(".");

    if (dir == NULL) {
        perror("Eroare la deschiderea directorului");
        exit(1);
    }

    char *files[1024]; 
    int cnt = 0;

    //citesc si stochez toate fisierele/directoarele din dir curent in vectorul files
    while ((file = readdir(dir)) != NULL) {
        files[cnt] = strdup(file->d_name); //copiaza numele de fisier, alocat dinamic
        cnt++;
    }

    closedir(dir);

    qsort(files, cnt, sizeof(char *), cmp); //sortez in ordine lexicografica file-urile

    for (int i = 0; i < cnt; i++) {
        printf("%s  ", files[i]);
        free(files[i]); 
    }
    printf("\n");
}

void f_ls_r(){
    struct dirent *file; // struct cu info despre fisier/dir: d_ino, d_off, d_reclen, d_type, d_name
    DIR *dir = opendir(".");

    if (dir == NULL) {
        perror("Eroare la deschiderea directorului");
        exit(1);
    }

    char *files[1024]; 
    int cnt = 0;

    //citesc si stochez toate fisierele/directoarele din dir curent in vectorul files
    while ((file = readdir(dir)) != NULL) {
        if (file->d_name[0] != '.') { // adica nu e ascuns
            files[cnt] = strdup(file->d_name); //copiaza numele de fisier, alocat dinamic
            cnt++;
        }
    }

    closedir(dir);

    qsort(files, cnt, sizeof(char *), cmpRev); //sortez in ordine invers lexicografica file-urile

    for (int i = 0; i < cnt; i++) {
        printf("%s  ", files[i]);
        free(files[i]); 
    }
    printf("\n");
}

void f_ls_ar(){
    struct dirent *file; // struct cu info despre fisier/dir: d_ino, d_off, d_reclen, d_type, d_name
    DIR *dir = opendir(".");

    if (dir == NULL) {
        perror("Eroare la deschiderea directorului");
        exit(1);
    }

    char *files[1024]; 
    int cnt = 0;

    //citesc si stochez toate fisierele/directoarele din dir curent in vectorul files
    while ((file = readdir(dir)) != NULL) {
        files[cnt] = strdup(file->d_name); //copiaza numele de fisier, alocat dinamic
        cnt++;
    }

    closedir(dir);

    qsort(files, cnt, sizeof(char *), cmpRev); //sortez in ordine lexicografica file-urile

    for (int i = 0; i < cnt; i++) {
        printf("%s  ", files[i]);
        free(files[i]); 
    }
    printf("\n");
}

void f_ls_la(){
    struct dirent *file; // struct cu info despre fisier/dir: d_ino, d_off, d_reclen, d_type, d_name
    DIR *dir = opendir(".");

    if (dir == NULL) {
        perror("Eroare la deschiderea directorului");
        exit(1);
    }

    char *files[1024]; 
    int cnt = 0;

    //citesc si stochez toate fisierele/directoarele din dir curent in vectorul files
    while ((file = readdir(dir)) != NULL) {
        files[cnt] = strdup(file->d_name); //copiaza numele de fisier, alocat dinamic
        cnt++;
    }

    closedir(dir);

    qsort(files, cnt, sizeof(char *), cmp); //sortez in ordine lexicografica file-urile

    for (int i = 0; i < cnt; i++) {
        struct stat fileStat; //struct cu info detaliate despre fisier/dir: st_div, st_ino, st_mode, st_nlink, st_mtime etc 
        if (stat(files[i], &fileStat) < 0) { //incerc sa obtin informatiile din fisier
            perror("Eroare la obtinerea informatiilor despre file");
            exit(1);
        }
        //else
        printPermissions(fileStat.st_mode); // afisez permisiunile

        printf("%2ld ", fileStat.st_nlink); // nr de legaturi

        struct passwd *pw = getpwuid(fileStat.st_uid); // afisez proprietarul
        if (pw != NULL) {
            printf("%s ", pw->pw_name);
        } 
        else {
            printf("%d ", fileStat.st_uid);
        }

        struct group *gr = getgrgid(fileStat.st_gid); //afisez grupul
        if (gr != NULL) {
            printf("%s ", gr->gr_name);
        } 
        else {
            printf("%d ", fileStat.st_gid);
        }

        printf("%5ld ", fileStat.st_size); //dimensiune fisier

        char timeStr[20]; //data ultimei modificari
        strftime(timeStr, sizeof(timeStr), "%b %d %H:%M", localtime(&fileStat.st_mtime)); //localtime transf nr de secunde in data, ora, minut, secunda etc si se muta in timeStr cu formatul %b %d %H:%M
        printf("%s ", timeStr);

        printf("%s\n", files[i]); //nume fisier
    }
}

void f_ls_lr(){
    struct dirent *file; // struct cu info despre fisier/dir: d_ino, d_off, d_reclen, d_type, d_name
    DIR *dir = opendir(".");

    if (dir == NULL) {
        perror("Eroare la deschiderea directorului");
        exit(1);
    }

    char *files[1024]; 
    int cnt = 0;

    //citesc si stochez toate fisierele/directoarele din dir curent in vectorul files
    while ((file = readdir(dir)) != NULL) {
        if (file->d_name[0] != '.') { // adica nu e ascuns
            files[cnt] = strdup(file->d_name); //copiaza numele de fisier, alocat dinamic
            cnt++;
        }
    }

    closedir(dir);

    qsort(files, cnt, sizeof(char *), cmpRev); //sortez in ordine invers lexicografica file-urile

    for (int i = 0; i < cnt; i++) {
        struct stat fileStat; //struct cu info detaliate despre fisier/dir: st_div, st_ino, st_mode, st_nlink, st_mtime etc 
        if (stat(files[i], &fileStat) < 0) { //incerc sa obtin informatiile din fisier
            perror("Eroare la obtinerea informatiilor despre file");
            exit(1);
        }
        //else
        printPermissions(fileStat.st_mode); // afisez permisiunile

        printf("%2ld ", fileStat.st_nlink); // nr de legaturi

        struct passwd *pw = getpwuid(fileStat.st_uid); // afisez proprietarul
        if (pw != NULL) {
            printf("%s ", pw->pw_name);
        } 
        else {
            printf("%d ", fileStat.st_uid);
        }

        struct group *gr = getgrgid(fileStat.st_gid); //afisez grupul
        if (gr != NULL) {
            printf("%s ", gr->gr_name);
        } 
        else {
            printf("%d ", fileStat.st_gid);
        }

        printf("%5ld ", fileStat.st_size); //dimensiune fisier

        char timeStr[20]; //data ultimei modificari
        strftime(timeStr, sizeof(timeStr), "%b %d %H:%M", localtime(&fileStat.st_mtime)); //localtime transf nr de secunde in data, ora, minut, secunda etc si se muta in timeStr cu formatul %b %d %H:%M
        printf("%s ", timeStr);

        printf("%s\n", files[i]); //nume fisier
    }
}

void f_ls_lar(){
    struct dirent *file; // struct cu info despre fisier/dir: d_ino, d_off, d_reclen, d_type, d_name
    DIR *dir = opendir(".");

    if (dir == NULL) {
        perror("Eroare la deschiderea directorului");
        exit(1);
    }

    char *files[1024]; 
    int cnt = 0;

    //citesc si stochez toate fisierele/directoarele din dir curent in vectorul files
    while ((file = readdir(dir)) != NULL) {
        files[cnt] = strdup(file->d_name); //copiaza numele de fisier, alocat dinamic
        cnt++;
    }

    closedir(dir);

    qsort(files, cnt, sizeof(char *), cmpRev); //sortez in ordine lexicografica file-urile

    for (int i = 0; i < cnt; i++) {
        struct stat fileStat; //struct cu info detaliate despre fisier/dir: st_div, st_ino, st_mode, st_nlink, st_mtime etc 
        if (stat(files[i], &fileStat) < 0) { //incerc sa obtin informatiile din fisier
            perror("Eroare la obtinerea informatiilor despre file");
            exit(1);
        }
        //else
        printPermissions(fileStat.st_mode); // afisez permisiunile

        printf("%2ld ", fileStat.st_nlink); // nr de legaturi

        struct passwd *pw = getpwuid(fileStat.st_uid); // afisez proprietarul
        if (pw != NULL) {
            printf("%s ", pw->pw_name);
        } 
        else {
            printf("%d ", fileStat.st_uid);
        }

        struct group *gr = getgrgid(fileStat.st_gid); //afisez grupul
        if (gr != NULL) {
            printf("%s ", gr->gr_name);
        } 
        else {
            printf("%d ", fileStat.st_gid);
        }

        printf("%5ld ", fileStat.st_size); //dimensiune fisier

        char timeStr[20]; //data ultimei modificari
        strftime(timeStr, sizeof(timeStr), "%b %d %H:%M", localtime(&fileStat.st_mtime)); //localtime transf nr de secunde in data, ora, minut, secunda etc si se muta in timeStr cu formatul %b %d %H:%M
        printf("%s ", timeStr);

        printf("%s\n", files[i]); //nume fisier
    }
}

void f_mkdir(const char *dir) {
    if (mkdir(dir, 0755) == -1) { // 0755 sunt permisiunile obisnuite (rwxr-xr-x)
        perror("Eroare la crearea directorului");
    } else {
        printf("Directorul '%s' a fost creat.\n", dir);
    }
}

void f_rmdir(const char *dir) {
    if (rmdir(dir) == -1) {
        perror("Eroare la stergerea directorului");
    } else {
        printf("Directorul '%s' a fost sters.\n", dir);
    }
}

void f_pwd() {
    char cwd[1024];  // buffer pentru stocarea caii directorului curent
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);  // afiseaza calea absoluta a directorului curent
    } else {
        perror("Eroare la afisarea directorului curent");  
    }
}

void trim(char *str) {
    char *start = str; 
    char *end;

    // elimină spatiile de la inceput
    while (isspace((unsigned char)*start)) start++;
    
    // verifica daca sirul este format doar din spatii
    if (*start == 0) {
        *str = 0; // seteaza sirul ca fiind gol
        return;
    }

    // elimina spatiile de la sfarsit
    end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) end--;
    end[1] = '\0'; // Pune terminatorul de șir

    // muta sirul trimuit la inceputul bufferului daca este necesar
    if (str != start) {
        memmove(str, start, end + 2 - start);
    }
}

char **parse_command(char *command) {
    static char *args[10]; // array de pointeri catre argumentele comenzi
    int i = 0;
    char *token = strtok(command, " "); 
    while (token != NULL) {
        args[i++] = token; // stocheaza fiecare argument in array
        token = strtok(NULL, " "); 
    }
    args[i] = NULL;
    return args;
}

char *trim_spaces(char *str) {
    while (*str == ' ') str++;
    
    char *end = str + strlen(str) - 1;
    while (end > str && *end == ' ') {
        *end = '\0';
        end--;
    }
    return str;
}

void comenzi_logice(char *linie_comanda) {
    char temp[200];
    strcpy(temp, linie_comanda);

    // Pointer pentru strtok_r
    char *saveptr;

    if (strstr(temp, "&&") != NULL) {
        char *cmd = strtok_r(temp, "&&", &saveptr);  // strtok_r e thread-safe
        while (cmd != NULL) {
            cmd = trim_spaces(cmd);

            int status = procesare_comanda(cmd);
            if(status == 0){
                break;
            }

            cmd = strtok_r(NULL, "&&", &saveptr);
        }
    }
    else if(strstr(temp, "||") != NULL){
        char *cmd = strtok_r(temp, "||", &saveptr);  // strtok_r e thread-safe
        while (cmd != NULL) {
            cmd = trim_spaces(cmd);
            int status;
            
            if(strcmp(cmd, "false") == 0)
                status = 1;
            else if(strcmp(cmd, "true") == 0)
                status = 0;
            else
                status = procesare_comanda(cmd);

            if(status != 0){
                break;
            }

            cmd = strtok_r(NULL, "||", &saveptr);
        }    
    }
}

void comanda_echo(int cnt){
    for(int i = 1; i < cnt; i++){
        for(int c = 0; c < strlen(temp_input[i]); c++)
            if (temp_input[i][c] != '"')
                printf("%c", temp_input[i][c]);
        printf(" ");
    }
        
    printf("\n");
}

void comanda_cd(int cnt){
    if (cnt != 2) {
        printf("Eroare: Trebuie sa specifici calea directorului.\n");
        return;
    }
    
    int result = chdir(temp_input[1]);
    if (result == 0) {
        printf("Director schimbat la: %s\n", temp_input[1]);
    } else {
        perror("Eroare la schimbarea directorului");
    }
}

void f_suspend() {
    printf("Programul este suspendat. Apăsați Enter pentru a relua.\n");
    getchar(); 
}

void comanda_pipe(char *linie_comanda) {
    char temp[200];
    strcpy(temp, linie_comanda);

    char *cmd1 = strtok(temp, "|");
    char *cmd2 = strtok(NULL, "");

    if (cmd1 == NULL || cmd2 == NULL) {
        printf("Format invalid pentru pipe.\n");
        return;
    }

    cmd1 = trim_spaces(cmd1);
    cmd2 = trim_spaces(cmd2);
    

    int fd[2];
    if (pipe(fd) == -1) {
        perror("Eroare la crearea pipe-ului");
        return;
    }

    pid_t pid1 = fork();
    if (pid1 == 0){
        close(fd[0]);  
        dup2(fd[1], STDOUT_FILENO);  
        close(fd[1]);

        char *argv1[] = {"/bin/sh", "-c", cmd1, NULL};
        execve("/bin/sh", argv1, NULL);  
        perror("Eroare la executia cmd1");
        exit(1);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {  // Al doilea proces copil
        close(fd[1]);  
        dup2(fd[0], STDIN_FILENO);  
        close(fd[0]);

        char *argv2[] = {"/bin/sh", "-c", cmd2, NULL};
        execve("/bin/sh", argv2, NULL);  
        perror("Eroare la executia cmd2");
        exit(1);
    }

    close(fd[0]);
    close(fd[1]);

    wait(NULL);
    wait(NULL);   
}

void comanda_grep(int cnt){
    if (cnt < 3){
        printf("Utilizare incorecta grep\n");
        return;
    }

    pid_t pid = fork();
    if (pid == 0) {
        char *argv[cnt + 1];
        argv[0] = "/bin/grep";
        argv[1] = temp_input[1];
        for (int i = 2; i < cnt; i++) {
            argv[i] = temp_input[i];
        }
        argv[cnt] = NULL;

        execve("/bin/grep", argv, NULL);
        perror("Eroare la executia comenzii grep");
    } else {
        wait(NULL);
    }
}


int procesare_comanda(char *c){
    char linie_comanda[200]; 
    strcpy(linie_comanda, c);
    char *token = strtok(c, " ");
    int cnt = 0;
    while (token != NULL)
    {
        strcpy(temp_input[cnt], token);            
        cnt++;
        token = strtok(NULL, " ");
    }
    
    if(strstr(linie_comanda, "&&") || strstr(linie_comanda, "||"))
        comenzi_logice(linie_comanda);
    else if(strstr(linie_comanda, "|"))
        comanda_pipe(linie_comanda);
    else if (strncmp(linie_comanda, "cd", 2) == 0)
        comanda_cd(cnt);
    else if (strncmp(linie_comanda, "echo", 4) == 0)
        comanda_echo(cnt);
    else if (strncmp(linie_comanda, "grep", 4) == 0)
        comanda_grep(cnt);
    else
        process_command(linie_comanda);

}

void process_command(char *command) {
    char *cmd = strtok(command, " "); // prima parte a comenzii pentru a det actiunea
    char *arg = strtok(NULL, " ");    // al doilea argument daca exista

    if (strcmp(cmd, "exit") == 0) {
        printf("Iesire din shell.\n");
        exit(0);
    } else if (strcmp(cmd, "ls") == 0) {
        if (arg == NULL) {
            f_ls();
        } else {
            if (strcmp(arg, "-l") == 0) f_ls_l();
            else if (strcmp(arg, "-a") == 0) f_ls_a();
            else if (strcmp(arg, "-r") == 0) f_ls_r();
            else if (strcmp(arg, "-ar") == 0 || strcmp(arg, "-ra") == 0) f_ls_ar();
            else if (strcmp(arg, "-la") == 0 || strcmp(arg, "-al") == 0) f_ls_la();
            else if (strcmp(arg, "-lr") == 0 || strcmp(arg, "-rl") == 0) f_ls_lr();
            else if (strcmp(arg, "-lar") == 0 || strcmp(arg, "-lra") == 0 || strcmp(arg, "-arl") == 0 || strcmp(arg, "-alr") == 0 || strcmp(arg, "-rla") == 0 || strcmp(arg, "-ral") == 0) f_ls_lar();
            else printf("Argumentul %s nu este recunoscut.\n", arg);
        }
    } else if (strcmp(cmd, "history") == 0) {
        f_history();
    } else if (strcmp(cmd, "pwd") == 0) {
        f_pwd();
    } else if (strcmp(cmd, "mkdir") == 0) {
        if (arg == NULL) {
            printf("Specificati numele directorului.\n");
        } else {
            if (mkdir(arg, 0755) == -1) { 
                perror("Eroare la crearea directorului");
            } else {
                printf("Directorul '%s' a fost creat.\n", arg);
            }
        }
    } else if (strcmp(cmd, "rmdir") == 0) {
        if (arg == NULL) {
            printf("Specificati numele directorului pentru a fi sters.\n");
        } else {
            if (rmdir(arg) == -1) { 
                perror("Eroare la stergerea directorului");
            } else {
                printf("Directorul '%s' a fost sters.\n", arg);
            }
        }
    } else {
        printf("Comanda %s nu a fost gasita.\n", cmd);
    }
}


int main(int argv, char *argc[])
{
    printf("Puteti iesi folosind CTRL+C sau comanda exit.\n");
    char command[1024];

    while(1)
    {
        readInput(command);
        if (strcmp(command, "suspend") == 0) 
            f_suspend();
        else if(command[0] != '\0')
            procesare_comanda(command); 
    }

    return 0;
}