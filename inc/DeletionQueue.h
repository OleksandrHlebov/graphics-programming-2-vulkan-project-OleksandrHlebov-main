#pragma once
#include <stack>
#include <functional>

class DeletionQueue final
{
public:
	DeletionQueue() = default;
	~DeletionQueue()
	{
		Flush();
	}
	
	DeletionQueue(const DeletionQueue&) 				= delete;
	DeletionQueue(DeletionQueue&&) noexcept 			= delete;
	DeletionQueue& operator=(const DeletionQueue&) 	 	= delete;
	DeletionQueue& operator=(DeletionQueue&&) noexcept 	= delete;

	void Flush()
	{
		while (!m_Queue.empty())
		{
			m_Queue.top()();
			m_Queue.pop();
		}
	}

	void Push(std::function<void()> deleter)
	{
		m_Queue.push(deleter);
	}

private:
	std::stack<std::function<void()>> m_Queue;
};