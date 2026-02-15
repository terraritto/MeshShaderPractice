#pragma once
#include <list>
#include <memory>
#include "MeshShaderPractice/Base/Util/SpinLock.h"
#include "MeshShaderPractice/Base/Util/ScopedLock.h"

//todo: before change

/*
static constexpr uint8_t DEFAULT_WEAK_LIFE_TIME = 4;

template<class T>
class WeakDisposer
{
public:
	WeakDisposer() { m_lock = std::make_shared<SpinLock>(); }
	~WeakDisposer() { Clear(); }

	void Push(std::weak_ptr<T>& object, uint8_t lifeTime = DEFAULT_LIFE_TIME)
	{
		if (auto p = object.lock(); !p)
		{
			return;
		}

		ScopedLock scopedLock{ m_lock };

		// register resource
		Item item = {};
		item.m_object = object;
		item.m_lifeTime = lifeTime;
		m_list.push_back(item);

		// release original pointer
		object.reset();
	}

	void FrameSync()
	{
		// Delay Release Process
		ScopedLock ScopedLock{ m_lock };
		auto iter = m_list.begin();

		while (iter != m_list.end())
		{
			// decrement lifetime
			--iter->m_lifeTime;
			if (iter->m_lifeTime <= 0)
			{
				// expired
				if (auto p = iter->m_object.lock())
				{
					p->Release();
					iter->m_object.reset();
				}

				iter = m_list.erase(iter);
			}
			else
			{
				// next
				++iter;
			}
		}
	}

	void Clear()
	{
		ScopedLock scopedLock{ m_lock };
		auto iter = m_list.begin();

		// release resource all
		while (iter != m_list.end())
		{
			if (auto p = iter->m_object.lock())
			{
				p->Release();
				iter->m_object.reset();
				iter->m_lifeTime = 0;
			}

			iter = m_list.erase(iter);
		}

		m_list.clear();
	}

private:
	struct Item
	{
		std::weak_ptr<T> m_object;
		uint8_t m_lifeTime;
	};

private:
	std::list<Item> m_list; // destroy list
	std::shared_ptr<SpinLock> m_lock; // spin lock
};
*/