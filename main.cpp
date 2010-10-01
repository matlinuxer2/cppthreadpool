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

#include <iostream>
#include <string>

// for pipe
#include <unistd.h> 

// for select
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

// for strcpy
#include <string.h>


#include "threadpool.h"

using namespace std;

#define BUFFER_SIZE 1024

ThreadPool *myPool = NULL; 


string callee( char* name ){
	string result;

	cout << "		callee>> hello world,..." << name << endl; 
	
	//sleep( 1 );

	result = result + name + " is a good guy";

	return result;
}


class SampleWorkerThread : public WorkerThread
{
public:
	int id;
	string _data;
	int _pipefd[2];

	unsigned virtual executeThis()
	{
	// Instead of sleep() we could do anytime consuming work here.
	//Using ThreadPools is advantageous only when the work to be done is really time consuming. (atleast 1 or 2 seconds)
           cout << "	->Processing SampleWorkerThread " << id << "\t address=" << this << endl;
		char *param = const_cast<char*>(_data.c_str());
		string result = callee( param );

		char buf2[BUFFER_SIZE];
		strcpy( buf2, result.c_str() );
		
		cout << "	writing..." << buf2 << "  to pipefd," << _pipefd[1] << endl;

           cout << "	<-Processing SampleWorkerThread " << id << "\t address=" << this << endl;

		write( _pipefd[1], &buf2, strlen( result.c_str() ) );

		return(0);
	}


        SampleWorkerThread(int id, int pipe_fd_read, int pipe_fd_write, string data ) : WorkerThread(id), id(id)
        {
	   _pipefd[0] = pipe_fd_read;
	   _pipefd[1] = pipe_fd_write;
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
	
	SampleWorkerThread* wthr = new SampleWorkerThread( 9999, pipefd[0], pipefd[1], name );

	// 丟給 thread pool 執行
	myPool->assignWork( wthr );


	fd_set rfds;
	struct timeval tv;
	int retval;
	int fd_max;

	/* Watch pipefd[1] to see when it has input. */
	FD_ZERO(&rfds);
	FD_SET( pipefd[0], &rfds);

	/* Wait up to five seconds. */
	tv.tv_sec = 7;
	tv.tv_usec = 0;

	fd_max = pipefd[1]+1;

	// 用 select 等待回傳
	cout << "等待回傳中...in 7 sec..." << pipefd[0] << "," << pipefd[1] << "," << fd_max << endl;
	retval = select( fd_max, &rfds, NULL, NULL, &tv);
	/* Don't rely on the value of tv now! */


	if( retval > 0 ){
		cout << "回傳觸發了!!" << endl;
		/* FD_ISSET(0, &rfds) will be true. */

		// 處理回傳值
		char buf2[BUFFER_SIZE];
		ssize_t read_bytes = read( pipefd[0], &buf2, BUFFER_SIZE ); // 接 callee 回傳的資料

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
	string result = caller( "Mat multi-threads" );
	cout << "MT:>>>" << result << "<<<" << endl;
}



int main(int argc, char **argv)
{
	myPool = new ThreadPool(1); // single thread;
	//myPool = new ThreadPool(5); // multi thread;
	myPool->initializeThreads();

	//We will count time elapsed after initializeThreads()
    time_t t1=time(NULL);

	pthread_t thread_id;
        pthread_create( &thread_id, NULL, &thread_start,NULL);
        pthread_create( &thread_id, NULL, &thread_start,NULL);
        pthread_create( &thread_id, NULL, &thread_start,NULL);
        pthread_create( &thread_id, NULL, &thread_start,NULL);
        pthread_create( &thread_id, NULL, &thread_start,NULL);
        pthread_create( &thread_id, NULL, &thread_start,NULL);
        pthread_create( &thread_id, NULL, &thread_start,NULL);
        pthread_create( &thread_id, NULL, &thread_start,NULL);


	string result = caller( "Mat" );
	cout << ">>>" << result << "<<<" << endl;

	string result1 = caller( "Matt" );
	cout << ">>>" << result1 << "<<<" << endl;

	string result2 = caller( "Matttt" );
	cout << ">>>" << result2 << "<<<" << endl;

	string result3 = caller( "Mattttttt" );
	cout << ">>>" << result3 << "<<<" << endl;
	
    time_t t2=time(NULL);


    // 注意，切換不同的 Worker thread 數，可以看到時間上的差異
    cout << t2-t1 << " seconds elapsed\n" << endl;

	//sleep( 3 );
	// destroyPool(int maxPollSecs)
	// Before actually destroying the ThreadPool, this function checks if all the pending work is completed.
	// If the work is still not done, then it will check again after maxPollSecs
	// The default value for maxPollSecs is 2 seconds.
	// And ofcourse the user is supposed to adjust it for his needs.
	myPool->destroyPool(2);

	delete myPool;
	
    return 0;
}
