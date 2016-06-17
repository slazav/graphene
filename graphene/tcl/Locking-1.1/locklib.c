#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>

#include "locklib.h"

static key_t string_to_key(char *string);

int lock_init(char *name) {
  key_t k = string_to_key(name);
  int sem, res;
  union semun {
    int val;
    struct semid_ds *buf; 
    unsigned short int *array; 
    struct seminfo *__buf; 
  } arg;
  /*fprintf(stderr,"assign key %lx to name %s\n",k,name);*/
  /* try to create semaphore */
  sem = semget(k,1,IPC_CREAT|IPC_EXCL|0600);
  if (sem < 0) {
    if (errno == EEXIST) {
      /* good it exists (and let's hope it is the one we want and
	 that it is initialized already).
	 open it normal way */
      return semget(k,1,0600);
    } else {
      return sem;
    }
  }
  /* now we have exclusive access, lets initialize semaphore */
  /*fprintf(stderr,"lock_init: try to initialize semaphore\n");*/
  arg.val = 1;
  res = semctl(sem,0,SETVAL,arg);
  if (res < 0) {
    /* error - delete semaphore */
    semctl(sem,0,IPC_RMID,arg);
    return res;
  }
  return sem;
}

int lock_release(int semid) {
  struct sembuf op;
  op.sem_num = 0;
  op.sem_op = 1;
  op.sem_flg = SEM_UNDO;
  return semop(semid,&op,1);
}

int lock_get(int semid) {
  struct sembuf op;
  op.sem_num = 0;
  op.sem_op = -1;
  op.sem_flg = IPC_NOWAIT|SEM_UNDO;
  return semop(semid,&op,1);
}

int lock_wait(int semid) {
  struct sembuf op;
  op.sem_num = 0;
  op.sem_op = -1;
  op.sem_flg = SEM_UNDO;
  return semop(semid,&op,1);
}

static key_t string_to_key(char *string) {
  /* This is stolen from tclHash.c */
  /* BROKEN!!: 30 and 29 result in hash collision! */
  key_t result;
  int c;

  /*
   * I tried a zillion different hash functions and asked many other
   * people for advice.  Many people had their own favorite functions,
   * all different, but no-one had much idea why they were good ones.
   * I chose the one below (multiply by 9 and add new character)
   * because of the following reasons:
   *
   * 1. Multiplying by 10 is perfect for keys that are decimal strings,
   *    and multiplying by 9 is just about as good.
   * 2. Times-9 is (shift-left-3) plus (old).  This means that each
   *    character's bits hang around in the low-order bits of the
   *    hash value for ever, plus they spread fairly rapidly up to
   *    the high-order bits to fill out the hash value.  This seems
   *    works well both for decimal and non-decimal strings.
   */

  result = 0;
  while (1) {
    c = *string;
    string++;
    if (c == 0) {
      break;
    }
    result += (result<<3) + c;
    /*fprintf(stderr,"char=%c res=%x\n",c,result);*/
  }
  if (result == IPC_PRIVATE) result++;
  return result;
}
