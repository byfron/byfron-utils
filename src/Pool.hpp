#pragma once
#include <memory>
#include <queue>

namespace ByfronUtils {
	
template <typename T>
class Pool : public std::enable_shared_from_this< Pool<T> >{

private:
	struct ExternalDeleter {
		explicit ExternalDeleter(std::weak_ptr<Pool<T> > pool)
			: m_pool(pool) {}

		void operator()(T* ptr) {
			if (auto pool_ptr = m_pool.lock()) {
				try {
					(pool_ptr->add(std::unique_ptr<T>(ptr)));
					return;
				} catch(...) {}
			}
			std::default_delete<T>{}(ptr);
		}
	private:
		std::weak_ptr<Pool<T> > m_pool;
	};

	std::queue<std::unique_ptr<T> > m_pool;

public:

	using PtrType = std::unique_ptr<T, ExternalDeleter>;	

	Pool() {}

	void add(std::unique_ptr<T> t) {
		m_pool.push(std::move(t));
	}
	
	PtrType acquire() {
		assert(!m_pool.empty());
		PtrType tmp(m_pool.front().release(), ExternalDeleter(this->shared_from_this()));
		m_pool.pop();
		return std::move(tmp);
	}

	bool empty() const {
		return m_pool.empty();
	}

	size_t size() const {
		return m_pool.size();
	}
	
};


}
