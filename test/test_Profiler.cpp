#include "gtest.h"
#include "Profiler.hpp"
#include <unistd.h>

unsigned int microseconds;

TEST(TestProfiler, Profiler) {

	int mcs = 200000;
	
	{
		__PROF(P1);
		usleep(mcs);
	}

	{
		mcs = 400000;
		__PROF(P2)
		usleep(mcs);

		{
			mcs = 100000;
			__PROF(P3)
			usleep(mcs);
		}

	}

	EXPECT_EQ(Profiler::getTimeInMilis("P1"), 200.0);
	EXPECT_EQ(Profiler::getTimeInMilis("P2"), 500.0);
	EXPECT_EQ(Profiler::getTimeInMilis("P3"), 100.0);
	EXPECT_EQ(Profiler::getTimeInMilis("__root__"), 700.0);

	Profiler::print();
	Profiler::clear();

}

TEST(TestProfiler, Paralel) {

	int mcs = 500000;
	
	#pragma omp parallel for
	for (int i = 0; i < 4; i++) {
		__PROF(P1)
		usleep(mcs);
	}

	EXPECT_EQ(Profiler::getTimeInMilis("P1"), 500.0d);
	
	std::vector<Profiler::Stats> fstats = Profiler::getFusedStats();
	for (auto s : fstats) {
		if (s.key == "P1") {
			EXPECT_EQ(Profiler::getTimeInMilis(s), 4*500.0d);
		}
	}
		
	Profiler::print();
	Profiler::clear();

}

