#ifndef _LOCKER_HPP_
#define _LOCKER_HPP_

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>

// Semaphore class
class SemaphoreLocker {
 private:
  sem_t my_sem;

 public:
  SemaphoreLocker() {
    if (sem_init(&my_sem, 0, 0) != 0) {
      printf("Semaphore initialization failed\n");
    }
  }

  ~SemaphoreLocker() { sem_destroy(&my_sem); }

  bool post() { return sem_post(&my_sem) == 0; }

  bool wait() { return sem_wait(&my_sem) == 0; }
};

// mutex locker

class MutexLocker {
 private:
  pthread_mutex_t my_mutex;

 public:
  MutexLocker() {
    if (pthread_mutex_init(&my_mutex, NULL) != 0) {
      printf("Mutex initialization failed\n");
    }
  }

  ~MutexLocker() { pthread_mutex_destroy(&my_mutex); }

  bool lock() { return pthread_mutex_lock(&my_mutex) == 0; }

  bool unlock() { return pthread_mutex_unlock(&my_mutex) == 0; }
};

// Condition class
class ConditionLocker {
 private:
  pthread_mutex_t my_mutex;
  pthread_cond_t my_cond;

 public:
  ConditionLocker() {
    if (pthread_mutex_init(&my_mutex, NULL) != 0) {
      printf("Mutex initialization failed\n");
    }
    if (pthread_cond_init(&my_cond, NULL) != 0) {
      pthread_mutex_destroy(&my_mutex);
      printf("Condition initialization failed");
    }
  }

  ~ConditionLocker() {
    pthread_mutex_destroy(&my_mutex);
    pthread_cond_destroy(&my_cond);
  }

  bool wait() {
    int result = 0;
    pthread_mutex_lock(&my_mutex);
    result = pthread_cond_wait(&my_cond, &my_mutex);
    pthread_mutex_unlock(&my_mutex);
    return result == 0;
  }

  bool signl() { return pthread_cond_signal(&my_cond) == 0; }

  bool broadcast() { return pthread_cond_broadcast(&my_cond) == 0; }
};

#endif