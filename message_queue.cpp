#include <functional>
#include <condition_variable>
#include <iostream>
#include <thread>
#include <vector>
#include <unordered_map>
#include <future>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <pthread.h>
template<class ReturnType,class Ident, class... Arguments>
class message_queue
{
	private:

		std::thread executing;
		std::queue<std::tuple<std::function<ReturnType(Arguments...)>,Ident, Arguments...>> job_queue;
		std::unordered_map<Ident,ReturnType> return_values;
		std::unordered_map<Ident,bool> added;
		std::mutex retrieval_lock;
		std::mutex queue_lock;
		std::condition_variable retrieval_check_cv;
		sem_t job_sem; // semaphore controlling the count of elements in the job_queue
		bool is_done;

	public:
		

		ReturnType get(Ident key)
		{
			std::unique_lock<std::mutex> unique_retrieval(retrieval_lock);

			//unique_retrieval.lock(); // acquire the lock
			while(added.count(key) == 0)
			{

					
					retrieval_check_cv.wait(unique_retrieval); // block unti the return value is signalled

					

			}
			if(added.count(key) == 0) // something weird happened here.. unlock and throw an exception
			{
				unique_retrieval.unlock();	
				throw 1;

			}

			// perform an atomic operation on these data structures
			ReturnType r1 = return_values[key];
			return_values.erase(key);
			added.erase(key);


			unique_retrieval.unlock(); // unlock the mutex
			return r1;
		} 
		bool hasFinished(Ident key)
		{
			retrieval_lock.lock();
			return added.count(key) > 0;
			retrieval_lock.unlock();
		}
		message_queue()
		{
			is_done = false;
			sem_init(&job_sem,0,0);
			executing = std::thread([&]()
			{
				while(!is_done) // while this message queue is still alive
				{
				/*	int queue_size = 0;
					queue_lock.lock();
					queue_size = job_queue.size(); // get the current size, don't want race conditions when adding
					queue_lock.unlock();
				*/
				  // while(queue_size > 0)
				  //{
				  	   
					  	   sem_wait(&job_sem); // wait for the semaphore to have a value > 0
					  	   
					   if(is_done) return;
				  	   auto thread_lambda_work = [&]()
				  	   {
				  	   		
							
					  	   queue_lock.lock();
					  	   
						   auto nxt_func = job_queue.front();
						   job_queue.pop();
						   queue_lock.unlock();
						   
							//retrieval_lock.lock();
						   auto returnVal = std::async(std::launch::async, std::get<0>(nxt_func),std::get<2>(nxt_func)).get();
						   retrieval_lock.lock();
						   return_values[std::get<1>(nxt_func)] = returnVal;
						   added[std::get<1>(nxt_func)] = true;
						   retrieval_lock.unlock();
						  
						 
						   retrieval_check_cv.notify_one(); // no need to hold a mutex lock
					   };

			//		   std::thread launcher(thread_lambda_work);
			//		   launcher.detach();
						std::async(std::launch::async,thread_lambda_work);
					   

				 // }		
				}

			});		
	
		}
		void destroy()
		{
			is_done = true;
			if(executing.joinable())
				executing.join();
			added.clear();
			return_values.clear();
			job_sem = NULL;
		}
		void add_message(std::function<ReturnType(Arguments...)> ftn,Ident key, Arguments... args)
		{
		
			queue_lock.lock();
			
			job_queue.push(std::make_tuple(ftn,key,args...));
			sem_post(&job_sem);
			queue_lock.unlock();
			
			
	
		}
		int jobsLeft()
		{
			queue_lock.lock();
			return job_queue.size();
			queue_lock.unlock();
		}
		~message_queue()
		{
			is_done = true;
			added.clear();
			return_values.clear();
			sem_post(&job_sem);
			sem_destroy(&job_sem);
			if(executing.joinable())
			{
				executing.join();
			}
			
		}



};




