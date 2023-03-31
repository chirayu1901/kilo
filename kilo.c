/*** includes ***/

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

/*** data ***/

struct termios orig_termios;

/** terminal **/

void die(const char *s) {
  perror(s);
  exit(1);
}

void disableRawMode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
    die("tcsetattr");
}

void enableRawMode() {
  if (tcgetattr(STDIN_FILENO, &orig_termios) == -1)
    die("tcgetattr");

  atexit(disableRawMode);

  struct termios raw = orig_termios;
  /**
   * BRKINT - When BRKINT is turned on, a break condition will cause a SIGINT
   *          signal to be sent to the program
   * ICRNL  - disable carriage return, newline
   * INPCK  - INPCK enables parity checking (doesn't apply to modern terminals).
   *          Traditionally done though for enabling raw mode
   * ISTRIP - causes the 8th bit of each input byte to be stripped, meaning it
   * will set it to 0. Traditionally done though for enabling raw mode IXON   -
   * to pause transmission (legacy stuff)
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
   * Next two lines set a timeout for the read() function
   * VMIN  - minimum number of bytes of i/p needed before read() can return
   * VTIME - maximum number of time to wait before read() returns
   */
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    die("tcsetattr"); // flushes input -> no longer feeds to the shell after
                      // ctrl-c
}

/*** init ***/

int main() {
  enableRawMode();

  while (1) {
    char c = '\0';
    if (read(STDIN_FILENO, &c, 1) == -1 &&
        errno != EAGAIN) // when read times out it return -1 w/ errno of EAGAIN
      die("read");

    if (iscntrl(c)) {
      // checking if the character is a control character
      printf("%d\r\n", c);
    } else {
      printf("%d ('%c')\r\n", c, c);
    }

    if (c == 'q')
      break;
  }

  return 0;
}
