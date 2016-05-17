#ifndef JFR_THREAD_POOL_H
#define JFR_THREAD_POOL_H


#include <string>
#include <map>
#include <vector>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/serialization/singleton.hpp>
#include <SimpleThreadPool/SimpleThreadPool.hpp>
#include "Common.h"
#include "Logger.h"


OPEN_NAMESPACE_JFR

using namespace std;
USING_NAMESPACE_SWITCHTOOL


class ThreadPool : public STJPool, public boost::serialization::singleton< ThreadPool >
{
protected:
	ThreadPool(void) {};
	~ThreadPool(void) {};
};






CLOSE_NAMESPACE_JFR



#endif // JFR_THREAD_POOL_H
