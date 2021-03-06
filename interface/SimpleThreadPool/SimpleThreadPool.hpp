#ifndef SIMPLE_THREAD_POOL_HPP
#define SIMPLE_THREAD_POOL_HPP

/**
 * 一个简单的线程池 v0.3
 * 采用boost线程库
 * 线程函数须采用boost线程库编写
 * 线程函数如有参数，可使用boost::bind传入参数
 * 编译时链接libboost_thread和libboost_system
 *
 * 分为两种模式，函数模式和任务模式。
 * 函数模式，为每一个传入的函数创建一个线程，函数返回则线程退出；
 * 任务模式，将函数封装为任务，任务在线程池内排队，按顺序执行，
 *				执行完成后，任务退出，线程等待。
 *
 * 欢迎补充！
 **/

#include <list>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/bind.hpp>
#include <SimpleJob.hpp>
#include <SimpleThreadList.hpp>


#ifndef NAMESPACE_SWITCH_TOOL
    #define NAMESPACE_SWITCH_TOOL
    #define OPEN_NAMESPACE_SWITCHTOOL       namespace Switch { \
												namespace Tool {
    #define CLOSE_NAMESPACE_SWITCHTOOL      	}; \
											};
    #define USING_NAMESPACE_SWITCHTOOL      using namespace Switch::Tool;
#endif


OPEN_NAMESPACE_SWITCHTOOL


typedef float 	SIMPLE_FUNCTION_MODE;
typedef double 	SIMPLE_JOB_MODE;


template < typename T >
class SimpleThreadPool;

template <>
class SimpleThreadPool < SIMPLE_FUNCTION_MODE >
{
public:
	SimpleThreadPool(void)
	{
		m_nMaxPoolSize = 0;
		m_pJoinThread = new boost::thread(boost::bind(&SimpleThreadPool::ThreadJoin, this));
	}

	SimpleThreadPool(unsigned int size)
	{
        m_nMaxPoolSize = 0;

		m_pJoinThread = new boost::thread(boost::bind(&SimpleThreadPool::ThreadJoin, this));
	}

	~SimpleThreadPool(void)
	{
	    m_pJoinThread->interrupt();
	    m_pJoinThread->join();
	    delete m_pJoinThread;

        StopThreadPool();
	}

public:
	template < typename T >
	int	AddThread(T func)
	{
	    m_TMutex.lock();
		if (m_lpThread.size() >= m_nMaxPoolSize) {
            m_TMutex.unlock();
			return -1;
		}

        m_lpThread.push_back(new boost::thread(func));
        m_TMutex.unlock();

		return 0;
	}

    /// 停止线程池
	int StopThreadPool(void)
	{
        std::list< boost::thread* >::iterator iter, end;

        end = m_lpThread.end();
        for (iter = m_lpThread.begin(); iter != end;)
        {
            boost::thread* p = *iter;
            p->interrupt();
            p->join();
            delete p;
        }

        return 0;
	}

	/// 设置线程池大小
	int SetMaxPoolSize(unsigned int size)
	{
        m_TMutex.lock();
        if (size > m_nMaxPoolSize || size >= m_lpThread.size())
        {
            m_nMaxPoolSize = size;
            m_TMutex.unlock();
            return 0;
        }
        m_TMutex.unlock();

        return -1;
	}

	/// 获取线程池大小
	int GetMaxPoolSize(void)
	{
	    m_TMutex.lock();
	    int size = m_nMaxPoolSize;
	    m_TMutex.unlock();

		return size;
	}

	/// 获取线程池当前大小
	int GetPoolSize(void)
	{
	    m_TMutex.lock();
	    int size = m_lpThread.size();
	    m_TMutex.unlock();

		return size;
	}

private:
    void ThreadJoin(void)
    {
        std::list< boost::thread* >::iterator iter, end;

        while(1)
        {
            m_TMutex.lock();
            end = m_lpThread.end();
            for (iter = m_lpThread.begin(); iter != end;)
            {
                if ((*iter)->timed_join(boost::posix_time::microseconds(1)))
                {
                    boost::thread* p = *iter;
                    iter = m_lpThread.erase(iter);
                    delete p;
                }
                else
                {
                    ++iter;
                }
            }
            m_TMutex.unlock();

            boost::this_thread::sleep(boost::posix_time::milliseconds(1));
        }
    }

private:
    boost::mutex                    m_TMutex;
	std::list< boost::thread* >		m_lpThread;
	boost::thread*                  m_pJoinThread;
	unsigned int 					m_nMaxPoolSize;
};

template <>
class SimpleThreadPool < SIMPLE_JOB_MODE >
{
public:
	SimpleThreadPool(void)
	{
		m_oTrdList.set_thread_func(boost::bind(&SimpleThreadPool::dispatch_thread, this));
		m_bStopFlag = false;
	}

	SimpleThreadPool(unsigned int max_thread_size, unsigned int max_job_size)
	{
		m_oTrdList.set_thread_func(boost::bind(&SimpleThreadPool::dispatch_thread, this));
		m_oTrdList.set_max_thread_num(max_thread_size);
		m_oJobList.set_max_job_num(max_job_size);
		m_bStopFlag = false;
	}

	bool set_max_thread_size(unsigned int max)
	{
		return m_oTrdList.set_max_thread_num(max);
	}

	bool set_max_job_size(unsigned int max)
	{
		return m_oJobList.set_max_job_num(max);
	}

	int get_max_thread_size(void)
	{
		return m_oTrdList.get_max_thread_num();
	}

	int get_max_job_size(void)
	{
		return m_oJobList.get_max_job_num();
	}

	int get_current_job_num(void)
	{
		return m_oJobList.get_current_job_num();
	}

	bool add_job_nonblock(const SimpleJob< void >& job)
	{
		m_oJobList.lock_add_job();
		bool ret = m_oJobList.push_back(job);
		m_oJobList.unlock_add_job();
		if (ret)
		{
			m_oJobList.notify_one_get_job();
		}

		return ret;
	}

	bool add_job_block(const SimpleJob< void >& job)
	{
		m_oJobList.lock_add_job();
		m_oJobList.wait_add_job();
		if (m_bStopFlag)
		{
			m_oJobList.unlock_add_job();
			return false;
		}
		bool ret = m_oJobList.push_back(job);
		m_oJobList.unlock_add_job();
		if (ret)
		{
			m_oJobList.notify_one_get_job();
		}

		return ret;
	}

	void stop(void)
	{
		m_bStopFlag = true;
		m_oJobList.stop();
		m_oTrdList.stop();
	}

private:
	void dispatch_thread(void)
	{
		SimpleJob< void > job;

		while (1)
		{
			m_oJobList.lock_get_job();
			m_oJobList.wait_get_job();
			if (m_bStopFlag)
			{
				m_oJobList.unlock_get_job();
				return;
			}
			m_oJobList.pop_front(job);
			m_oJobList.unlock_get_job();
			m_oJobList.notify_all_add_job();
			job.CallJob();
		}
	}

private:
    boost::mutex                    m_oMutex;
	SimpleJobList                   m_oJobList;
	SimpleThreadList				m_oTrdList;
	bool							m_bStopFlag;
};


typedef SimpleThreadPool < SIMPLE_FUNCTION_MODE >	STFPool;
typedef SimpleThreadPool < SIMPLE_JOB_MODE >		STJPool;
typedef SimpleJob< void >							SJob;


CLOSE_NAMESPACE_SWITCHTOOL


#endif // SIMPLE_THREAD_POOL_HPP

