/*******************************************************************************
 * Software Name :safe
 *
 * Copyright (C) 2014 HanVideo.
 *
 * author : 梁智游
 ******************************************************************************/

#ifndef OS_H_
#define OS_H_

#include "../include/def.h"

typedef struct __tag_thread Suntech_Thread;

//thread status
enum
{
	SUNTECH_THREAD_STATUS_STOP = 0,   // the thread has been initialized but is not started yet
	SUNTECH_THREAD_STATUS_RUN = 1,    //the thread is running
	SUNTECH_THREAD_STATUS_DEAD = 2    // the thread has exited its body function
};

//thread priorities
enum
{
	SUNTECH_THREAD_PRIORITY_IDLE=0,
	SUNTECH_THREAD_PRIORITY_LESS_IDLE,
	SUNTECH_THREAD_PRIORITY_LOWEST,
	SUNTECH_THREAD_PRIORITY_LOW,
	SUNTECH_THREAD_PRIORITY_NORMAL,
	SUNTECH_THREAD_PRIORITY_HIGH,
	SUNTECH_THREAD_PRIORITY_HIGHEST,
	SUNTECH_THREAD_PRIORITY_REALTIME,
	SUNTECH_THREAD_PRIORITY_REALTIME_END=255
};

//function define

//Constructs a new thread object
Suntech_Thread* suntech_thread_new();

//thread destructor
void suntech_thread_del(Suntech_Thread* th);

//thread run function callback
typedef unsigned int (*suntech_th_run)(void *param);

//thread execution
bool suntech_thread_run(Suntech_Thread *th, suntech_th_run run, void *param);

//thread stoping
void suntech_thread_stop(Suntech_Thread *th);

//thread status query
unsigned int suntech_thread_status(Suntech_Thread *th);

//thread priority
void suntech_thread_set_priority(Suntech_Thread *th, int priority);

//current thread ID
unsigned int suntech_thread_id();

/*********************************************************************
					Semaphore Object
**********************************************************************/

//abstracted semaphore object
typedef struct __tag_semaphore Suntech_Semaphore;

//semaphore constructor
Suntech_Semaphore *suntech_sema_new(unsigned int MaxCount, unsigned int InitCount);

//semaphore destructor
void suntech_sema_del(Suntech_Semaphore *sm);

//semaphore notifivation
unsigned int suntech_sema_notify(Suntech_Semaphore *sm, unsigned int nb_rel);

//semaphore wait
void suntech_sema_wait(Suntech_Semaphore *sm);

//semaphore time wait
bool suntech_sema_wait_for(Suntech_Semaphore *sm, unsigned int time_out);


/*********************************************************************
					Mutex Object
**********************************************************************/

//abstracted mutex object
typedef struct __tag_mutex Suntech_Mutex;

//Contructs a new mutex object
Suntech_Mutex *suntech_mx_new();

//mutex denstructor
void suntech_mx_del(Suntech_Mutex *mx);

//mutex locking
unsigned int suntech_mx_lock(Suntech_Mutex *mx);

//mutex unlocking
void suntech_mx_unlock(Suntech_Mutex *mx);

//mutex non-blocking lock
bool suntech_mx_try_lock(Suntech_Mutex *mx);

//os
void suntech_sleep(unsigned int ms);



#endif /* OS_H_ */
