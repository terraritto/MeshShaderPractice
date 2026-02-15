#pragma once
#include <list>
#include "MeshShaderPractice/Base/Util/SpinLock.h"
#include "MeshShaderPractice/Base/Util/ScopedLock.h"

static constexpr uint8_t DEFAULT_FRAME_COUNT = 4;

template<class T>
class Disposer
{
public:
	Disposer() : Disposer(DEFAULT_FRAME_COUNT) {}
	Disposer(uint8_t frameCount) : m_count(0) { m_lists.resize(frameCount); }
	~Disposer() { Clear(); }

	void Push(T*& object)
	{
		if (object == nullptr)
		{
			return;
		}

		// register resource
		m_lists.front().push_back(object);

		// release original pointer
		object = nullptr;

		m_count++;
	}

	void FrameSync()
	{
		// rotate to move from head to tail.
		std::rotate(m_lists.begin(), (++m_lists.begin()), m_lists.end());

		std::list<T*>& list = m_lists.front();
		auto iter = list.begin();

		while (iter != list.end())
		{
			T*& object = *iter;
			if (object != nullptr)
			{
				object->Release();
				object = nullptr;
			}

			iter = list.erase(iter);
			m_count--;
		}

		list.clear();
	}

	void Clear()
	{
		for (auto& list : m_lists)
		{
			auto iter = list.begin();

			while (iter != list.end())
			{
				T*& object = *iter;
				if (object != nullptr)
				{
					object->Release();
				}

				iter = list.erase(iter);
				m_count--;
			}

			list.clear();
		}
	}

	uint32_t GetCount() const { return m_count; }
	bool Empty() const { return m_count == 0; }

private:
	std::vector<std::list<T*>> m_lists; // destroy list
	uint32_t m_count;
};