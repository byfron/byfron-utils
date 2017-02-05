#include "gtest.h"
#include "Pool.hpp"
#include <unistd.h>

using namespace ByfronUtils;

TEST(TestPool, Pool) {

	std::cout << "Pool" << std::endl;
	std::shared_ptr<Pool<int> > pool = std::make_shared<Pool<int> >();
	EXPECT_TRUE(pool->empty());

	pool->add(std::unique_ptr<int>(new int(45)));
	EXPECT_TRUE(not pool->empty());

	{
	auto v = pool->acquire();
	EXPECT_TRUE(pool->empty());
	EXPECT_TRUE(*v == 45);
	}

	EXPECT_TRUE(not pool->empty());	
}
