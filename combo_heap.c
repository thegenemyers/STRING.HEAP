#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/resource.h>

#undef DEBUG

#define MAX_BUFFER 2000000

static char *INFINITY = "~";

static int    T;
static int   *H;
static int   *L;
static char **V;

#define LCP2(x,y,n)		\
{ while (1)			\
    { a = x[n];			\
      b = y[n];			\
      if (a != b || a == 0)	\
        break;			\
      n += 1;			\
    }				\
}

#define LCP3(x,y,z,n)			\
{ while (1)				\
    { a = x[n];				\
      b = y[n];				\
      u = z[n];				\
      if (a != b || a != u || a == 0)	\
        break;				\
      n += 1;				\
    }					\
}

#ifdef DEBUG

static void print_heap(int i, int d)
{ printf("%*s%3d: (%2d) %s %d\n",2*d,"",i,L[i],V[H[i]],H[i]);
  if (2*i <= T)
    print_heap(2*i,d+1);
  if (2*i+1 <= T)
    print_heap(2*i+1,d+1);
}

#endif

void Heapify(int i, char *x, int t)
{ int   l,  r,  c;
  int   pl, pr, p;
  int   hl, hr;
  char *vl, *vr;
  int   px, py;
  int   a, b, u;

  c = i;
  p = 0;
  LCP2(V[H[i]],x,p)
  while ((l = 2*c) <= T)
    { hl = H[l];
      pl = L[l];
      pr = L[r = l+1];
      if (pr < pl)
        { if (p < pl)         //  Case 1L: v_l < x && v_l < v_r
            { H[c] = hl;
              L[c] = pl;
              c    = l;
            }
          else if (p > pl)    //   Case 2L: x < v_l && v_l < v_r
            break;
          else
            { vl = V[hl];
              px = pl;
              LCP2(vl,x,px)
              if (a < b)      //  Case 3La: v_l < x && v_l < v_r
                { H[c] = hl;
                  L[c] = pl;
                  p    = px;
                  c    = l;
                }
              else            //  Case 3Lb: x <= v_l && v_l < v_r
                { L[l] = px;
                  break;
                }
            }
        }
      else if (pr > pl)
        { hr = H[r];
          if (p < pr)         //  Case 1R
            { H[c] = hr;
              L[c] = pr;
              c    = r;
            }
          else if (p > pr)    //   Case 2R
            break;
          else
            { vr = V[hr];
              px = pr;
              LCP2(vr,x,px)
              if (a < b)       //  Case 3Ra
                { H[c] = hr;
                  L[c] = pr;
                  p    = px;
                  c    = r;
                }
              else                        //  Case 3La
                { L[r] = px;
                  break;
                }
            }
        }
      else if (p > pl)       //   Case 2: x < v_l, x < v_r
        break;
      else
        { vl = V[hl];
          vr = V[hr = H[r]];
          if (p < pl)               //   Case 4
            { px = pl;
              LCP2(vl,vr,px)
              if (a <= b)              //   Case 4L: v_l <= v_r < x
                { H[c] = hl;
                  L[c] = pl;
                  L[r] = px;
                  c    = l;
                }
              else                     //   Case 4R: x > v_l > v_r
                { H[c] = hr;
                  L[c] = pr;
                  L[l] = px;
                  c    = r;
                }
            }
          else                         //   Case 5
            { px = p;
              LCP3(vr,vl,x,px)
              if (a > b)
                { if (u > b)        //  Case 5.1L: v_r, x > v_l
                    { H[c] = hl;
                      L[c] = pl;
                      L[r] = px;
                      p    = px;
                      c    = l;
                    }
                  else if (u < b)   //  Case 5.2L: x < v_l < v_r
                    { L[l] = px;
                      L[r] = px;
                      break;
                    }
                  else
                    { py = px;
                      LCP2(vl,x,py)
                      if (a < b)    //   Case 5.3La: v_l < x, v_r
                        { H[c] = hl;
                          L[c] = pl;
                          L[r] = px;
                          p    = py;
                          c    = l;
                        }
                      else                     //   Case 5.3Lb: x <= v_l < v_r
                        { L[l] = py;
                          L[r] = px;
                          break;
                        }
                    }
                }
              else if (a < b)
                { if (u > a)         //  Case 5.1R
                    { H[c] = hr;
                      L[c] = pr;
                      L[l] = px;
                      p    = px;
                      c    = r;
                    }
                  else if (u < a)   //  Case 5.2R
                    { L[r] = px;
                      L[l] = px;
                      break;
                    }
                  else
                    { py = px;
                      LCP2(vr,x,py);
                      if (a < b)   //  Case 5.3Ra
                        { H[c] = hr;
                          L[c] = pr;
                          L[l] = px;
                          p    = py;
                          c    = r;
                        }
                      else                     //  Case 5.3Rb
                        { L[r] = py;
                          L[l] = px;
                          break;
                        }
                    }
                }
              else if (u <= b)    //  Case 5.2: v_l = v_r >= x
                { L[l] = px;
                  L[r] = px;
                  break;
                }
              else                //  Case 5.4
                { py = px;
                  LCP2(vl,vr,py)
                  if (a < b)     //  Case 5.4L
                    { H[c] = hl;
                      L[c] = pl;
                      L[r] = py;
                      p    = px;
                      c    = l;
                    }
                  else                        //  Case 5.4R
                    { H[c] = hr;
                      L[c] = pr;
                      L[l] = py;
                      p    = px;
                      c    = r;
                    }
                }
            }
        }
    }
  H[c] = t;
  L[c] = p;
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

  //  make a list of all the nodes containing equal elements

void cohort(int c, int t)
{ int l, u;

  l = 2*c;
  if (l <= T)
    { u = H[l];
      if (Len[u] == L[l])
        cohort(l,u);
      l += 1;
      u = H[l];
      if (Len[u] == L[l])
        cohort(l,u);
    }

  { char *x;

#ifdef DEBUG
    printf("Out %d: %s\n",t,V[t]);
#endif
    x = Pop(t);
    if (x == NULL)
      Heapify(c,INFINITY,0);
    else
      Heapify(c,x,t);
  }
}

void string_merge()
{ int   O    = IOU[0];
  char *Obuf = Buf[0];
  char *Optr = Ptr[0];
  char *Oend = Optr + MAX_BUFFER;

  int   t;
  int   len;
  char *x;

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

  while ((t = H[1]) > 0)
    { len = Len[t];
      if (Optr + len > Oend)
        { write(O,Obuf,Optr-Obuf);
          Optr = Obuf;
        }
      memcpy(Optr,V[t],len);
      Optr += len;
      *Optr++ = '\n';

      cohort(1,t);

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
    { fprintf(stderr,"Usage: SCheap <R:int> <out:file> <in:file> ...\n");
      exit (1);
    }

  R = atoi(argv[1]);

  T = argc-3;

  H = malloc(sizeof(int)*(T+2));
  L = malloc(sizeof(int)*(T+2));
  V = malloc(sizeof(char *)*(T+1));
  if (H == NULL || L == NULL || V == NULL)
    { fprintf(stderr,"merge out of memory\n");
      exit (1);
    }
  H[T+1] = 0;
  L[T+1] = -1;

  IOU = (int *) malloc(sizeof(int)*(T+1));
  Buf = (char **) malloc(sizeof(char *)*(T+1));
  Ptr = (char **) malloc(sizeof(char *)*(T+1));
  Len = (int *) malloc(sizeof(int)*(T+1));
  if (IOU == NULL || Buf == NULL || Ptr == NULL || Len == NULL)
    { fprintf(stderr,"merge out of memory\n");
      exit (1);
    }
  Len[0] = 1; 

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
  printf(" & %.4g",timeTo()/R); fflush(stdout);

  exit (0);
}
