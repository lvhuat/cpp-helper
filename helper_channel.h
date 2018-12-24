#ifndef _CHANNEL_H_
#define _CHANNEL_H_
#include <mutex>
#include <queue>
#include <condition_variable>

// �����Ѿ��رգ����·���
#define ERR_CHANNEL_CLOSED  1001
// ���������·��ó�ʱ�����߶��пյ��»�ȡ��ʱ
#define ERR_CHANNEL_TIMEOUT 1002 
// ����UNTIL����
#define ERR_CHANNEL_UNTIL_OK 1003

namespace helper {
// ��Ϣ����
// ���������ߺ������ߵİ�ȫ����
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

	// �ж�channel�Ƿ��Ѿ��ر�
	bool closed() { return m_closed; }

	// �ر�һ��channel������get��������NULL,
	void close() {
		m_closed = true;
		m_cond.notify_all();
	}

	size_t length() {
		std::lock_guard<std::mutex> guard(m_mutex);
		return m_queue.size();
	}

	// ����
	template<class _Rep,class _Period>
	bool put_for(T &item,std::chrono::duration<_Rep,_Period> timeout)
	{
		// �жϹر�
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
				// ���Ѳ���ʱ
				errno = ERR_CHANNEL_TIMEOUT;
				return false;
			}

			// channel�Ѿ��ر�
			if (m_closed) {
				// channel�Ѿ����ر�
				errno = ERR_CHANNEL_CLOSED;
				return false;
			}


			m_queue.push(item);
			m_cond.notify_all();

			return true;
		}

		return false;
	}

	// ����
	// ������û������ʱ������
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
			// �ȴ�������(notify_*)�����Ѻ���ж�����������Ϊ���򷵻�
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