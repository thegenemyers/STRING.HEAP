#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>

#undef VERBOSE

#define NTHREADS 8

typedef unsigned long long uint64;

static int    K;
static int    KE;
static int    Ksft;
static uint64 Kpow;

//  Psuedo-randome number generator

#define myrand48_a 0x5deece66dull
#define myrand48_c 0xbull

static inline double erand(uint64 *GENER)
{ uint64 temp;

  *GENER = ((*GENER) * myrand48_a + myrand48_c) & 0xffffffffffffull;
  temp = 0x3ff0000000000000ull | (*GENER << 4);
  return (*((double *) &temp) - 1.0);
}

static inline uint64 eseed(uint64 seedval)
{ return ((((uint64) seedval) << 16) | 0x330e); }

static inline uint64 sample_exponential(uint64 mean, uint64 *GENER)
{ return ((uint64) (-log(1.-erand(GENER))*mean+.5)); }

typedef struct
  { char  *root;
    int    tid;
    uint64 beg;
    uint64 end;
    int    N;
    uint64 gen;
  } ARG;

static void *list_thread(void *args)
{ ARG *parm = (ARG *) args;
  int    f;
  int    i, k;
  uint64 val, end, dist, lim;
  char  *name;
  char  *bufr, *bend, *bptr;
  uint64 GENER;
  uint64 KMASK, KPOWR;

  name = malloc(strlen(parm->root)+100);

  KPOWR  = (0x1ull << Ksft); 
  KMASK  = KPOWR-1;

  bufr = malloc((KE+1)*1000000);
  bend = bufr + 1000000*(KE+1);

  sprintf(name,"%s.U.%d",parm->root,parm->tid);
  f = open(name,O_WRONLY|O_CREAT|O_TRUNC,S_IRWXU);

  free(name);

  GENER = parm->gen;

  bptr = bufr;
  end = parm->end;
  lim = parm->end - (parm->N-1)*Kpow;
  val = parm->beg;
  for (i = parm->N; i > 0; i--)
    { dist = sample_exponential((end-val)/i,&GENER);
      if (dist < Kpow)
        dist = Kpow;
      if (val+dist > lim)
        dist = lim-val;
      val += dist;
      if (dist < Kpow)
        { fprintf(stderr,"Ended %d strings early, should not happen\n",i);
          exit (1);
        }
      if (bptr >= bend)
        { write(f,bufr,bptr-bufr);
          bptr = bufr;
        }
      for (k = 1; k <= K; k++)
        *bptr++ = '0' + ((val >> (62-k*Ksft)) & KMASK);
      for (k = K; k < KE; k++)
        *bptr++ = '0' + erand(&GENER)*KPOWR;

      *bptr++ = '\n';
      lim += Kpow;
    }
  if (bptr > bufr)
    write(f,bufr,bptr-bufr);

  close(f);

  return (NULL);
}

#define LIMIT 0x4000000000000000ull

int main(int argc, char *argv[])
{ int    N, T;
  int    t, p;
  int    PID;
  char  *command;

  if (argc != 6)
    { fprintf(stderr,"Usage: Rran <root:string> <N:int> <K:int> <S:int> <T:int>\n");
      exit (1);
    }

  N = atoi(argv[2]);
  K = atoi(argv[3]);
  Ksft = atoi(argv[4]);
  T = atoi(argv[5]);

  KE = K;
  if (K*Ksft > 62)
    K = 62/Ksft;
  Kpow = (0x1ull << (62-K*Ksft));

#ifdef VERBOSE
  printf("Generating %d sorted files with %d random elements of length %d with alpha size %d\n",
         T,N,KE,(0x1<<Ksft));
#else
  printf(" %d, %d",N*T,(0x1<<Ksft));
#endif

  command = malloc(2*strlen(argv[1])+100);

  PID  = getpid();

  for (p = 0; p < T; p++)
    {
#ifdef VERBOSE
      printf("  Generating %s.%d\n",argv[1],p+1);
#endif

      { pthread_t threads[NTHREADS];
        ARG       parm[NTHREADS];

        for (t = 0; t < NTHREADS; t++)
          { parm[t].root = argv[1];
            parm[t].tid  = t;
            if (t == 0)
              parm[0].beg = 0x0ull;
            else
              parm[t].beg = parm[t-1].end;
            if (t == NTHREADS-1)
              parm[t].end = LIMIT-1;
            else
              parm[t].end = LIMIT * ((t+1.)/NTHREADS);
            parm[t].N = N/NTHREADS;
            parm[t].gen = eseed(PID + p*NTHREADS + t);
          }

#ifdef DEBUG_THREADS
        for (t = 0; t < NTHREADS; t++)
          list_thread(parm+t);
#else
        for (t = 1; t < NTHREADS; t++)
          pthread_create(threads+t,NULL,list_thread,parm+t);
        list_thread(parm);
        for (t = 1; t < NTHREADS; t++)
          pthread_join(threads[t],NULL);
#endif
      }

#ifdef VERBOSE
      printf("  Cat'ing %s.%d\n",argv[1],p+1);
#endif

      sprintf(command,"cat %s.U.* >%s.%d",argv[1],argv[1],p+1);
      system(command);

      sprintf(command,"rm %s.U.*",argv[1]);
      system(command);
    }

  exit (0);
}
