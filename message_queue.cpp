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
#define DEBUG false
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
			#if DEBUG
				std::cout << "getting " << key << "\n";
			#endif
			std::unique_lock<std::mutex> unique_retrieval(retrieval_lock);

			//unique_retrieval.lock(); // acquire the lock
			while(added.count(key) == 0)
			{

					#if DEBUG
					std::cout << "blocking.. return value not yet specified\n";
					#endif 
					//END DEBUG
					retrieval_check_cv.wait(unique_retrieval); // block unti the return value is signalled

					#if DEBUG
						std::cout << "signal acquired in blocking call\n";
					#endif

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
				  	   #if DEBUG
						std::cout << "Creating the new lambda work\n";
					   #endif
						#if DEBUG
				  	   			std::cout << "Waiting on semaphore\n";
				  	   		#endif
					  	   sem_wait(&job_sem); // wait for the semaphore to have a value > 0
					  	   #if DEBUG
					  	   		std::cout << "Done waiting on semaphore\n";
					  	   #endif
					   if(is_done) return;
				  	   auto thread_lambda_work = [&]()
				  	   {
				  	   		#
					  	    #if DEBUG
								std::cout << "locking after sem_wait\n";
					   		#endif
					  	   queue_lock.lock();
					  	   	 #if DEBUG
						std::cout << "locked after sem_wait the new lambda work\n";
					   #endif
						   auto nxt_func = job_queue.front();
						   job_queue.pop();
						   queue_lock.unlock();
						    #if DEBUG
						std::cout << "unlocked after semwait\n";
					   #endif
							//retrieval_lock.lock();
						   auto returnVal = std::async(std::launch::async, std::get<0>(nxt_func),std::get<2>(nxt_func)).get();
						   retrieval_lock.lock();
						   return_values[std::get<1>(nxt_func)] = returnVal;
						   added[std::get<1>(nxt_func)] = true;
						   retrieval_lock.unlock();
						   #if DEBUG
						   	std::cout << "signalling  condition variable measuring " << std::get<1>(nxt_func) << "\n";
						   #endif
						 
						   retrieval_check_cv.notify_one(); // no need to hold a mutex lock
					   };

			//		   std::thread launcher(thread_lambda_work);
			//		   launcher.detach();
						std::async(std::launch::async,thread_lambda_work);
					    #if DEBUG
						std::cout << " lambda work launched\n";
					   #endif

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
			#if DEBUG
				std::cout << "Locking in add " << key << "\n";
			#endif
			queue_lock.lock();
			#if DEBUG
				std::cout << "LockED in add " << key << "\n";
			#endif
			job_queue.push(std::make_tuple(ftn,key,args...));
			sem_post(&job_sem);
			queue_lock.unlock();
			#if DEBUG
				std::cout << "unlockED in add " << key << "\n";
			#endif
			
	
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
				std::cout << "JOINING\n";
				executing.join();
			}
			
		}



};




