#ifndef _THREAD_POOL_HPP_
#define _THREAD_POOL_HPP_

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <exception>
#include <iostream>
#include <queue>
#include "locker.hpp"

using namespace std;

template <typename Type>
class ThreadPool {
 private:
  int thread_number;
  pthread_t *threads;
  queue<Type *> tasks;
  MutexLocker mutex_locker;
  SemaphoreLocker sem_locker;
  ConditionLocker cond_locker;
  bool state;

 public:
  ThreadPool(){};
  ThreadPool(int thread_number = 20);
  ~ThreadPool();
  bool add_task(Type *task);
  void start();
  void stop();

 private:
  static void *worker(void *arg);
  void run();
  Type *get_task();
};

template <typename Type>
void clear(queue<Type> &q) {
  queue<Type> empty;
  swap(empty, q);
}

template <typename Type>
ThreadPool<Type>::ThreadPool(int thread_number) {
  this->thread_number = thread_number;
  clear(this->tasks);
  this->state = true;

  if (this->thread_number <= 0) {
    cout << "threadpool can't init because threadnumber <= 0" << endl;
    return;
  }

  threads = new pthread_t[this->thread_number];
  if (threads == NULL) {
    cout << "thread array can't be creat" << endl;
    return;
  }
}

template <typename Type>
ThreadPool<Type>::~ThreadPool() {
  delete[] threads;
  stop();
}

template <typename Type>
bool ThreadPool<Type>::add_task(Type *task) {
  mutex_locker.lock();

  bool is_signal = tasks.empty();

  tasks.push(task);
  mutex_locker.unlock();
  if (is_signal) {
    cond_locker.signl();
  }
  return true;
}

template <typename Type>
void ThreadPool<Type>::start() {
  for (int i = 0; i < thread_number; i++) {
    if (pthread_create(threads + i, NULL, worker, this) != 0) {
      delete[] threads;
      throw std::exception();
    }
    if (pthread_detach(threads[i])) {
      delete[] threads;
      throw std::exception();
    }
  }
  cout << "creat therad pool" << endl;
}

template <typename Type>
void ThreadPool<Type>::stop() {
  state = false;
  cond_locker.broadcast();
}

template <typename Type>
void *ThreadPool<Type>::worker(void *arg) {
  ThreadPool *pool = (ThreadPool *)arg;
  pool->run();
  return pool;
}

template <typename Type>
void ThreadPool<Type>::run() {
  while (state) {
    Type *task = get_task();
    if (task == NULL) {
      cond_locker.wait();
    } else {
      task->run();
    }
  }
}

template <typename Type>
Type *ThreadPool<Type>::get_task() {
  Type *task = NULL;
  mutex_locker.lock();
  if (!tasks.empty()) {
    task = tasks.front();
    tasks.pop();
  }
  mutex_locker.unlock();
  return task;
}

#endif 