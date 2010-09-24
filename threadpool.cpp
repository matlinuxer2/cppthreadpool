/*
    Thread Pool implementation for unix / linux environments
    Copyright (C) 2008 Shobhit Gupta

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include "threadpool.h"

using namespace std;

pthread_mutex_t ThreadPool::mutexSync = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ThreadPool::mutexWorkCompletion = PTHREAD_MUTEX_INITIALIZER;


ThreadPool::ThreadPool(unsigned int num_thread)
:_num_thread(num_thread)
,_worker_queue(num_thread, NULL)
,queueSize(num_thread)
{
	pthread_mutex_lock(&mutexSync);
	topIndex = 0;
	bottomIndex = 0;
	incompleteWork = 0;
	sem_init(&_available_work, 0, 0);
	sem_init(&_available_thread, 0, queueSize);
	pthread_mutex_unlock(&mutexSync);
}

void ThreadPool::initializeThreads()
{
	for (unsigned int i = 0; i < _num_thread; ++i)
	{
		pthread_t tempThread;
		pthread_create(&tempThread, NULL, &ThreadPool::threadExecute, (void *) this );
		//threadIdVec[i] = tempThread;
	}

}

ThreadPool::~ThreadPool()
{
	_worker_queue.clear();
}



void ThreadPool::destroyPool(int maxPollSecs = 2)
{
	while( incompleteWork>0 )
	{
		//cout << "Work is still incomplete=" << incompleteWork << endl;
		sleep(maxPollSecs);
	}
	cout << "All Done!! Wow! That was a lot of work!" << endl;
	sem_destroy(&_available_work);
	sem_destroy(&_available_thread);
	pthread_mutex_destroy(&mutexSync);
	pthread_mutex_destroy(&mutexWorkCompletion);

}


bool ThreadPool::assignWork(WorkerThread *workerThread)
{
	pthread_mutex_lock(&mutexWorkCompletion);
	incompleteWork++;
	//cout << "assignWork...incomapleteWork=" << incompleteWork << endl;
	pthread_mutex_unlock(&mutexWorkCompletion);

	sem_wait(&_available_thread);

	pthread_mutex_lock(&mutexSync);
	//workerVec[topIndex] = workerThread;
	_worker_queue[topIndex] = workerThread;
	//cout << "Assigning Worker[" << workerThread->id << "] Address:[" << workerThread << "] to Queue index [" << topIndex << "]" << endl;
	if(queueSize !=1 )
		topIndex = (topIndex+1) % (queueSize-1);
	sem_post(&_available_work);
	pthread_mutex_unlock(&mutexSync);
	return true;
}

bool ThreadPool::fetchWork(WorkerThread **workerArg)
{
	sem_wait(&_available_work);

	pthread_mutex_lock(&mutexSync);
	WorkerThread * workerThread = _worker_queue[bottomIndex];
	_worker_queue[bottomIndex] = NULL;
	*workerArg = workerThread;
	if(queueSize !=1 )
		bottomIndex = (bottomIndex+1) % (queueSize-1);
	sem_post(&_available_thread);
	pthread_mutex_unlock(&mutexSync);
	return true;
}

void *ThreadPool::threadExecute(void *param)
{
	WorkerThread *worker = NULL;

	while(((ThreadPool *)param)->fetchWork(&worker))
	{
		if(worker)
		{
			worker->executeThis();
			//cout << "worker[" << worker->id << "]\tdelete address: [" << worker << "]" << endl;
			delete worker;
			worker = NULL;
		}

		pthread_mutex_lock( &(((ThreadPool *)param)->mutexWorkCompletion) );
		//cout << "Thread " << pthread_self() << " has completed a Job !" << endl;
		((ThreadPool *)param)->incompleteWork--;
		pthread_mutex_unlock( &(((ThreadPool *)param)->mutexWorkCompletion) );
	}
	return 0;
}
