#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/resource.h>

#undef  DEBUG

#define MAX_BUFFER 2000000

static char *INFINITY = "~";

static int    T;
static int   *H;
static char **V;

static inline int mycmp(char *l, char *r)
{ int a, b;

  do
    { a = *l++;
      b = *r++;
      if (a != b)
        return (a-b);
    }
  while (a != 0);
  return (0);
}

#ifdef DEBUG

static void print_heap(int i, int d)
{ printf("%*s%3d: %s %d\n",2*d,"",i,V[H[i]],H[i]);
  if (2*i <= T)
    print_heap(2*i,d+1);
  if (2*i+1 <= T)
    print_heap(2*i+1,d+1);
}

#endif

static void Heapify(int i, char *x, int t)
{ int       l, c;
  int       hl;
  char     *vl;
  
  c = i;
  while ((l = (2*c)) <= T)
    { hl = H[l];
      vl = V[hl];
      if (l < T)
        { if (mycmp(V[H[l+1]],vl) < 0)
            { l += 1;
              hl = H[l];
              vl = V[hl];
            }
        }
      if (mycmp(x,vl) <= 0)
        break;
      H[c] = hl;
      c    = l;
    }
  H[c] = t;
  V[t] = x;
}

static int   *IOU;
static char **Buf;
static char **Ptr;
static int   *Len;

char *Pop(int t)
{ char *v, *x;
  int   n;

  //  V[t] points at the previous string pulled from this input
  //  If the buffer reaches the end, then one keeps not just the current
  //    entry, but also the previous one, and V[t] is updated to point
  //    at that previous entry

  for (v = x = Ptr[t]; *x != '\n'; x++)
    if (*x == '\0')
      { n = x-V[t];
        bcopy(V[t],Buf[t],n);
#ifdef DEBUG
        printf("   Buffer %d: moving %d,",t,n);
#endif
        x = Buf[t] + n;
        v = Buf[t] + Len[t] + 1;
        V[t] = Buf[t];
        n = MAX_BUFFER - (n+1);
        if (n <= 0)
          { fprintf(stdout,"2 consecutive strings longer than %d\n",MAX_BUFFER);
            exit (1);
          }
#ifdef DEBUG
        printf(" reading %d,",n);
#endif
        n = read(IOU[t],x,n);
        x[n] = 0;
#ifdef DEBUG
        printf(" read %d(%ld) [%d]\n",n,(x+n)-Buf[t],n == 0);
#endif
        if (n == 0)
          return (NULL);
        x -= 1;
      }

  Len[t] = x-v;
  *x++   = '\0';
  Ptr[t] = x;

#ifdef DEBUG
  printf("Pop %d: %s (%d) [%ld]\n",t,v,Len[t],Ptr[t]-Buf[t]);
#endif

  return (v);
}

void string_merge()
{ int   O    = IOU[0];
  char *Obuf = Buf[0];
  char *Optr = Ptr[0];
  char *Oend = Optr + MAX_BUFFER;

  int   t;
  char *x;
  char *last;

  for (t = T; t >= 1; t--)
    { x = Pop(t);
      if (x == NULL)
        Heapify(t,INFINITY,0);
      else
        Heapify(t,x,t);
    }
#ifdef DEBUG
  print_heap(1,0);
#endif

  last = INFINITY;
  while ((t = H[1]) > 0)
    { x = V[t];
#ifdef DEBUG
      printf("Out %d: %s\n",t,x);
#endif
      if (mycmp(x,last) != 0)
        { int len = Len[t];
          if (Optr + len > Oend)
            { write(O,Obuf,Optr-Obuf);
              Optr = Obuf;
            }
          memcpy(Optr,x,len);
          Optr += len;
          *Optr++ = '\n';
        }
      x = Pop(t);
      last = V[t];
      if (x == NULL)
        Heapify(1,INFINITY,0);
      else
        Heapify(1,x,t);
#ifdef DEBUG
      print_heap(1,0);
#endif
    }

  if (Optr > Obuf)
    write(O,Obuf,Optr-Obuf);
}


static struct rusage   Mtime;

static void startTime()
{ getrusage(RUSAGE_SELF,&Mtime);
}

static double timeTo()
{ struct rusage    now;
  struct rusage   *t;
  int usecs, umics;
  int ssecs, smics;

  getrusage(RUSAGE_SELF,&now);
  t = &Mtime;

  usecs = now.ru_utime.tv_sec  - t->ru_utime.tv_sec;
  umics = now.ru_utime.tv_usec - t->ru_utime.tv_usec;
  if (umics < 0)
    { umics += 1000000;
      usecs -= 1;
    }

  ssecs = now.ru_stime.tv_sec  - t->ru_stime.tv_sec;
  smics = now.ru_stime.tv_usec - t->ru_stime.tv_usec;
  if (smics < 0)
    { smics += 1000000;
      ssecs -= 1;
    }

  Mtime = now;

  return (usecs + ssecs + (umics+smics)/1000000.);
}

int main(int argc, char *argv[])
{ int r, t, R;

  if (argc < 4)
    { fprintf(stderr,"Usage: Heap <R:int> <out:file> <in:file> ...\n");
      exit (1);
    }

  R = atoi(argv[1]);

  T = argc-3;

  H = malloc(sizeof(int)*(T+1));
  V = malloc(sizeof(char *)*(T+1));
  if (H == NULL || V == NULL)
    { fprintf(stderr,"merge out of memory\n");
      exit (1);
    }

  IOU = (int *) malloc(sizeof(int)*(T+1));
  Buf = (char **) malloc(sizeof(char *)*(T+1));
  Ptr = (char **) malloc(sizeof(char *)*(T+1));
  Len = (int *) malloc(sizeof(int)*(T+1));
  if (IOU == NULL || Buf == NULL || Ptr == NULL || Len == NULL)
    { fprintf(stderr,"merge out of memory\n");
      exit (1);
    }

  { struct rlimit rlp;

    getrlimit(RLIMIT_NOFILE,&rlp);
    if (T+5llu > rlp.rlim_max)
      { fprintf(stderr,"\ncannot open %d files simultaneously\n",T+5);
        exit (1);
      }
    rlp.rlim_cur = T+5;
    setrlimit(RLIMIT_NOFILE,&rlp);
  }

  for (t = 0; t <= T; t++)
    { Buf[t] = malloc(MAX_BUFFER);
      if (Buf[t] == NULL)
        { fprintf(stderr,"merge out of memory\n");
          exit (1);
        }
    }

  startTime();
  for (r = 0; r < R; r++)
    { for (t = 1; t <= T; t++)
        { Ptr[t] = V[t] = Buf[t]+(MAX_BUFFER-1);
          *(Ptr[t]) = 0;
          V[t] = Ptr[t]-1;
          *(V[t]) = 0;
          H[t] = t;
          Len[t] = 0;
          IOU[t] = open(argv[t+2],O_RDONLY);
        }

      Ptr[0] = Buf[0];
      IOU[0] = open(argv[2],O_WRONLY|O_CREAT|O_TRUNC,S_IRWXU);

      string_merge();

      close(IOU[0]);
      for (t = 1; t <= T; t++)
        close(IOU[t]);
    }
  printf(" & %.4g",timeTo()/R);
  fflush(stdout);

  exit (0);
}
