#include "threadpool.h"

class ThreadPool{

	public:
		static pthread_mutex_t mutexSync;
		static pthread_mutex_t mutexWorkCompletion;

		bool fetchWork(WorkerThread **worker);

		static void *threadExecute(void *param);

	private:
		int maxThreads;

		pthread_cond_t  condCrit;
		sem_t availableWork;
		sem_t availableThreads;

		//WorkerThread ** workerQueue;
		vector<WorkerThread *> workerQueue;

		int topIndex;
		int bottomIndex;

		int incompleteWork;


		int queueSize;

};




