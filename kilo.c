/*** includes ***/

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

/** defines **/
#define CTRL_KEY(k)                                                            \
  ((k)&0x1f) // bitwise-AND a character with 00011111
             // this replicates the behaviour of the ctrl-key

/*** data ***/

struct editorConfig {
  int screenrows;
  int screencols;
  struct termios orig_termios;
};

struct editorConfig E;

/** terminal **/

void die(const char *s) {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);
  perror(s);
  exit(1);
}

void disableRawMode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
    die("tcsetattr");
}

void enableRawMode() {
  if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1)
    die("tcgetattr");

  atexit(disableRawMode);

  struct termios raw = E.orig_termios;
  /**
   * BRKINT - When BRKINT is turned on, a break condition will cause a SIGINT
   *          signal to be sent to the program
   * ICRNL  - disable carriage return, newline
   * INPCK  - INPCK enables parity checking (doesn't apply to modern terminals).
   *          Traditionally done though for enabling raw mode
   * ISTRIP - causes the 8th bit of each input byte to be stripped, meaning it
   *          will set it to 0. Traditionally done though for enabling raw mode
   * IXON   - to pause transmission (legacy stuff)
   */
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST); // disable all output processing
  raw.c_cflag |= (CS8);    // sets the character size (CS) to 8 bits per byte
  /**
   * ICANON - allows to read byte by byte
   * ECHO   - turns off the echo after exiting the programo
   * IEXTEN - disables the ctrl-v synchronos idle
   * ISIG   - disable the ctrl-c control character
   */
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

  /**
   * Next two lines set a timeout for the read() function:
   *    VMIN  - minimum number of bytes of i/p needed before read() can return
   *    VTIME - maximum number of time to wait before read() returns
   */
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    die("tcsetattr"); // flushes input -> no longer feeds to the shell after
                      // ctrl-c
}

/**
 * Waits for keypresses and handles escape sequences
 */
char editorReadKey() {
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN)
      die("read");
  }

  return c;
}

int getWindowSize(int *rows, int *cols) {
  struct winsize ws;

  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    return -1;
  } else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

/** ouptut **/

void editorDrawRows() {
  int y;
  for (y = 0; y < E.screenrows; y++) {
    write(STDOUT_FILENO, "~\r\n", 3);
  }
}

void editorRefreshScreen() {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H",
        3); // reposition curesor to top left of corner the screen

  editorDrawRows();

  write(STDOUT_FILENO, "\x1b[H", 3);
}

/** input **/

/**
 * Maps keys to editor functions
 */
void editorProcessKeypress() {
  char c = editorReadKey();

  switch (c) {
  case CTRL_KEY('q'):
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    exit(0);
    break;
  }
}

/*** init ***/

void initEditor() {
  if(getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
}

int main() {
  enableRawMode();
  initEditor();

  while (1) {
    editorRefreshScreen();
    editorProcessKeypress();
  }

  return 0;
}
