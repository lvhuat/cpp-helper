#ifndef _CHANNEL_H_
#define _CHANNEL_H_
#include <mutex>
#include <queue>
#include <condition_variable>

// 队列已经关闭，导致返回
#define ERR_CHANNEL_CLOSED  1001
// 队列满导致放置超时，或者队列空导致获取超时
#define ERR_CHANNEL_TIMEOUT 1002 
// 触发UNTIL条件
#define ERR_CHANNEL_UNTIL_OK 1003

namespace helper {
// 消息队列
// 用于生产者和消费者的安全队列
template <class T>
class channel
{
private:
	std::mutex m_mutex;
	std::queue<T> m_queue;
	std::condition_variable m_cond;
	int m_max;
	bool m_closed;
public:
	channel(int max = 1)
		: m_max(max), m_closed(false){

		if (m_max <= 0) {
			m_max = 1;
		}
	}

	// 判断channel是否已经关闭
	bool closed() { return m_closed; }

	// 关闭一个channel，导致get立即返回NULL,
	void close() {
		m_closed = true;
		m_cond.notify_all();
	}

	size_t length() {
		std::lock_guard<std::mutex> guard(m_mutex);
		return m_queue.size();
	}

	// 生产
	template<class _Rep,class _Period>
	bool put_for(T &item,std::chrono::duration<_Rep,_Period> timeout)
	{
		// 判断关闭
		if (m_closed)
		{
			errno = ERR_CHANNEL_CLOSED;
			return false;
		}

		while (true)
		{
			std::unique_lock<std::mutex> locker(m_mutex);
			auto ret = m_cond.wait_for(
				locker,
				timeout,
				[this]() { return this->m_queue.size() < m_max || this->m_closed; }
			);


			if (!ret) {
				// 消费不及时
				errno = ERR_CHANNEL_TIMEOUT;
				return false;
			}

			// channel已经关闭
			if (m_closed) {
				// channel已经被关闭
				errno = ERR_CHANNEL_CLOSED;
				return false;
			}


			m_queue.push(item);
			m_cond.notify_all();

			return true;
		}

		return false;
	}

	// 消费
	// 队列中没有数据时会阻塞
	template<class _Rep, class _Period>
	T get(std::chrono::duration<_Rep, _Period> timeout = std::chrono::hours(100000))
	{
		if (m_closed) {
			errno = ERR_CHANNEL_CLOSED;
			return nullptr;
		}

		while (true)
		{
			std::unique_lock<std::mutex> locker(m_mutex);
			// 等待被唤醒(notify_*)，唤醒后会判断条件，条件为真则返回
			auto ok = m_cond.wait_for(
				locker,
				timeout,
				[this]() { return m_queue.size() > 0 || m_closed; }
			);
			if (!ok) {
				errno = ERR_CHANNEL_TIMEOUT;
				return nullptr;
			}


			T back = nullptr;
			if (m_closed) {
				errno = ERR_CHANNEL_CLOSED;
				return nullptr;
			} else {
				if (m_queue.size() > 0) {
					back = m_queue.front();
					m_queue.pop();
				}
			}

			m_cond.notify_all();

			return back;
		}

		return nullptr;
	}
};
}
#endif //_CHANNEL_H_