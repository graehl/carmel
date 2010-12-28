/*
  catn: copy stdin to stdout - must be blocking FDs.
  minimal library usage so as to fork/exec faster. build: gcc -O3 -std=c99 or g++ -O3

  optional arg1=N: exit after N bytes (N < 2^64). 0 = no limit.
  optional arg2=k: k=0 (default) exit w/ no error on stdin EOF or N bytes. k=1 exit w/ error code 2 on eof w/ less than N bytes read.
  optional arg3=s: timeout of s seconds (0=default means no timeout) - if no data received in last s seconds, exit with error code 3
  optional arg4=B: buffer size of B bytes

*/

#include <err.h> // err
#include <signal.h> // sigaction
#include <unistd.h> // read,write,alarm
#include <limits.h> // UINT_MAX
#include <stdlib.h> //strtoull
#include <errno.h>

typedef unsigned long long cat_size_t;

const cat_size_t default_bufsz=256*1024;
const cat_size_t max_bufsz=512*1024*1024; // 512MB buffer (huge) - keep it small enough for stack

unsigned timeout_handler_sec;
void timeout_handler(int signum) {
  errx(3,"catn: timed out after reading no data for %u sec",timeout_handler_sec);
}

// cp rfd->wfd (blocking), up to the first max bytes. return number of bytes written
cat_size_t cat_fd_n(int rfd,int wfd,cat_size_t max,cat_size_t bufsz,unsigned timeout_sec) {
  char buf[bufsz];
  cat_size_t totalw=0;
  ssize_t nr,nw;

  struct sigaction act,oldact;
  timeout_handler_sec=timeout_sec;
  if (timeout_sec) {
    act.sa_handler=timeout_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags=0;
    sigaction(SIGALRM,&act,&oldact);
  }

  for(;;) {
    if (max && totalw+bufsz>max) bufsz=max-totalw; // never read or write more than max if set.
    if (timeout_sec) alarm(timeout_sec);
    nr=read(rfd,buf,bufsz);
    if (timeout_sec) alarm(0);
    if (nr==-1) err(4,"cat_fd_n failed read from fd %d",rfd);
    if (nr==0) break; //EOF
    totalw+=nr;
    char *bufend=buf+nr;
    for (;nr;nr-=nw)
      if ((nw=write(wfd,bufend-nr,nr))==-1 || nw==0)
        err(5,"cat_fd_n failed write to fd %d",wfd);
    if (max && totalw>=max) break;
  }
  if (timeout_sec) sigaction(SIGALRM,&oldact,0); // restore old handler
  return totalw;
}

// err unless whole string is used (no whitespace allowed)
unsigned long long ull_or_die(char const* c,int argi,unsigned long long max,char const* name) {
  char *end;
  errno=0;
  unsigned long long i=strtoull(c,&end,0);
  char const *errmsg="arg %d - wanted unsigned %s but got '%s'";
  if (*end || !*c) errx(1,errmsg,argi,name,c);
  if (errno) err(1,errmsg,argi,name,c); // should just be ERANGE in C99
  if (max && i>max) errx(1,"arg %d (%s) too large - max allowed is %llu",argi,name,max);
  return i;
}

int main(int argc, char *argv[]) {
  cat_size_t max=0;
  cat_size_t bufsz=default_bufsz;
  int err_incomplete=0;
  unsigned timeout_sec=0;

  //arg parsing:
  int ai=0;
  if (++ai<argc) {
    max=ull_or_die(argv[ai],ai,0,"max-length");
    if (max==0) return 0; // else we'll use max==0 to indicate no limit
  }
  if (++ai<argc)
    if (argv[ai][0]=='1') err_incomplete=1;
  if (++ai<argc)
    timeout_sec=ull_or_die(argv[ai],ai,UINT_MAX,"timeout-sec");
  if (++ai<argc)
    bufsz=ull_or_die(argv[ai],ai,max_bufsz,"buffer-size");

  //action:
  cat_size_t nout=cat_fd_n(STDIN_FILENO,STDOUT_FILENO,max,bufsz,timeout_sec);
  if (err_incomplete && nout<max)
    return 2;

  return 0;
}

