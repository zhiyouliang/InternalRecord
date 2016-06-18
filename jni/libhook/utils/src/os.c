/*******************************************************************************
 * Software Name :safe
 *
 * Copyright (C) 2014 HanVideo.
 *
 * author : 梁智游
 ******************************************************************************/

#include "../include/os.h"

#if defined(WIN32)
#include <windows.h>
typedef HANDLE TH_HANDLE;
#else
#include <sched.h>
#include <pthread.h>
#include <semaphore.h>
typedef pthread_t TH_HANDLE ;
#endif

struct __tag_thread
{

	unsigned int status;
	TH_HANDLE threadH;
	unsigned int stackSize;
	unsigned int (*Run)(void *param);
	void *args;
	Suntech_Semaphore *_signal;
};

void Thread_Stop(Suntech_Thread *th, bool Destroy);
#ifdef WIN32
DWORD WINAPI RunThread(void *ptr);
#else
void *RunThread(void *ptr);
#endif

Suntech_Thread* suntech_thread_new()
{
	Suntech_Thread *tmp = (Suntech_Thread*)malloc(sizeof(Suntech_Thread));
	memset(tmp, 0, sizeof(Suntech_Thread));
	tmp->status = SUNTECH_THREAD_STATUS_STOP;
	return tmp;
}

void suntech_thread_del(Suntech_Thread* th)
{
	Thread_Stop(th, 0);
	#ifdef WIN32
	if (th->threadH)
		CloseHandle(th->threadH);
	#endif
		free(th);
}


bool suntech_thread_run(Suntech_Thread *th, suntech_th_run run, void *param)
{
#ifdef WIN32
	DWORD id;
#else
	pthread_attr_t att;
#endif

	if (!th || th->Run || th->_signal)
		return 1;

	th->Run = run;
	th->args = param;
	th->_signal = suntech_sema_new(1, 0);

#ifdef WIN32
	th->threadH = CreateThread(0,0,&(RunThread),(void *)th, 0, &id);
	if (th->threadH == 0)
	{
#else
	if ( pthread_attr_init(&att) != 0 )
		return 1;
	pthread_attr_setdetachstate(&att, PTHREAD_CREATE_JOINABLE);
	if ( pthread_create(&th->threadH, &att, RunThread, th) != 0 )
	{
#endif
		th->status = SUNTECH_THREAD_STATUS_DEAD;
		return 1;
	}

	//wait for the child function to call us - do NOT return before, otherwise the thread status would
	//be unknown
	suntech_sema_wait(th->_signal);
	suntech_sema_del(th->_signal);
	th->_signal = 0;

	return 0;
}

void suntech_thread_stop(Suntech_Thread *th)
{
	Thread_Stop(th, 0);
}

unsigned int suntech_thread_status(Suntech_Thread *th)
{
	if (!th)
		return 0;

	return th->status;
}

void suntech_thread_set_priority(Suntech_Thread *th, int priority)
{
#ifdef WIN32
	SetThreadPriority(th ? th->threadH : GetCurrentThread(), priority);

#else

	struct sched_param s_par;
	if (!th)
		return;


	if (priority > 200)
	{
		s_par.sched_priority = priority - 200;
		pthread_setschedparam(th->threadH, SCHED_RR, &s_par);
	}
	else
	{
		s_par.sched_priority = priority;
		pthread_setschedparam(th->threadH, SCHED_OTHER, &s_par);
	}

#endif
}

unsigned int suntech_thread_id()
{
#ifdef WIN32
	return ((unsigned int) GetCurrentThreadId());
#else
	return ((unsigned int) pthread_self());
#endif
}

void Thread_Stop(Suntech_Thread *th, bool Destroy)
{
	if (suntech_thread_status(th) == SUNTECH_THREAD_STATUS_RUN)
	{
	#ifdef WIN32
			if (Destroy)
			{
				DWORD dw = 1;
				TerminateThread(th->threadH, dw);
				th->threadH = 0;
			}
			else
			{
				WaitForSingleObject(th->threadH, INFINITE);
			}
	#else
			pthread_join(th->threadH, 0);

	#endif
		}

		th->status = SUNTECH_THREAD_STATUS_DEAD;
}

#ifdef WIN32
DWORD WINAPI RunThread(void *ptr)
{
	DWORD ret = 0;
#else
void *RunThread(void *ptr)
{
	unsigned ret = 0;
#endif

	Suntech_Thread *th = (Suntech_Thread *)ptr;

	// Signal the caller
	if (! th->_signal)
		goto exit;

	th->status = SUNTECH_THREAD_STATUS_RUN;

	suntech_sema_notify(th->_signal, 1);
	// Run our thread
	ret = th->Run(th->args);

exit:
	th->status = SUNTECH_THREAD_STATUS_DEAD;
	th->Run = 0;
#ifdef WIN32
	CloseHandle(th->threadH);
	th->threadH = 0;
	return ret;
#else
	pthread_exit((void *)0);
	return (void *)ret;
#endif

}

/*********************************************************************
						OS-Specific Semaphore Object
**********************************************************************/

struct __tag_semaphore
{
#ifdef WIN32
	HANDLE hSemaphore;
#else
	sem_t *hSemaphore;
	sem_t SemaData;
#ifdef __Darwin__
	const char *SemName;
#endif
#endif
};

Suntech_Semaphore *suntech_sema_new(unsigned int MaxCount, unsigned int InitCount)
{
	Suntech_Semaphore *tmp = (Suntech_Semaphore*)malloc(sizeof(Suntech_Semaphore));

	if (!tmp)
		return 0;

#if defined(WIN32)
	tmp->hSemaphore = CreateSemaphore(0, InitCount, MaxCount, 0);
#elif defined(__DARWIN__)

	{
		char semaName[40];
		sprintf(semaName,"SUNTECH_SEM%d", (unsigned int)tmp);
		tmp->SemName = strdup(semaName);
	}
	tmp->hSemaphore = sem_open(tmp->SemName, O_CREAT, S_IRUSR|S_IWUSR, InitCount);
#else
	if (sem_init(&tmp->SemaData, 0, InitCount) < 0 )
	{
		free(tmp);
		return 0;
	}
	tmp->hSemaphore = &tmp->SemaData;
#endif

	if (!tmp->hSemaphore)
	{
		free(tmp);
		return 0;
	}
	return tmp;
}

void suntech_sema_del(Suntech_Semaphore *sm)
{
#if defined(WIN32)
	CloseHandle(sm->hSemaphore);
#elif defined(__DARWIN__)
	sem_t *sema = sem_open(sm->SemName, 0);
	sem_destroy(sema);
	free(tmp->SemName);
#else
	sem_destroy(sm->hSemaphore);
#endif

	free(sm);
}

unsigned int suntech_sema_notify(Suntech_Semaphore *sm, unsigned int nb_rel)
{
	unsigned int prevCount;
#ifndef WIN32
	sem_t *hSem;
#endif

	if (!sm)
		return 0;

#if defined(WIN32)
	ReleaseSemaphore(sm->hSemaphore, nb_rel, (LPLONG)&prevCount);
#else

#if defined(__DARWIN__)
	hSem = sem_open(sm->SemName, 0);
#else
	hSem = sm->hSemaphore;
#endif
	sem_getvalue(hSem, (int *) &prevCount);
	while (nb_rel) {
		if (sem_post(hSem) < 0) return 0;
		nb_rel -= 1;
	}
#endif
	return (unsigned int) prevCount;
}

void suntech_sema_wait(Suntech_Semaphore *sm)
{
#ifdef WIN32
	WaitForSingleObject(sm->hSemaphore, INFINITE);
#else
	sem_t *hSem;
#ifdef __DARWIN__
	hSem = sem_open(sm->SemName, 0);
#else
	hSem = sm->hSemaphore;
#endif
	sem_wait(hSem);
#endif
}

bool suntech_sema_wait_for(Suntech_Semaphore *sm, unsigned int time_out)
{
#ifdef WIN32
	if (WaitForSingleObject(sm->hSemaphore, time_out) == WAIT_TIMEOUT)
		return 0;

	return 1;
#else
	return 1;
#endif
}

/*********************************************************************
						OS-Specific Mutex Object
**********************************************************************/
struct __tag_mutex
{
#ifdef WIN32
	HANDLE hMutex;
#else
	pthread_mutex_t hMutex;
#endif

	unsigned int Holder, HolderCount;
};

Suntech_Mutex *suntech_mx_new()
{
#ifndef WIN32
	pthread_mutexattr_t attr;
#endif

	Suntech_Mutex *tmp = (Suntech_Mutex*)malloc(sizeof(Suntech_Mutex));
	if (!tmp)
		return 0;

	memset(tmp, 0, sizeof(Suntech_Mutex));

#ifdef WIN32
	tmp->hMutex = CreateMutex(0, FALSE, 0);
	if (!tmp->hMutex)
	{
#else
	pthread_mutexattr_init(&attr);
	if ( pthread_mutex_init(&tmp->hMutex, &attr) != 0 )
	{
#endif
		free(tmp);
		return 0;
	}

	return tmp;
}

void suntech_mx_del(Suntech_Mutex *mx)
{
#ifdef WIN32
	CloseHandle(mx->hMutex);
#else
	pthread_mutex_destroy(&mx->hMutex);
#endif

	free(mx);
}

unsigned int suntech_mx_lock(Suntech_Mutex *mx)
{
	unsigned int caller;

	if (!mx)
		return 0;

	caller = suntech_thread_id();

	if (caller == mx->Holder)
	{
		mx->HolderCount += 1;
		return 1;
	}

#ifdef WIN32
	switch (WaitForSingleObject(mx->hMutex, INFINITE))
	{
	case WAIT_ABANDONED:
	case WAIT_TIMEOUT:
		return 0;
	default:
		mx->HolderCount = 1;
		mx->Holder = caller;
		return 1;
	}
#else
	if (pthread_mutex_lock(&mx->hMutex) == 0 )
	{
		mx->Holder = caller;
		mx->HolderCount = 1;
		return 1;
	}

	return 0;
#endif
}

void suntech_mx_unlock(Suntech_Mutex *mx)
{
	unsigned int caller;

	if (!mx)
		return;

	caller = suntech_thread_id();

	//assert(caller == mx->Holder);

	if (caller != mx->Holder)
		return;

	//assert(mx->HolderCount > 0);

	mx->HolderCount -= 1;

	if (mx->HolderCount == 0)
	{
		mx->Holder = 0;
#ifdef WIN32
		ReleaseMutex(mx->hMutex);
#else
		pthread_mutex_unlock(&mx->hMutex);
#endif
	}
}

bool suntech_mx_try_lock(Suntech_Mutex *mx)
{
	unsigned int caller;

	if (!mx)
		return 0;

	caller = suntech_thread_id();

	if (caller == mx->Holder)
	{
		mx->HolderCount += 1;
		return 1;
	}

#ifdef WIN32
	switch (WaitForSingleObject(mx->hMutex, 1))
	{
	case WAIT_ABANDONED:
	case WAIT_TIMEOUT:
		return 0;
	default:
		mx->HolderCount = 1;
		mx->Holder = caller;
		return 1;
	}
#else
	if (pthread_mutex_trylock(&mx->hMutex) == 0 )
	{
		mx->Holder = caller;
		mx->HolderCount = 1;
		return 1;
	}
	return 0;
#endif
}


void suntech_sleep(unsigned int ms)
{
#ifdef WIN32
	Sleep(ms);
#else
	//
#endif
}




