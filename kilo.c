/* INCLUDES  */
/* Feature test macros for getline() to make code portable */
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <ctype.h>   /* iscntrl() */
#include <errno.h>   /* errno, EAGAIN */
#include <stdio.h>   /* printf(), perror() */
#include <stdlib.h>  /* atexit(), exit() */
#include <string.h>  /* memcpy() */
#include <sys/ioctl.h>
#include <sys/types.h> /* ssize_t */
#include <termios.h> /* struct termios, tcgetattr(), tcsetattr, ECHO, TCSAFLUSH, ISIG, IXON, IEXTEN, ICRNL, OPOST */
#include <unistd.h>  /* read(), STDIN_FILENO */

/* DEFINES */
#define KILO_VERSION "0.0.1"

#define CTRL_KEY(k) ((k) & 0x1f)

enum editorKey{
    ARROW_LEFT = 1000, 
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY,
    HOME_KEY, 
    END_KEY,
    PAGE_UP,
    PAGE_DOWN
};

/* DATA */
typedef struct erow {
    int size;
    char * chars;
} erow;

struct editorConfig {
    /* global variables for cursor positioning / termianl state */
    int cx, cy;
    int rowoff;
    int coloff;
    int screenrows;
    int screencols;
    int numrows;
    erow * row;
    struct termios original_termios;
};
struct editorConfig E;


/* TERMINAL */
void die(const char * s) {
    // Clear the screen and reposition cursor
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    perror(s);
    exit(1);
}

void disableRawMode(){
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.original_termios) == -1){
        die("tcsetattr");
    }
}

void enableRawMode(){
    /* Switching terminal from canonical  mode to raw mode.
     * We get termianl attributes using tcgetarrt() and store them
     * in the struct terminos. We then flip the ECHO bit to 0.
     * We then store the modified sturct back in memory. 
     * */
    if (tcgetattr(STDIN_FILENO, &E.original_termios) == -1){
        die("tcgetattr");
    }
    atexit(disableRawMode);

    struct termios raw = E.original_termios;
    raw.c_iflag &= ~(BRKINT | INPCK | ISTRIP | ICRNL | IXON);
    raw.c_iflag &= ~(OPOST);
    /* Set 8 bits per byte, if not already. */
    raw.c_iflag &= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0; /* number of bytes before read returns */ 
    raw.c_cc[VTIME] = 1; /* seconds before timeout (1/10 of a second) */
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1){
        die("tcsetattr");
    }
}

int editorReadKey(){
    int nread;
    char c;
    while ( (nread = read(STDIN_FILENO, &c, 1)) != 1 ){
        if (nread == -1 && errno != EAGAIN) {
            die("read");
        }
    }
    const char esc = '\x1b';
    if (c == esc){
        char seq[3];
        if ( read(STDIN_FILENO, &seq[0], 1) != 1 ){
            return esc;
        }
        if ( read(STDIN_FILENO, &seq[1], 1) != 1 ){
            return esc;
        }
        if (seq[0] == '['){
            if (seq[1] >= '0' && seq[1] <= '9'){
                if (read(STDIN_FILENO, &seq[2], 1) != 1){
                    return esc;
                }
                if (seq[2] == '~'){
                    switch(seq[1]){
                        case '1': return HOME_KEY;
                        case '3': return DEL_KEY;
                        case '4': return END_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                        case '7': return HOME_KEY;
                        case '8': return END_KEY;
                    }
                }
            }
            else{
                switch(seq[1]){
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'H': return HOME_KEY;
                    case 'F': return END_KEY;
                } 
            }
        }
        else if (seq[0] == 'O'){
            switch (seq[1]){
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
            }

        }
        return esc;
    }
    else{
        return c;
    }
}

int getCursorPosition(int * rows, int * cols){
    char buf[32];
    unsigned int i = 0;
    if ( write(STDOUT_FILENO, "\x1b[6n", 4) != 4 ){
        return -1;
    } 
    while ( i < sizeof(buf) - 1 ){
        if ( read(STDIN_FILENO, &buf[i], 1) != 1  ){
            break;
        }
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0'; 
    if (buf[0] != '\x1b' || buf[1] != '['){
        return -1;
    }
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) {
        return -1;
    }
    return 0;
}

int getWindowSize(int * rows, int * cols){
    struct winsize ws;
    /* If we cannot find the screen dimensions using these functions, find it manually. */
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
        if ( write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12 ) return -1;
        return getCursorPosition(rows, cols); 
    }
    else{
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;    
    }
}

/* ROW OPERATIONS */

void editorAppendRow(char * s, size_t len){
    E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
                
    int at = E.numrows;
    E.row[at].size = len;
    E.row[at].chars = malloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0';
    E.numrows ++;
}

/* FILE I/O */
void editorOpen(char * filename){
    FILE * fp = fopen(filename, "r");
    if (fp == NULL){
        die("fopen");
    }
    char * line = NULL;
    size_t linecap = 0;
    ssize_t linelen;

    while ((linelen = getline(&line,&linecap, fp)) != -1){

        while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')){
            linelen--;
        }
        editorAppendRow(line, linelen);
    }
    free(line);
    fclose(fp);

}

/* APPEND BUFFER */
struct abuf{
    char * b;
    int len;
};

#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf * ab, const char * s, int len){
    char * new = realloc(ab->b, ab->len + len);

    if (new == NULL){
        return;
    }
    memcpy(&new[ab->len], s, len);
    ab->b = new;
    ab->len += len;
}

void abFree(struct abuf * ab){
    free(ab->b);
}

/* OUTPUT  */
void editorScroll(){
    if (E.cy < E.rowoff){
        E.rowoff = E.cy;
    }
    
    if (E.cy >= E.rowoff + E.screenrows){
        E.rowoff = E.cy - E.screenrows + 1;
    }

    if (E.cx < E.coloff){
        E.coloff = E.cx;
    }
    if (E.cx >= E.coloff + E.screencols){
        E.coloff = E.cx - E.screencols + 1;
    }
}


void editorDrawRows(struct abuf * ab){
    for (int y = 0; y < E.screenrows; y++){
        int filerow = y + E.rowoff;
        if (y >= E.numrows){
            /* If writing rows with no content, print ~ or terminal name */
            if (E.numrows == 0 && y == E.screenrows / 4){
                char welcome[80];
                int welcomelen = snprintf(welcome, sizeof(welcome),
                        "KILO EDITOR - Version %s", KILO_VERSION);
                if (welcomelen > E.screencols){
                    welcomelen = E.screencols;
                }
                int padding = (E.screencols - welcomelen) / 2;
                if (padding){
                    abAppend(ab, "~", 1);
                    padding--;
                }
                while (padding){
                    abAppend(ab, " ", 1);
                    padding--;
                }
                abAppend(ab, welcome, welcomelen); 
            }
            else{
                abAppend(ab, "~", 1);
            }
        }
        /* Printing file contents */
        else{
            int len = E.row[filerow].size - E.coloff;
            if (len < 0) len = 0;
            if (len >= E.screencols) len = E.screencols;
            abAppend(ab, &E.row[filerow].chars[E.coloff], len);
        }
        /* Clear contents of line after cursor */
        abAppend(ab, "\x1b[K", 3);
        if (y < E.screenrows - 1) {
            abAppend(ab, "\r\n", 2);
        }
    }
}

void editorRefreshScreen(){
    editorScroll();
    struct abuf ab = ABUF_INIT;

    abAppend(&ab, "\x1b[?25l", 6);
    abAppend(&ab, "\x1b[H", 3);

    editorDrawRows(&ab);

    char buffer[32];
    /* H command moves the cursor to specified location */
    snprintf(buffer, sizeof(buffer), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1, (E.cx - E.coloff)+ 1); 
    abAppend(&ab, buffer, strlen(buffer));
    abAppend(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}


/* INPUT */
void editorMoveCursor(int key){
    erow * row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
    switch (key){
        case ARROW_LEFT:
            if (E.cx != 0){
                E.cx--;
            }
            else if (E.cy > 0){
                E.cy--;
                E.cx = E.row[E.cy].size;
            }
            break;
        case ARROW_RIGHT:
            if (row && E.cx < row->size){
                E.cx++;
            }
            else if (row && E.cx == row->size){
                E.cy++;
                E.cx = 0;
            }
            break;
        case ARROW_UP:
            if (E.cy != 0){
                E.cy--;
            }
            break;
        case ARROW_DOWN:
            if (E.cy < E.numrows){ 
                 E.cy++;
            }
            break;
    }
    row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
    int rowlen = row ? row->size : 0;
    if (E.cx > rowlen){
        E.cx = rowlen;
    }
}


void editorProcessKeypress(){
    int c  = editorReadKey();

    switch(c){
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;

        case HOME_KEY:
            E.cx = 0;
            break;
        case END_KEY:
            E.cx = E.screencols - 1;
            break;

        case PAGE_UP:
        case PAGE_DOWN:
            {
                int times = E.screenrows;
                while (times){
                    editorMoveCursor( (c == PAGE_UP) ? ARROW_UP : ARROW_DOWN );
                    times--;

                }
            }
            break;

        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
            editorMoveCursor(c);
            break;
    }
}

/* INIT */

void initEditor(){
    E.cx = 0;
    E.cy = 0;
    E.rowoff = 0;
    E.coloff = 0;
    E.numrows = 0;
    E.row = NULL;
    
    if ( getWindowSize(&E.screenrows, &E.screencols) == -1 ){
        die("getWindowSize");
    } 
}

int main(int argc, char * argv[]){
    enableRawMode();
    initEditor(); 
    if (argc >= 2){
        editorOpen(argv[1]);
    }
    while (1){  
        editorRefreshScreen();
        editorProcessKeypress();
    }
    return 0;
}
