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
#include <cassert>

#include <errno.h>

class ScopedMutex {
public:
	explicit ScopedMutex(pthread_mutex_t * const mutex):_mutex(mutex)
	{
		assert(mutex);

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

const struct timespec ThreadPool::DESTROY_TIMEOUT = { 10, 0 };

ThreadPool::ThreadPool(unsigned int num_thread)
:_thread_pool(num_thread)
,_work()
{
	if (0 == num_thread) {
		throw Error("zero thread?");
	}

	init_sem(&_available_work);
	init_mutex(&_work_mutex);

	for (std::vector<pthread_t>::iterator i = _thread_pool.begin(); i != _thread_pool.end(); ++i) {
		int ret = pthread_create(&*i, NULL, &ThreadPool::thread_execute, this);
		switch (ret) {
		case 0:
			break;
		case EAGAIN:
			throw Error("EAGAIN returned by pthread_create()");
		case EINVAL:
			throw Error("EINVAL returned by pthread_create()");
		case EPERM:
			throw Error("EPERM returned by pthread_create()");
		default:
			throw Error("UNKNOWN returned by pthread_create()");
		}
	}
}

ThreadPool::~ThreadPool()
{
	/*
	 * All failures are ignored in destructor.
	 */

	int ret = 0;
	// make sure all thread finish its jobs.
	for (unsigned int i = 0; i < _thread_pool.size(); ++i) {
		ret = sem_timedwait(&_available_work, &DESTROY_TIMEOUT);
		if (0 != ret) {
			std::cerr << "Timeout, stop ThreadPool" << std::endl;
			break;
		}
	}
	ret = sem_destroy(&_available_work);
	if (0 != ret) {
		std::cerr << errno << " returned by sem_destory(). Ignore" << std::endl;
	}

	ret = pthread_mutex_destroy(&_work_mutex);
	if (0 != ret) {
		std::cerr << ret << " returned by pthread_mutex_destroy(). Ignore" << std::endl;
	}
}


void ThreadPool::assign_work(WorkerThread *workerThread)
{
	if (!workerThread) {
		throw Error("null?");
	}

	ScopedMutex mutex(&_work_mutex);
	_work.push_back(workerThread);
	post_sem(&_available_work);
}

WorkerThread * ThreadPool::fetch_work()
{
	wait_sem(&_available_work);
	ScopedMutex mutex(&_work_mutex);
	WorkerThread* work = _work.front();
	_work.pop_front();
	return work;
}

void * ThreadPool::thread_execute(void *param)
{
	ThreadPool *pool = static_cast<ThreadPool*>(param);

	for (;;) {
		WorkerThread* worker = pool->fetch_work();
		worker->executeThis();
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

void ThreadPool::init_sem(sem_t* const sem)
{
	int ret = sem_init(sem, 0, 0);
	if (0 != ret) {
		switch (errno) {
		case EINVAL:
			throw Error("EINVAL returned by sem_init()");
		case ENOSYS:
			throw Error("ENOSYS returned by sem_init()");
		default:
			throw Error("UNKNOWN returned by sem_init()");
		}
	}
}

void ThreadPool::post_sem(sem_t* const sem)
{
	int ret = sem_post(sem);
	if (0 != ret) {
		switch (errno) {
		case EINVAL:
			throw Error("EINVAL returned by sem_post()");
		case EOVERFLOW:
			throw Error("EOVERFLOW returned by sem_post()");
		default:
			throw Error("UNKNOWN returned by sem_post()");
		}
	}
}

void ThreadPool::wait_sem(sem_t* const sem)
{
	for (;;) {
		int ret = sem_wait(sem);
		if (0 == ret) {
			return;
		}

		switch (errno) {
		case EINTR:
			break;
		case EINVAL:
			throw Error("EINVAL returned by sem_wait()");
		default:
			throw Error("UNKNOWN returned by sem_wait()");
		}
	}
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
