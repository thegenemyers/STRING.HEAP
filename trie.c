#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/resource.h>

#undef DEBUG
#undef READ_DNA_DATA

#define MAX_BUFFER 2000000

#ifdef REAL_DNA_DATA

#define SIGMA 4

static char Map[SIGMA] = { 'a', 'c', 'g', 't' };

static int IMap[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1,  0, -1,  1, -1, -1, -1,  2, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1,  3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1,  0, -1,  1, -1, -1, -1,  2, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1,  3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

#else

#define SIGMA 16     //  This could be better tuned to the specific input character set
                     //    but in our trials made a negligible difference when the alphabet
                     //    was smaller.

static char Map[SIGMA] = {
     '0', '1', '2', '3', '4', '5', '6', '7',
     '8', '9', ':', ';', '<', '=', '>', '?' };

static int IMap[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1,  3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1,  3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

#endif

static int T;

typedef struct _Node
  { void *e[SIGMA];
    char *s;
    int   c, m, d;
  } Node;

static int *List;

static Node *FreeList;
static Node *Root;

Node *New(char *tile)
{ Node *x;

  x = FreeList;
  FreeList = x->e[0];
  x->m = SIGMA;
  x->c = 0;
  x->d = 0;
  strcpy(x->s,tile);
  x->e[0] = NULL;
  return (x);
}

void Free(Node *x)
{ x->e[0] = FreeList;
  FreeList = x;
}

#ifdef DEBUG

void print_trie(int lev, Node *v)
{ int a, t;
 
  for (t = v->c; t > 0; t = List[t])
    printf(" %d+",t);
  printf(" %c(%d)\n",Map[v->m],v->d);
  for (a = 0; a < SIGMA; a++)
    if (v->e[a] != NULL)
      { printf("%*s->%c'%s: ",2*lev,"",Map[a],((Node *) v->e[a])->s);
        print_trie(lev+1,v->e[a]);
      }
}

void Print_Trie()
{ printf(" *: ");
  print_trie(0,Root);
  fflush(stdout);
}

#endif

void Setup_Trie(int N, int K)
{ int   i, a;
  char *heap;

  FreeList = malloc(sizeof(Node)*N);
  heap     = malloc(N*(K+1));
  if (FreeList == NULL)
    { fprintf(stderr,"Could not allocate free space\n");
      exit (1);
    }
  for (i = 0; i < N; i++)
    { FreeList[i].e[0] = FreeList+(i+1);
      FreeList[i].s    = heap + (K+1)*i;
      for (a = 1; a < SIGMA; a++)
        FreeList[i].e[a] = NULL;
    }
  FreeList[N-1].e[0] = NULL;

  Root = New("");
}

//  Follow segments until diverge, usually need to split a segment
//  All S letters scanned or copied. Split part also copied

void Add_To_Trie(char *x, int t)
{ Node *v, *e;
  char *y;
  int   a, s;

  v = Root; 
  while ((a = *x++) != 0)
    { s = IMap[a];
      e = v->e[s];
      if (e == NULL)
        { if (s < v->m)
            v->m = s;
          v->d += 1;
          v = v->e[s] = New(x);
          break;
        }
      for (y = e->s; *y != 0; y++, x++)
        if (*x != *y)
          break;
      if (*y == 0)
        v = e;
      else
        { v = v->e[s] = New(y+1);
          s = IMap[(int) (*y)];
          *y = 0;
          v->m = s;
          v->d = 1;
          v->e[s] = e;
          y = v->s;
          v->s = e->s;
          e->s = y;
        }
    }
  List[t] = v->c;
  v->c = t;
}

//  Min is just node traversals (at most T)
//  Interior final or deg >= 2 node, del min find next min edge
//  If collapsable then collapse, copy segment

int Extract_Min()
{ int a, t;
  char *y;
  Node *v, *e, *u;
  Node *p, *q;

  u = Root;
  if (u->d == 0)
    return (0);

  p = NULL;
  for (v = Root; v->c == 0; v = e)
    { e = v->e[v->m];
      if (v->d > 1)
        { u = v;
          q = p;
        }
      p = v;
    }

  t = v->c;
  a = v->m;
  if (a < SIGMA)
    { v->c = 0;
      q = p;
    }
  else
    { v = u;
      a = v->m;
      u = v->e[a];
      v->e[a] = NULL;
      for (a++; a < SIGMA; a++)
        if (v->e[a] != NULL)
          break;
      v->m = a;
      v->d -= 1;
      Free(u);
    }
  if (v->d == 1 && q != NULL)
    { e = v->e[v->m];
      v->e[v->m] = NULL;
      q->e[q->m] = e;
      y = index(v->s,0);
      *y++ = Map[v->m];
      strcpy(y,e->s);
      y = v->s;
      v->s = e->s;
      e->s = y;
      Free(v);
    }
  return (t);
}

static int   *IOU;
static char **Buf;
static char **Ptr;
static char **Nxt;
static int   *Len;

char *Pop(int t)
{ char *v, *x;
  int   n;

  //  Ptr[t] points at the previous string pulled from this input
  //  If the buffer reaches the end, then one keeps not just the current
  //    entry, but also the previous one, and Ptr[t] is updated to point
  //    at that previous entry

  for (v = x = Nxt[t]; *x != '\n'; x++)
    if (*x == '\0')
      { n = x-Ptr[t];
        bcopy(Ptr[t],Buf[t],n);
#ifdef DEBUG
        printf("   Buffer %d: moving %d,",t,n);
#endif
        x = Buf[t] + n;
        v = Buf[t] + Len[t] + 1;
        Ptr[t] = Buf[t];
        Nxt[t] = v;
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
  Ptr[t] = Nxt[t];
  Nxt[t] = x;

#ifdef DEBUG
  printf("Pop %d: %s (%d) [%ld]\n",t,v,Len[t],Nxt[t]-Buf[t]);
#endif

  return (v);
}

void string_merge()
{ int   O    = IOU[0];
  char *Obuf = Buf[0];
  char *Optr = Ptr[0];
  char *Oend = Optr + MAX_BUFFER;

  int   t, u;
  int   len;
  char *x;

  for (t = T; t >= 1; t--)
    { x = Pop(t);
      if (x != NULL)
        Add_To_Trie(x,t);
    }
#ifdef DEBUG
  Print_Trie();
#endif

  while ((t = Extract_Min()) > 0)
    { len = Len[t];
      if (Optr + len > Oend)
        { write(O,Obuf,Optr-Obuf);
          Optr = Obuf;
        }
      memcpy(Optr,Ptr[t],len);
      Optr += len;
      *Optr++ = '\n';

      while (t > 0)
        { u = List[t];
          List[t] = 0;
#ifdef DEBUG
          printf("Out %d: %s\n",t,Ptr[t]);
#endif
          x = Pop(t);
          if (x != NULL)
            Add_To_Trie(x,t);
          t = u;
        }
#ifdef DEBUG
      Print_Trie();
#endif
    }

  if (Optr > Obuf)
    write(O,Obuf,Optr-Obuf);
}


static struct rusage   Mtime;

static void startTime()
{ getrusage(RUSAGE_SELF,&Mtime); }

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
    { fprintf(stderr,"Usage: Trie <R:int> <out:file> <in:file> ...\n");
      exit (1);
    }

  R = atoi(argv[1]);

  T = argc-3;

  List = malloc(sizeof(int)*(T+1));
  if (List == NULL)
    { fprintf(stderr,"merge out of memory\n");
      exit (1);
    }

  IOU = (int *) malloc(sizeof(int)*(T+1));
  Buf = (char **) malloc(sizeof(char *)*(T+1));
  Ptr = (char **) malloc(sizeof(char *)*(T+1));
  Nxt = (char **) malloc(sizeof(char *)*(T+1));
  Len = (int *) malloc(sizeof(int)*(T+1));
  if (IOU == NULL || Buf == NULL || Ptr == NULL || Nxt == NULL || Len == NULL)
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
    { List[t] = 0;
      Buf[t]  = malloc(MAX_BUFFER);
      if (Buf[t] == NULL)
        { fprintf(stderr,"merge out of memory\n");
          exit (1);
        }
    }

  Setup_Trie(2*T,100);
  startTime();
  for (r = 0; r < R; r++)
    { for (t = 1; t <= T; t++)
        { Nxt[t] = Ptr[t] = Buf[t]+(MAX_BUFFER-1);
          *(Nxt[t]) = 0;
          Ptr[t] = Nxt[t]-1;
          *(Ptr[t]) = 0;
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
  printf(" & %.4g \\\\\n",timeTo()/R);
  fflush(stdout);

  exit (0);
}
