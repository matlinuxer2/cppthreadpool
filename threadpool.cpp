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
#include "threadpool.h"
#include <iostream>
#include <stdlib.h>
#include <errno.h>

using namespace std;

class ScopedMutex {
public:
	explicit ScopedMutex(pthread_mutex_t * const mutex):_mutex(mutex)
	{
		int ret = pthread_mutex_lock(_mutex);
		switch (ret) {
		case 0:
			break;
		case EINVAL:
			throw Error("EINVAL returned by pthread_mutex_lock()");
		case EAGAIN:
			throw Error("EAGAIN returned by pthread_mutex_lock()");
		case EDEADLK:
			throw Error("EDEADLK returned by pthread_mutex_lock()");
		default:
			throw Error("UNKNOWN returned by pthread_mutex_lock()");
		}
	}

	~ScopedMutex()
	{
		int ret = pthread_mutex_unlock(_mutex);
		switch (ret) {
		case 0:
			break;
		case EPERM:
		default:
			/*
			 * No choose but die here. Destructor shall be a nofail function.
			 */
			std::cerr << ret << " returned by pthread_mutex_unlock()" << std::endl;

			std::terminate();
		}
	}
private:
	pthread_mutex_t * const _mutex;
};


ThreadPool::ThreadPool(unsigned int num_thread)
:_thread_pool(num_thread)
,_num_thread(num_thread)
,_worker_queue(num_thread, NULL)
,_queue_size(num_thread)
{
	_top_index = 0;
	_bottom_index = 0;
	_incomplete_work = 0;
	sem_init(&_available_work, 0, 0);
	sem_init(&_available_thread, 0, _queue_size);

	init_mutex(&_mutex_sync);
	init_mutex(&_mutex_work_completion);

	for (std::vector<pthread_t>::iterator i = _thread_pool.begin(); i != _thread_pool.end(); ++i) {
		pthread_create(&*i, NULL, &ThreadPool::thread_execute, this);
	}
}

ThreadPool::~ThreadPool()
{
	_worker_queue.clear();
}



void ThreadPool::destroy_pool(int maxPollSecs = 2)
{
	while( _incomplete_work>0 )
	{
		//cout << "Work is still incomplete=" << _incomplete_work << endl;
		sleep(maxPollSecs);
	}
	cout << "All Done!! Wow! That was a lot of work!" << endl;
	sem_destroy(&_available_work);
	sem_destroy(&_available_thread);
	pthread_mutex_destroy(&_mutex_sync);
	pthread_mutex_destroy(&_mutex_work_completion);

}


bool ThreadPool::assign_work(WorkerThread *workerThread)
{
	{
		ScopedMutex mutex(&_mutex_work_completion);
		_incomplete_work++;
	}

	sem_wait(&_available_thread);

	{
		ScopedMutex mutex(&_mutex_sync);
		_worker_queue[_top_index] = workerThread;
		if(_queue_size !=1 )
			_top_index = (_top_index+1) % (_queue_size-1);
		sem_post(&_available_work);
	}
	return true;
}

bool ThreadPool::fetch_work(WorkerThread **workerArg)
{
	sem_wait(&_available_work);

	{
		ScopedMutex mutex(&_mutex_sync);
		WorkerThread * workerThread = _worker_queue[_bottom_index];
		_worker_queue[_bottom_index] = NULL;
		*workerArg = workerThread;
		if(_queue_size !=1 )
			_bottom_index = (_bottom_index+1) % (_queue_size-1);
		sem_post(&_available_thread);
	}
	return true;
}

void *ThreadPool::thread_execute(void *param)
{
	WorkerThread *worker = NULL;

	while(((ThreadPool *)param)->fetch_work(&worker))
	{
		if(worker)
		{
			worker->executeThis();
			//cout << "worker[" << worker->id << "]\tdelete address: [" << worker << "]" << endl;
			delete worker;
			worker = NULL;
		}

		{
			ThreadPool * pool = static_cast<ThreadPool*>(param);
			ScopedMutex mutex(&pool->_mutex_work_completion);
			--pool->_incomplete_work;
		}
	}
	return 0;
}

void ThreadPool::init_mutex(pthread_mutex_t* const mutex)
{
	int ret = pthread_mutex_init(mutex, NULL);
	switch (ret) {
	case 0:
		break;
	case EAGAIN:
		throw Error("EAGAIN returned by pthread_mutex_init()");
	case ENOMEM:
		throw Error("ENOMEM returned by pthread_mutex_init()");
	case EPERM:
		throw Error("EPERM returned by pthread_mutex_init()");
	case EBUSY:
		throw Error("EBUSY returned by pthread_mutex_init()");
	case EINVAL:
		throw Error("EINVAL returned by pthread_mutex_init()");
	default:
		throw Error("UNKNOWN returned by pthread_mutex_init()");
	};
}

Error::Error(const char * what)
:_what(what)
{
}

Error::~Error() throw()
{
}

const char* Error::what() const throw()
{
	return _what;
}
