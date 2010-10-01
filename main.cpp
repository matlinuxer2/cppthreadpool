#include <iostream>
#include <string>
#include <unistd.h> 
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>


#include "threadpool.h"

using namespace std;

#define BUFFER_SIZE 1024

ThreadPool *myPool = NULL; 


string callee( char* name ){
	string result;

	srandom(time(NULL));
	int rdm = random() % 8;

	switch ( rdm ){
		case 1:
		case 2: 
		case 3:
		case 4: 
		case 5: {
			cout << "我是 short task" << endl; 
			sleep( 1 );
		} break;

		case 6: 
		case 7: {
			cout << "我是 long task" << endl; 
			sleep( 7 );
		} break;

		default: {
			cout << "我是 tiny task" << endl; 
		} break;
	}

	result = result + name + " is return by callee.";

	return result;
}


class SampleWorkerThread : public WorkerThread
{
public:
	int id;
	string _data;
	int _pipefd_return;

	unsigned virtual executeThis()
	{
		char *param = const_cast<char*>(_data.c_str());
		string result = callee( param );

		char buf2[BUFFER_SIZE];
		strcpy( buf2, result.c_str() );
		
		// 將 callee 的回傳值，透過 pipe 寫回給 caller.
		write( _pipefd_return, &buf2, strlen( result.c_str() ) );

		return(0);
	}


        SampleWorkerThread(int id, int pipefd_return, string data ) : WorkerThread(id), id(id)
        {
	   _pipefd_return = pipefd_return;
	   _data = data;

           cout << "	Creating SampleWorkerThread " << id << "\t address=" << this << endl;
        }

        ~SampleWorkerThread()
        {
           cout << "	Deleting SampleWorkerThread " << id << "\t address=" << this << endl;
        }
};


string caller ( const char * name ){


	int pipefd[2];

	if (pipe(pipefd) == -1) {
		cout << "Pipe initialize error" << endl;
		return NULL;
	}
	
	SampleWorkerThread* wthr = new SampleWorkerThread( 1, pipefd[1], name );

	// 丟給 thread pool 執行
	//myPool->assignWork( wthr ); // API: change
	myPool->assign_work( wthr );


	fd_set rfds;
	struct timeval tv;
	int retval;
	int fd_max;

	/* Watch pipefd[1] to see when it has input. */
	FD_ZERO(&rfds);
	FD_SET( pipefd[0], &rfds);

	/* Wait up to five seconds. */
	tv.tv_sec = 10;
	tv.tv_usec = 0;

	fd_max = pipefd[1]+1;

	// 用 select 等待回傳
	retval = select( fd_max, &rfds, NULL, NULL, &tv);
	/* Don't rely on the value of tv now! */


	if( retval > 0 ){
		cout << "回傳觸發了!!" << endl;
		/* FD_ISSET(0, &rfds) will be true. */

		// 處理回傳值
		char buf2[BUFFER_SIZE];
		read( pipefd[0], &buf2, BUFFER_SIZE ); // 接 callee 回傳的資料

		cout << " buffer :: " << buf2 << endl;
	
		string result = string( buf2 );

		return result;
	}
	else if ( retval == 0 ){
		cout << "等不到回傳" << endl;
		// 處理逾期
	}
	else {
		cout << "等待有錯誤" << endl;
		// 處理例外
	}

	return NULL;
}


static void * thread_start(void *arg)
{
	string result = caller( "DATA_002" );
	cout << "Noreturn Call result: " << result << endl;

	return NULL;
}



int main(int argc, char **argv)
{
	//myPool = new ThreadPool(1); // single thread;
	myPool = new ThreadPool(5); // multi thread;
	//myPool->initializeThreads(); // API: remove

	//We will count time elapsed after initializeThreads()
	time_t t1=time(NULL);

	int task_num= 100;
	pthread_t thread_id[task_num];


	for ( int i = 0; i< task_num; i++ ){

		srandom(time(NULL));
		int rdm = random() % 3;

		switch ( rdm ){
			case 0: {
				cout << "Blocking Calling" << endl;
				string result = caller( "DATA_001" );
				cout << "Blocking Call return: " << result << endl;
			} break;

			case 1: {
				cout << "Non-blocking Calling ( not yet implemented)" << endl;
				sleep (1);
			} break;

			case 2: {
				cout << "Noreturn Calling" << endl;
				pthread_create( &thread_id[i], NULL, &thread_start,NULL);
				sleep (1);
			} break;

			default: {
			} break;
		}

	}

	
	time_t t2=time(NULL);


	// 注意，切換不同的 Worker thread 數，可以看到時間上的差異
	cout << t2-t1 << " seconds elapsed\n" << endl;

	// destroyPool(int maxPollSecs)
	// Before actually destroying the ThreadPool, this function checks if all the pending work is completed.
	// If the work is still not done, then it will check again after maxPollSecs
	// The default value for maxPollSecs is 2 seconds.
	// And ofcourse the user is supposed to adjust it for his needs.
	//myPool->destroyPool(2); // API: remove

	sleep( 3 ); // 若 destructor 是強制 kill, 需設 sleep 以避免太早被關閉
	delete myPool;
	
    return 0;
}
