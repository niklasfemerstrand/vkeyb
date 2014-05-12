#include <stdio.h>
#include <termios.h>
#include <string.h>
#include <poll.h>

#define MAX_OUTPUT_SIZE 1000
#define KBD_COLOR "\x1b[1;34;40m"
#define KBD_SEP KBD_COLOR "| \x1b[0m"
#define KEY_COLOR "\x1b[0m\x1b[2;31;40m"

char alphabet[256] = "~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:\"ZXCVBNM<>?`1234567890-=qwertyuiop[]\\asdfghjkl;'zxcvbnm,./";
char keys[256]; // modify size if you enlarge qwerty
char remapped[256]; // modify size if you enlarge qwerty
unsigned int shift_offset = 47;
struct pollfd fds[1];
FILE *randomfp ;

unsigned char rand_unbiased_modulo(unsigned char amount_of_elements){
  // discards random chars above the treshold so we can cleanly do % on a random number
  unsigned char r = getc(randomfp);
  if(256 % amount_of_elements != 0)
    while(r < amount_of_elements)
      r = getc(randomfp);
  return r % amount_of_elements;// gets a number between 0 and amount_of_elements - 1
}

unsigned char get_char(unsigned char *buf) {
//   http://stackoverflow.com/questions/421860/c-c-capture-characters-from-standard-input-without-waiting-for-enter-to-be-pr
//   stolen from
  struct termios old = {0};
  tcgetattr(0, &old);
  old.c_lflag &= ~ICANON;
  old.c_lflag &= ~ECHO;
  old.c_cc[VMIN] = 1;
  old.c_cc[VTIME] = 0;
  tcsetattr(0, TCSANOW, &old);
  read(0, buf, 1);
  short lol=0;
  unsigned char retval=0;
  if(0x7f == *buf)
    retval = 1;
  else if(0xa  == *buf)
    retval = 2;
  else while((lol = poll(fds, 1, 1 /*1 ms timeout*/)) && lol & POLLIN ){
    read(0,buf,1);
    retval = 3;
  }
  old.c_lflag |= ICANON;
  old.c_lflag |= ECHO;
  tcsetattr(0, TCSADRAIN, &old);
  return retval;
}

void print_keyboard(){
  fprintf(stderr, "%s%s\x1b[0m", KBD_COLOR, " ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ _____\n");
  char ia;
  for(ia=0;ia<13;ia++)
    fprintf(stderr, "%s%s\x1b[4m%c\x1b[0m%s%c", KBD_SEP, KEY_COLOR, keys[ia], KEY_COLOR, keys[ia+shift_offset]);
  fprintf(stderr, "%s DEL %s\n", KBD_SEP, KBD_SEP);
  fprintf(stderr, "%sTAB%s", KBD_SEP, KBD_SEP);
  for(ia=0;ia<13;ia++){
    fprintf(stderr, "%s\x1b[4m%c\x1b[0m%s%c%s", KEY_COLOR, keys[ia+13], KEY_COLOR, keys[ia+13+shift_offset], KBD_SEP);
  }
  fprintf(stderr, "\n%sCAPS%s", KBD_SEP, KBD_SEP);
  for(ia=0;ia<11;ia++){
    fprintf(stderr, "%s\x1b[4m%c\x1b[0m%s%c%s", KEY_COLOR, keys[ia+26],KEY_COLOR, keys[ia+26+shift_offset], KBD_SEP);
  } 
  fprintf(stderr, " ENTER %s\n%sSHIFT %s", KBD_SEP, KBD_SEP, KBD_SEP);
  for(ia=0;ia<10;ia++){
    fprintf(stderr, "%s\x1b[4m%c\x1b[0m%s%c%s", KEY_COLOR, keys[ia+37], KEY_COLOR, keys[ia+37+shift_offset],KBD_SEP);
  }
  fprintf(stderr, "SHIFT%s\n", KBD_SEP);
}

void print_table(){
  unsigned char c;
  // struct winsize{short ws_row;short ws_col;short ws_xpixel;short ws_ypixel;} window size tty_ioctl(stdout, TIOCGWINSZ);
  fprintf(stderr, "\x1b[2J\x1b[H\x1b[2;37;40m"); // erase screen, move to home, color dark gray on black (try to prevent shoulder surfing the best we can)

  // do the Knuth shuffle, he's our homie after all
  keys[0] = alphabet[0];
  for(c=1;c<strlen(alphabet);c++){ // reference implementation, https://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle
    unsigned char j = rand_unbiased_modulo(c); 
    keys[c] = keys[j];
    keys[j] = alphabet[c];
  }
  print_keyboard();

  for(c=32;c<255;c++)
    remapped[c] = c;
  for(c=0;c<strlen(alphabet);c++)
    remapped[keys[c]] = alphabet[c];
}

int main(int argc, char **argv){
  unsigned char c;
  unsigned char output[MAX_OUTPUT_SIZE+1]="";
  randomfp = fopen("/dev/urandom", "r");
  fds[0].fd = 0;
  fds[0].events=POLLIN;

while(1){ 
  print_table();
  switch(get_char(&c)){
    case 1: // backspace
      output[abs(strlen(output)-1)] = 0;break;
    case 2: // Enter/return received - we're done
      printf("%s", output);return(0);break;
    case 3:  // control char/multibyte input (paste for instance) - if not backspace, ignore
      break;
    default: // maximum output is hardcoded
      if(strlen(output)<=MAX_OUTPUT_SIZE){
        output[strlen(output)+1] = 0;
        output[strlen(output)]   = remapped[c];
      }
      break;
  }
// TODO add DEBUG flag fprintf(stderr, "lol:  [%2x] = %c [%2x]\nOutput [%d]: %s\n", c, remapped[c], remapped[c], strlen(output), output);
}
}
