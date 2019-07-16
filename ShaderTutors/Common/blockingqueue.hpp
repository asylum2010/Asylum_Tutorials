
#ifndef _BLOCKINGQUEUE_HPP_
#define _BLOCKINGQUEUE_HPP_

#include <deque>
#include <mutex>

template <typename value_type>
class BlockingQueue
{
private:
	std::deque<value_type> queue;
	mutable std::mutex lock;
	std::condition_variable condition;
	
public:
	void Push(const value_type& value);
	value_type Pop();

	bool Empty() const;
	size_t Size() const;
};

template <typename value_type>
void BlockingQueue<value_type>::Push(const value_type& value)
{
	std::unique_lock<std::mutex> guard(lock);

	queue.push_back(value);
	condition.notify_one();
}

template <typename value_type>
value_type BlockingQueue<value_type>::Pop()
{
	std::unique_lock<std::mutex> guard(lock);
	
	condition.wait(guard, [=] {
		return !queue.empty();
	});
	
	value_type result = queue.front();
	queue.pop_front();
	
	return result;
}

template <typename value_type>
bool BlockingQueue<value_type>::Empty() const
{
	std::unique_lock<std::mutex> guard(lock);
	return queue.empty();
}

template <typename value_type>
size_t BlockingQueue<value_type>::Size() const
{
	std::unique_lock<std::mutex> guard(lock);
	return queue.size();
}

#endif
