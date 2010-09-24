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

#include <exception>

#include <pthread.h>
#include <semaphore.h>
#include <iostream>
#include <vector>
#include <list>

class Error: public std::exception {
public:
	explicit Error(const char * what);
	virtual ~Error() throw();
	virtual const char* what() const throw();
private:
	const char * _what;
};

/*
WorkerThread class
This class needs to be sobclassed by the user.
*/
class WorkerThread{
public:
	int id;

	unsigned virtual executeThis()
	{
		return 0;
	}

	WorkerThread(int id) : id(id) {}
	virtual ~WorkerThread(){}
};

/*
ThreadPool class manages all the ThreadPool related activities. This includes keeping track of idle threads and ynchronizations between all threads.
*/
class ThreadPool{
public:
	explicit ThreadPool(unsigned int num_thread);
	virtual ~ThreadPool();

	void assign_work(WorkerThread *worker);

private:
	static const struct timespec DESTROY_TIMEOUT;

	static void *thread_execute(void *param);
	static void init_mutex(pthread_mutex_t* mutex);
	static void init_sem(sem_t* sem);
	static void post_sem(sem_t* sem);
	static void wait_sem(sem_t* sem);

	WorkerThread* fetch_work();

	std::vector<pthread_t> _thread_pool;
	std::list<WorkerThread *> _work;
	pthread_mutex_t _work_mutex;
	sem_t _available_work;
};




