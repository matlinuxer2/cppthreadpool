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

#include <pthread.h>
#include <semaphore.h>
#include <iostream>
#include <vector>

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

	void destroy_pool(int maxPollSecs);

	bool assign_work(WorkerThread *worker);
	bool fetch_work(WorkerThread **worker);

	void initialize_thread();

	static void *thread_execute(void *param);

	static pthread_mutex_t mutexSync;
	static pthread_mutex_t mutexWorkCompletion;


private:
	unsigned int _num_thread;

	sem_t _available_work;
	sem_t _available_thread;

	std::vector<WorkerThread *> _worker_queue;

	int _top_index;
	int _bottom_index;

	int _incomplete_work;


	int _queue_size;

};




