#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h> // pt manipularea directoarelor
#include <sys/stat.h>  // pt stat()
#include <time.h> // pt conversia timestamp-ului
#include <pwd.h> // pt info despre utilizator
#include <grp.h> // pt info despre grup

char *history_c[1024];  // Vector de șiruri pentru comenzile introduse
int cmd_count = 1;

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

int main(int argv, char *argc[])
{
    printf("Puteti iesi folosind CTRL+C sau comanda exit.\n");
    char command[1024];

    while(1)
    {
        readInput(command);
        if (strcmp(command, "exit") == 0) { //iesirea din shell
            printf("Iesire din shell.\n");
            exit(0);
        }
        else if(strcmp(command, "ls") == 0){ //afisarea fisierelor/dir din directorul curent
            f_ls();
            continue;
        }
        else if(strcmp(command, "ls -l") == 0){//afisarea detaliata a fisierelor/dir din directorul curent
            f_ls_l();
            continue;
        }
        else if(strcmp(command, "ls -a") == 0){//afisarea fisierelor/dir din directorul curent, inclusiv cele ascunse
            f_ls_a();
            continue;
        }
        else if(strcmp(command, "ls -r") == 0){//afisarea fisierelor/dir din directorul curent in ordine inversa
            f_ls_r();
            continue;
        }
        else if(strcmp(command, "ls -ar") == 0){//afisarea fisierelor/dir din directorul curent in ordine inversa, inclusiv cele ascunse
            f_ls_ar();
            continue;
        }
        else if(strcmp(command, "ls -la") == 0){//afisarea detaliata a fisierelor/dir din directorul curent, inclusiv cele ascunse
            f_ls_la();
            continue;
        }
        else if(strcmp(command, "ls -lr") == 0){//afisarea detaliata a fisierelor/dir din directorul curent in ordine inversa
            f_ls_lr();
            continue;
        }
        else if(strcmp(command, "ls -lar") == 0){//afisarea detaliata a fisierelor/dir din directorul curent in ordine inversa, inclusiv cele ascunse
            f_ls_lar();
            continue;
        }
        else if(strcmp(command, "history") == 0){ //istoricul comenzilor pana in momentul actual
            f_history();
            continue;
        }
        else{
            printf("Comanda %s nu a fost gasita.\n", command);
        }
    }

    return 0;
}