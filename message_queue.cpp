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
	//	std::thread reading;
		std::thread executing;
		std::queue<std::tuple<std::function<ReturnType(Arguments...)>,Ident, Arguments...>> job_queue;
	//	std::queue<ReturnType> return_values; 
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
		
	//		if(reading != nullptr && reading.joinable())
	//			reading.join();
			if(executing.joinable())
				executing.join();

			

		}


};
int someFunction(std::tuple<int,int> tup)
{
	return 6445 * 121 + 12231 / 43441 * 2124 - 542324 + 123442 - 5 / 212 + 47;


}
int moreMath(std::tuple<int,int> tup)
{
	return someFunction(tup) / 121 + 3 * 223 - std::get<0>(tup) + std::get<1>(tup);
	

}
int simpleMath(std::tuple<int,int> tup)
{
	return 16 * std::get<0>(tup) + std::get<1>(tup);

}

int main()
{
	message_queue<int,int,std::tuple<int,int>> m_queue;
	std::function<int(std::tuple<int,int>)> f1 = someFunction;
	std::function<int(std::tuple<int,int>)> f2 = moreMath;
	std::function<int(std::tuple<int,int>)> f3 = simpleMath;
	m_queue.add(f1,1,std::make_tuple(1,5));
	m_queue.add(f2,2,std::make_tuple(1,5));
	m_queue.add(f3,3,std::make_tuple(1,5));
//	for(int j = 0; j < 50 ; ++j)
	for(int i =0; i < 3; ++i)
	{
		std::cout << "Getting id: " << (i+1 ) << "  " <<  m_queue.get(i+1) << "\n";

	}
	


	return 0;
}
