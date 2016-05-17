#ifndef SWITCH_RECYCLING_FACTORY_H
#define SWITCH_RECYCLING_FACTORY_H


/**
 * 可回收产品的工厂类 v0.11
 * 1. 重复利用产品
 * 2. 仅支持单一产品
 *
 * 欢迎补充！
 **/

#include <string>
#include <map>
#include <vector>
#include <list>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/serialization/singleton.hpp>


#ifndef NAMESPACE_SWITCH_TOOL
    #define NAMESPACE_SWITCH_TOOL
    #define OPEN_NAMESPACE_SWITCHTOOL       namespace Switch { \
												namespace Tool {
    #define CLOSE_NAMESPACE_SWITCHTOOL      	}; \
											};
    #define USING_NAMESPACE_SWITCHTOOL      using namespace Switch::Tool;
#endif

#define SWITCH_RECYCLING_FACTORY_DEFAULT_MAXSIZE			100
#define SWITCH_RECYCLING_FACTORY_DEFAULT_MINSIZE			50


OPEN_NAMESPACE_SWITCHTOOL


class Product
{
public:
	virtual void Recycling(void) = 0;
	virtual ~Product(void) {}
};

template < typename ProductType, typename ProductName = std::string >
class ProductContainer
{
public:
	ProductContainer(const ProductName& name) : __sName(name) { __pType = new ProductType; }
	~ProductContainer(void) { delete __pType; }
	ProductContainer(const ProductContainer< ProductType, ProductName >& product)
	{
        this->__sName = product.__sName;
        this->__pType = new ProductType;
        *this->__pType = *product.__pType;
	}
	ProductContainer< ProductType, ProductName >& operator=(const ProductContainer< ProductType, ProductName >& product)
	{
        this->__sName = product.__sName;
        this->__pType = new ProductType;
        *this->__pType = *product->__pType;
	}
	const ProductName& GetName(void) const { return __sName; }
	ProductType* GetProduct(void) const { return __pType; }

private:
	ProductName 	__sName;
	ProductType*	__pType;
};

template< typename ProductType, typename ProductName = std::string >
class RecyclingFactory
{
public:
	typedef ProductContainer< ProductType, ProductName > TheProduct;

	TheProduct* Produce(const ProductName& name)
	{
		boost::lock_guard< boost::mutex > guard(m_oWaitMutex);
		typename std::map< ProductName, std::list < TheProduct* > >::iterator m_iter;

		m_iter = m_mapProducts.find(name);
		if (m_iter == m_mapProducts.end() || m_iter->second.empty())
		{
            return ProduceNew(name);
		}
		return ProduceIdle(m_iter->second);
	}

	void Duplicate(const TheProduct* product, const size_t count = 1)
	{
		typename std::map< ProductName, std::list< TheProduct* > >::iterator m_iter;

		assert(IsValidProduct(product));
        for (size_t i = 0; i < count; ++i)
		{
			TheProduct* p = new TheProduct(*product);
			p->GetProduct()->Recycling();
			m_lIdleList.push_back(p);
			m_setProducts.insert(p);
			m_iter = m_mapProducts.find(p->GetName());
			if (m_iter == m_mapProducts.end())
			{
				std::list< TheProduct* > lTmp;
				lTmp.push_back(p);
				m_mapProducts.insert(make_pair(p->GetName(), lTmp));
			}
			else
			{
				m_iter->second.push_back(p);
			}
		}
	}

	void Recycling(const TheProduct* product)
	{
		typename std::list< TheProduct* >::iterator s_iter, s_end;
		typename std::map< ProductName, std::list< TheProduct* > >::iterator m_iter;

		boost::lock_guard< boost::mutex > guard(m_oWaitMutex);
		assert(product);

		if (!IsValidProduct(product))
		{
			return;
		}
		s_end = m_lBusyList.end();
		for(s_iter = m_lBusyList.begin(); s_iter != s_end; ++s_iter)
		{
			if (*s_iter == product)
			{
				break;
			}
		}
		if (s_iter == s_end)
		{
			return;
		}
		m_lBusyList.erase(s_iter);
		product->GetProduct()->Recycling();
		m_lIdleList.push_back((TheProduct*)product);
		m_iter = m_mapProducts.find(product->GetName());
		if (m_iter == m_mapProducts.end())
		{
			std::list< TheProduct* > lTmp;
			lTmp.push_back((TheProduct*)product);
			m_mapProducts.insert(make_pair(product->GetName(), lTmp));
		}
		else
		{
			m_iter->second.push_back((TheProduct*)product);
		}
	}

	void Destroy(const TheProduct* product)
	{
		typename std::list< TheProduct* >::iterator s_iter;
		typename std::map< ProductName, std::list< TheProduct* > >::iterator m_iter;

		boost::lock_guard< boost::mutex > guard(m_oWaitMutex);
		assert(product);

		if (!IsValidProduct(product))
		{
			return;
		}
		m_lBusyList.remove((TheProduct*)product);
		m_lIdleList.remove((TheProduct*)product);
		m_iter = m_mapProducts.find(product->GetName());
		if (m_iter != m_mapProducts.end())
		{
			m_iter->second.remove((TheProduct*)product);
		}
		m_setProducts.erase((TheProduct*)product);
		delete product;
	}

public:
    RecyclingFactory(void) {}
    ~RecyclingFactory(void)
    {
		typename std::set< TheProduct* >::iterator iter, end;

		m_lIdleList.clear();
		m_lBusyList.clear();
		m_mapProducts.clear();
		end = m_setProducts.end();
		for (iter = m_setProducts.begin(); iter != end; ++iter)
		{
            delete *iter;
		}
		m_setProducts.clear();
    }

private:
	inline TheProduct* ProduceNew(const ProductName& name)
	{
		TheProduct* p = new TheProduct(name);
		m_lBusyList.push_back(p);
		m_setProducts.insert(p);
		return p;
	}
	inline TheProduct* ProduceIdle(std::list< TheProduct* >& nameIdlelist)
	{
		assert(!nameIdlelist.empty());
		TheProduct* p = nameIdlelist.front();
		assert(p && p->GetProduct());
		nameIdlelist.pop_front();
		m_lIdleList.remove(p);
		m_lBusyList.push_back(p);
		return p;
	}
	inline bool IsValidProduct(const TheProduct* p)
	{
		assert(p);
		return m_setProducts.find((TheProduct*)p) == m_setProducts.end() ? false : true;
	}

private:
	boost::mutex										m_oWaitMutex;
	std::list< TheProduct* >							m_lIdleList;			// 空闲列表
	std::list< TheProduct* > 							m_lBusyList;			// 忙碌列表
	std::map< ProductName, std::list< TheProduct* > >	m_mapProducts;			// 产品名称 -- 空闲产品列表
	std::set< TheProduct* >								m_setProducts;			// 所有产品
};


/// 单例
template< typename ProductType, typename ProductName = std::string >
class RecyclingFactorySingleton : public RecyclingFactory< ProductType, ProductName >, \
								public boost::serialization::singleton< RecyclingFactorySingleton< ProductType, ProductName > >
{
};

CLOSE_NAMESPACE_SWITCHTOOL


#endif // SWITCH_RECYCLING_FACTORY_H

