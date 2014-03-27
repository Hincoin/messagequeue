#include <functional>
#include <iostream>
#include <thread>
#include <vector>
#include <unordered_map>
#include <future>
#include <queue>
#include <mutex>
template<class ReturnType,class Ident, class... Arguments>
class message_queue
{
	private:

		std::thread executing;
		std::queue<std::tuple<std::function<ReturnType(Arguments...)>,Ident, Arguments...>> job_queue;

		std::unordered_map<Ident,ReturnType> return_values;
		std::unordered_map<Ident,bool> added;
		std::mutex retrieval_lock;
		
		bool isDone;

	public:
		

		ReturnType get(Ident key)
		{
			while(added.count(key) == 0){}
			retrieval_lock.lock();
			if(added.count(key) == 0)
			{
				retrieval_lock.unlock();	
				throw 1;

			}
			ReturnType r1 = return_values[key];
			return_values.erase(key);
			added.erase(key);
			retrieval_lock.unlock();
			return r1;
		} 
		message_queue()
		{
//			currentReturn =0 ;
			isDone = false;
			executing = std::thread([&](){
				
				while(!isDone)
				{

					while(job_queue.size() > 0)
					{
						std::cout << "Chugging...\n";		
						auto nxt_func = job_queue.front();
						job_queue.pop();
						std::cout << "Storing into key: " << std::get<1>(nxt_func) << "\n";						
						return_values[std::get<1>(nxt_func)] = std::async(std::launch::async, std::get<0>(nxt_func),std::get<2>(nxt_func)).get();
						added[std::get<1>(nxt_func)] = true;

					}		






				}
			


	
			});		
	
		}
		void add(std::function<ReturnType(Arguments...)> ftn,Ident key, Arguments... args)
		{
			job_queue.push(std::make_tuple(ftn,key,args...));
	
		}
		~message_queue()
		{
			isDone = true;
			added.clear();
			return_values.clear();
			if(executing.joinable())
				executing.join();

			

		}


};

