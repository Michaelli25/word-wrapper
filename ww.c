#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#include "ww.h"
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#define BUFFER 256

unsigned width;
int fail = 0, begin = 0, lineLength = 0, words = 0;


int init(strbuf* sb) {
    sb->data = malloc(sizeof(char) * (BUFFER + 1));
    if(!sb->data) return 1; // error

    sb->length = BUFFER;
    sb->used = 0;
    sb->data[0] = '\0';
    return 0;
}


int append(strbuf* sb, char item) {
    if(sb->used == sb->length) {
        size_t size = sb->length * 2; // double buffer if more space needed
        char* p = realloc(sb->data, sizeof(char) * (size + 1));
        if(!p) return 1;
        sb->data = p;
        sb->length = size;
    }

    sb->data[sb->used] = item;
    ++sb->used;
    sb->data[sb->used] = '\0';
    return 0;
}


void wrap(strbuf* sb, int input, int output) {
    char buf[BUFFER];
    int bytes;
    int newLine = 0;

    while((bytes = read(input, buf, BUFFER)) > 0) {
        // BUFFER size has been written to buf
        // process buffer
        for(int i = 0; i < bytes; i++) {
            char c = buf[i];
            // check if next character is a space
            if(isspace(c)) {
                // check for second \n
                if(c == '\n') 
                    newLine++;

                printer(sb, output);
            }
            else {
                // check for new lines & write a new paragraph
                if(newLine >= 2 && begin) {
                    write(output, "\n\n", 2);
                    words = 0;
                    lineLength = 0;
                }
                newLine = 0;
                // add character to token array
                append(sb, c);
            }
        }
    }
    // print last token if needed
    printer(sb, output);
    write(output, "\n", 1);
    lineLength = 0, words = 0;

}


void printer(strbuf* sb, int out) {
    // word is longer than width write on a line by itself, FAIL
    if(sb->used > width) {
        if(words)
            write(out, "\n", 1);
        write(out, sb->data, sb->used);
        words = 1;
        lineLength = sb->used;
        fail = 1;
        begin = 1;
    }
    // check if there is a token
    else if(sb->used != 0) {
        // if atleast 1 word on a line & if (token length + lineLength + 1) is <= width then print
        if(words && ((sb->used + lineLength + 1) <= width)) {
            write(out, " ", 1);
            write(out, sb->data, sb->used);
            lineLength += sb->used + 1;
            words = 1;
        }
        // if no words on a line & if token length is <= width then print
        else if(!words && (sb->used <= width)) {
            write(out, sb->data, sb->used);
            lineLength += sb->used;
            words = 1;
        }
        // word is too big to fit on this line, push it to the next line
        else {
            write(out, "\n", 1);
            write(out, sb->data, sb->used);
            lineLength = sb->used;
        }
        begin = 1;
    }
    sb->used = 0;
}

int main(int argc, char* argv[argc+1]) {
    if(argc < 2 || argc > 3)
        return EXIT_FAILURE;
    width = atoi(argv[1]);
    int fd;
    int fd1;
    struct stat statbuf;
    struct dirent *de;
    DIR *dirp;
    stat(argv[2], &statbuf);
    // if the argument is a file
    if(S_ISREG(statbuf.st_mode)){
        if(argc == 3)
            fd = open(argv[2], O_RDONLY);
        else
            fd = 0;
        strbuf token;
        init(&token);
        wrap(&token, fd, 1);
        close(fd);
        free(token.data);
    }
    // if the argument is a directory
    else if(S_ISDIR(statbuf.st_mode)){
        dirp = opendir(argv[2]);
        chdir(argv[2]);
        // if directory is empty
        if(dirp == NULL){
            return EXIT_FAILURE;
        }
        // loops through the directory
        while((de = readdir(dirp)) != 0){
            // ignores specified files
            stat(de->d_name, &statbuf);
            if(S_ISDIR(statbuf.st_mode) || strncmp(de->d_name, "wrap.", 5) == 0)
                continue;
            // file path
            fd = open(de->d_name, O_RDONLY);
            // destination path
            char* path = malloc(sizeof(char) * (strlen(de->d_name) + 6));
            strcpy(path, "wrap.");
            strcat(path, de->d_name);
            // creates the new file
            fd1 = open(path, O_WRONLY | O_TRUNC | O_CREAT, 0666);
            strbuf token2;
            init(&token2);
            wrap(&token2, fd, fd1);
            close(fd);
            close(fd1);
            free(path);
            free(token2.data);
        }
        closedir(dirp);
    }
    if(fail)
        return EXIT_FAILURE;
    else
        return EXIT_SUCCESS;
}
