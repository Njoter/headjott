#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

#define MAX_LENGTH 32

char* HOME;
char* PATH_TO_FILES;
char* notebook;
char* header;
char* notebook_path;
char* header_path;

void usage(char program_name[]);
void help();
int add(char* arg);
int create_notebook(char* foldername);
int create_header(char* filename);
int append_line(char* arg);
int folder_exists(char* path);
void set_notebook(char* arg);
int set_header(char* arg);
void free_stuff();
int file_exists(char* path);
void print(int all);
void print_notebook(char* s, int all);
void print_headers(char* path, int size, int indent, int all);
int print_lines(char* path, int indent);
int count_files(char* path);
void get_filenames(char* strings[], char* path);


int main(int argc, char *argv[]) {

    char* program_name = "headjott";
    int c;

    HOME = getenv("HOME");
    if (HOME != NULL) {
        char* s = "/.local/share/";
        char* t = "/files/";
        PATH_TO_FILES = malloc(
                strlen(HOME) + strlen(s) + strlen(program_name) + strlen(t)
        );
        strcpy(PATH_TO_FILES, HOME);
        strcat(PATH_TO_FILES, s);
        strcat(PATH_TO_FILES, program_name);
        strcat(PATH_TO_FILES, t);
    } else {
        fprintf(stderr, "Error: varible HOME is null");
        exit(1);
    }

    while (1) {
        static struct option long_options[] = {
            {"notebook",    required_argument,  0,  'n'},
            {"header",      required_argument,  0,  'h'},
            {"add",         required_argument,  0,  'a'},
            {"remove",      required_argument,  0,  'r'},
            {"print",       no_argument,        0,  'p'},
            {"print-all",   no_argument,        0,  'P'},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        c = getopt_long (argc, argv, "n:h:a:r:pP", long_options, &option_index);

        if (c == -1)
            break;

        switch (c) {
            case 'n':
                set_notebook(optarg);
                break;
            case 'h':
                if (set_header(optarg) != 0) {
                    free_stuff();
                    exit(1);
                }
                break;
            case 'a':
                add(optarg);
                break;
            case 'r':
                printf("case r - optarg: %s", optarg);
                break;
            case 'p':
                print(0);
                break;
            case 'P':
                print(1);
                break;
        }
    }

    //usage(program_name);
    free_stuff();
    exit(0);
}

void free_stuff() {
    if (notebook_path != NULL) {
        free(notebook_path);
    }
    if (header_path != NULL) {
        free(header_path);
    }
    free(PATH_TO_FILES);
}

void usage(char program_name[]) {
    fprintf(stderr, "usage: %s [OPTION] {ARGUMENT}", program_name);
}

void help() {
    printf("This is the help function");
}

int add(char* arg) {

    if (notebook == NULL) {
        create_notebook(arg);
    } else if (folder_exists(notebook_path)) {
        if (header == NULL) {
            create_header(arg);
        } else if (file_exists(header_path)) {
            append_line(arg);
        }
    }
    return 0;
}

int create_notebook(char* arg) {

    char dir[strlen(PATH_TO_FILES) + strlen(arg)];
    strcpy(dir, PATH_TO_FILES);
    strcat(dir, arg);

    if (mkdir(dir, 0777) != 0) {
        perror("Error creating notebook");
        return 1;
    }

    printf("Successfully created notebook %s", arg);
    return 0;
}

int create_header(char* arg) {

    set_header(arg);

    if (file_exists(header_path)) {
        fprintf(stderr, "Header %s already exists in %s", arg, notebook);
        return 1;
    }

    FILE* file = fopen(header_path, "w");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    fclose(file);
    return 0;
}

int append_line(char* arg) {

    FILE* file = fopen(header_path, "a");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    fprintf(file, "%s\n", arg);
    fclose(file);
    return 0;
}

int folder_exists(char* path) {

    DIR* dir = opendir(path);

    if (dir) {
        closedir(dir);
        return 1;
    } else if (ENOENT == errno) {
        fprintf(stderr, "Error: can't find %s", path);
        return 0;
    } else {
        fprintf(stderr, "Error opening %s", path);
        return -1;
    }
}

int file_exists(char* path) {

    FILE* file;

    if ((file = fopen(path, "r")) == NULL) {
        return 0;
    } else {
        fclose(file);
        return 1;
    }
}

void set_notebook(char* arg) {

    notebook = arg;
    char* s = malloc(strlen(PATH_TO_FILES) + strlen(notebook));
    strcpy(s, PATH_TO_FILES);
    strcat(s, notebook);
    notebook_path = s;
}

int set_header(char* arg) {

    if (notebook_path == NULL) {
        fprintf(stderr, "Error: notebook not set. Please set notebook before setting header.");
        return 1;
    }    

    header = arg;
    char* s = malloc(strlen(notebook_path) + strlen(arg) + 1);
    strcpy(s, notebook_path);
    strcat(s, "/");
    strcat(s, arg);
    header_path = s;

    return 0;
}

void print(int all) {

    int size = count_files(PATH_TO_FILES);
    char* arr[size];

    printf("-----------------------------------------------------------\n");
    if (notebook == NULL) {
        get_filenames(arr, PATH_TO_FILES);
        for (int i = 0; i < size; i++) {
            print_notebook(arr[i], all);
            free(arr[i]);
        }
    } else if (header == NULL) {
        print_notebook(notebook, all);
    } else {
        print_lines(header_path, 0);
    }
    printf("-----------------------------------------------------------\n");
}

void print_notebook(char* s, int all) {

    char* path = malloc(strlen(PATH_TO_FILES) + strlen(s));
    strcpy(path, PATH_TO_FILES);
    strcat(path, s);

    printf("%s\n", s);
    int size = count_files(path);
    if (size > 0) {
        print_headers(path, size, 1, all);
    }
}

void print_headers(char* path, int size, int indent, int all) {
    char* arr[size];
    get_filenames(arr, path);
    for (int i = 0; i < size; i++) {
        if (indent == 1) {
            printf("    ");
        }
        printf("# %s\n", arr[i]);
        if (all == 1) {
            char* s = malloc(strlen(path) + strlen(arr[i]) + 1);
            strcpy(s, path);
            strcat(s, "/");
            strcat(s, arr[i]);
            print_lines(s, 1);
            free(s);
        }
        free(arr[i]);
    }
}

int print_lines(char* path, int indent) {

    FILE* file = fopen(path, "r");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    char s[256];
    while(fgets(s, 256, file)) {
        if (indent == 1) {
            printf("        ");
        }
        printf("* %s", s);
    }

    fclose(file);
    return 0;
}

int count_files(char* path) {

    DIR* d = opendir(path);
    struct dirent *dir;
    if (d) {
        int count = 0;
        while ((dir = readdir(d)) != NULL) {
            if (strcmp(dir->d_name, "..") != 0 && strcmp(dir->d_name, ".") != 0) {
                count++;
            }
        }
        closedir(d);
        return count;
    }
    return 0;
}

void get_filenames(char* strings[], char* path) {

    DIR* d;
    struct dirent *dir;
    d = opendir(path);
    if (d) {
        int i = 0;
        while ((dir = readdir(d)) != NULL) {
            if (strcmp(dir->d_name, "..") != 0 && strcmp(dir->d_name, ".") != 0) {
                char* s = malloc(strlen(PATH_TO_FILES) + strlen(dir->d_name));
                strcpy(s, dir->d_name);
                strings[i] = s;
                i++;
            }
        }
        closedir(d);
    }
}
