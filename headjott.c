#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <ftw.h>
#include <unistd.h>
#include <ctype.h>

#define MAX_LENGTH 32
#define OPTLIST "Options: [-H | -n | -h | -a | -D | -r | -p] (-H for help)"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

char* PATH_TO_FILES;
char* notebook;
char* header;
char* notebook_path;
char* header_path;

void set_path_to_files(char* program_name);
void help(char* program_name);
void usage(char* program_name);
int add(char* arg);
int create_notebook(char* foldername);
int create_header(char* filename);
int append_line(char* arg);
int folder_exists(char* path);
int delete();
int delete_header();
int delete_notebook();
int rename_item(char* arg);
int set_notebook(char* arg);
int set_header(char* arg);
void free_stuff();
int file_exists(char* path);
void print(int all);
void print_notebook(char* s, int all);
void print_headers(char* path, int size, int indent, int all);
int print_lines(char* path, int indent);
int count_files(char* path);
void get_filenames(char* strings[], char* path);
int yes_no();

int main(int argc, char *argv[]) {

    char* program_name = "headjott";
    char* add_arg = NULL;
    char* rename_arg = NULL;
    int Dflag, pflag, Hflag;
    int option_index = 0;
    int c, result;

    set_path_to_files(program_name);

    while (1) {
        static struct option long_options[] = {
            {"help",        no_argument,        0,  'H'},
            {"notebook",    required_argument,  0,  'n'},
            {"header",      required_argument,  0,  'h'},
            {"add",         required_argument,  0,  'a'},
            {"delete",      no_argument,        0,  'D'},
            {"rename",      required_argument,  0,  'r'},
            {"print",       no_argument,        0,  'p'},
            {0, 0, 0, 0}
        };

        c = getopt_long (argc, argv, "Hn:h:a:Dr:p", long_options, &option_index);

        if (c == -1)
            break;

        switch (c) {
            case 'H':
                Hflag = 1;
                break;
            case 'n':
                if (set_notebook(optarg) != 0) {
                    free_stuff();
                    exit(1);
                }
                break;
            case 'h':
                if (set_header(optarg) != 0) {
                    free_stuff();
                    exit(1);
                }
                break;
            case 'a':
                add_arg = optarg;
                break;
            case 'D':
                Dflag = 1;
                break;
            case 'r':
                rename_arg = optarg;
                break;
            case 'p':
                pflag = 1;
                break;
            case '?':
                usage(program_name);
                free_stuff();
                exit(1);
        }
    }

    if (Hflag == 1) {
        help(program_name);
        result = 0;
    } else if (add_arg != NULL) {
        result = add(add_arg);
    } else if (rename_arg != NULL) {
        result = rename_item(rename_arg);
    } else if (Dflag == 1) {
        result = delete();
    } else if (pflag == 1) {
        print(1);
        result = 0;
    } else {
        print(0);
        result = 0;
    }

    free_stuff();
    exit(0);
}

void set_path_to_files(char* program_name) {
    char* HOME = getenv("HOME");
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
        fprintf(stderr, "Error: varible HOME is null\n");
        exit(1);
    }
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

void help(char* program_name) {
    printf("\n%s is written in c by Njoter - stianovesen@gmail.com\n\n", program_name);
    printf("-n, --notebook\n");
    printf("\tTakes the name of a notebook as an argument.\n"
            "\tWhen set, the notebook can be deleted, added to, or its content can be printed with -p.\n"
            "\tSee -a, -d and -p for further explanation.\n");
    printf("-h, --header\n");
    printf("\tTakes the name of a header as an argument.\n"
            "\tMuch like the --notebook option, when a header is set, it can be deleted, added to, or its content can be printed with -p.\n"
            "\tA notebook MUST be set before a header can be set. See -n.\n");
    printf("-a, --add\n");
    printf("\tThis option adds a thing. What kind of thing is added is dependent on wether a notebook or a header within a notebook is set.\n"
            "\tIf a notebook is set (but not a header), the program will add a header within that notebook.\n"
            "\tIf a header is set (can only be set if a notebook is already set), a line will be added under that header.\n"
            "\tIf nothing is set, the program will add a new notebook. See --notebook and --header for more info.\n");
    printf("-d, --delete\n");
    printf("\tWork in progress.\n");
    printf("-p, --print\n");
    printf("\tWill print everything from the selected point.\n"
            "\tIf a notebook is set, the program will print everything within that notebook.\n"
            "\tIf a header is set, everything within that header will be printed.\n"
            "\tIf nothing is set, everything will be printed.\n");
}

void usage(char* program_name) {
    fprintf(stderr, "usage: %s [OPTION] [ARGUMENT]\n", program_name);
}

int delete() {
    if (header != NULL) {
        printf("Are you sure you want to delete %s? 'y' to accept.\n", header);
        if (yes_no() == 1) {
            return(delete_header());
        } else {
            printf("%s not deleted.\n", header);
            return 0;
        }
    } else if (notebook != NULL) {
        printf("Are you sure you want to delete %s? 'y' to accept.\n", notebook);
        if (yes_no() == 1) {
            return(delete_notebook());
        } else {
            printf("%s not deleted.\n", notebook);
            return 0;
        }
    } else {
        fprintf(stderr, "Please set a notebook or header to be deleted\n");
        return 1;
    }
}

int delete_header() {
    if ((remove(header_path)) == 0) {
        printf("%s deleted successfully\n", header);
        return 0;
    } else {
        perror("Unable to delete file\n");
        return 1;
    }
}

int delete_notebook() {
    DIR* dir = opendir(notebook_path);
    if (dir == NULL) {
        return 1;
    }

    struct dirent* entity;
    while((entity = readdir(dir)) != NULL) {
        if (strcmp("..", entity->d_name) != 0 && strcmp(".", entity->d_name) != 0) {
            set_header(entity->d_name);
            if (remove(header_path) == 0) {
                printf("%s deleted successfully\n", header);
            }
            else {
                fprintf(stderr, "Unable to delete %s\n", header);
                return 1;
            }
        }
    }

    closedir(dir);
    if (rmdir(notebook_path) == 0) {
        printf("%s deleted successfully\n", notebook);
    } else {
        perror("Unable to delete notebook\n");
    }
    return rmdir(notebook_path);
}

int rename_item(char* arg) {

    int result;
    if (strlen(arg) > MAX_LENGTH) {
        fprintf(stderr, "Argument can't be longer than %d characters\n", MAX_LENGTH);
        return 1;
    }

    if (header != NULL) {
        char new_path[strlen(notebook_path) + strlen(arg) + 1];
        strcpy(new_path, notebook_path);
        strcat(new_path, "/");
        strcat(new_path, arg);
        result = rename(header_path, new_path);
        if (result == 0) {
            printf("Successfully renamed %s to %s.\n", header, arg);
        }
    } else if (notebook != NULL) {
        char new_path[strlen(PATH_TO_FILES) + strlen(arg)];
        strcpy(new_path, PATH_TO_FILES);
        strcat(new_path, arg);
        result = rename(notebook_path, new_path);
        if (result == 0) {
            printf("Successfully renamed %s to %s.\n", notebook, arg);
        }
    }

    if (result != 0) {
        fprintf(stderr, "Failed to rename file.\n");
    }
    return result;
}

int add(char* arg) {

    int result;
    if (strlen(arg) > MAX_LENGTH) {
        fprintf(stderr, "Argument can't be longer than %d characters\n", MAX_LENGTH);
        return 1;
    }

    if (notebook == NULL) {
        result = create_notebook(arg);
    } else if (folder_exists(notebook_path)) {
        if (header == NULL) {
            result = create_header(arg);
        } else if (file_exists(header_path)) {
            result = append_line(arg);
        }
    }

    return result;
}

int create_notebook(char* arg) {

    char dir[strlen(PATH_TO_FILES) + strlen(arg)];
    strcpy(dir, PATH_TO_FILES);
    strcat(dir, arg);

    if (mkdir(dir, 0777) != 0) {
        perror("Error creating notebook\n");
        return 1;
    }

    printf("Successfully created notebook %s\n", arg);
    return 0;
}

int create_header(char* arg) {

    header = arg;
    char* s = malloc(strlen(notebook_path) + strlen(arg) + 1);
    strcpy(s, notebook_path);
    strcat(s, "/");
    strcat(s, arg);
    header_path = s;

    if (file_exists(header_path)) {
        fprintf(stderr, "Header %s already exists in %s\n", arg, notebook);
        return 1;
    }

    FILE* file = fopen(header_path, "w");
    if (file == NULL) {
        perror("Error opening file\n");
        return 1;
    }

    fclose(file);
    printf("Successfully created header %s\n", arg);
    return 0;
}

int append_line(char* arg) {

    FILE* file = fopen(header_path, "a");
    if (file == NULL) {
        perror("Error opening file\n");
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
        fprintf(stderr, "Error: can't find %s\n", path);
        return 0;
    } else {
        fprintf(stderr, "Error opening %s\n", path);
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

int set_notebook(char* arg) {

    int exists = 0;

    DIR* d = opendir(PATH_TO_FILES);
    struct dirent *dir;
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (strcmp(dir->d_name, arg) == 0) {
                exists = 1;
                break;
            }
        }
        closedir(d);
    }

    if (exists == 0) {
        printf("Error: notebook %s does not exist.\n", arg);
        return 1;
    }

    notebook = arg;
    char* s = malloc(strlen(PATH_TO_FILES) + strlen(arg));
    strcpy(s, PATH_TO_FILES);
    strcat(s, arg);
    notebook_path = s;
    return 0;
}

int set_header(char* arg) {

    if (notebook_path == NULL) {
        fprintf(stderr, "Error: notebook not set. Please set notebook before setting header.\n");
        return 1;
    }    

    int exists = 0;

    DIR* d = opendir(notebook_path);
    struct dirent *dir;
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (strcmp(dir->d_name, arg) == 0) {
                exists = 1;
                break;
            }
        }
        closedir(d);
    }

    if (exists == 0) {
        printf("Error: header %s does not exist.\n", arg);
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

    printf("\n");
    printf("%s\n", OPTLIST);
    printf(ANSI_COLOR_RED "(--notebook)");
    printf(ANSI_COLOR_CYAN "\t(--header)" ANSI_COLOR_RESET);
    printf("\n-----------------------------------------------------------\n");
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

    printf(ANSI_COLOR_RED "%s\n" ANSI_COLOR_RESET, s);
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
        printf(ANSI_COLOR_CYAN "# %s\n" ANSI_COLOR_RESET, arr[i]);
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
        perror("Error opening file\n");
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

int yes_no() {

    char input;
    scanf("%c", &input);
    input = tolower(input);
    if (input == 'y') {
        return 1;
    }
    return 0;
}
