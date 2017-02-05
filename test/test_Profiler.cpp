#include "gtest.h"
#include "Profiler.hpp"
#include <unistd.h>

using namespace ByfronUtils;
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

	{
		mcs = 150000;
		__PROF(P4);
		usleep(mcs);
		__STOP(P4);
		mcs = 100000;
		usleep(mcs);
		
	}

	EXPECT_EQ(Profiler::getTimeInMilis("P1"), 200.0);
	EXPECT_EQ(Profiler::getTimeInMilis("P2"), 500.0);
	EXPECT_EQ(Profiler::getTimeInMilis("P3"), 100.0);
	EXPECT_EQ(Profiler::getTimeInMilis("P4"), 150.0);
	EXPECT_EQ(Profiler::getTimeInMilis("__root__"), 850.0);

	Profiler::print();
	Profiler::clear();

}

TEST(TestProfiler, NumCalls) {

	int mcs = 20000;

	for (int i = 0; i < 5; i++)
	{
		__PROF(P1);
		usleep(mcs);		
	}
	
	{
		__PROF(P2);
		usleep(mcs);
	}

	{
		__PROF(P1);
		usleep(mcs);		
	}

	EXPECT_EQ(Profiler::getNumCalls("P1"), 6);
	EXPECT_EQ(Profiler::getNumCalls("P2"), 1);
	EXPECT_EQ(Profiler::getNumCalls("__root__"), 1);
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

	{
		mcs = 200000;
		__PROF(P2)
		usleep(mcs);
	}
				

	EXPECT_EQ(Profiler::getTimeInMilis("P1"), 500.0d);
	EXPECT_EQ(Profiler::getTimeInMilis("P2"), 200.0d);

	std::vector<Profiler::Stats> fstats = Profiler::getFusedStats();
	for (auto s : fstats) {
		if (s.key == "P1") {
			EXPECT_EQ(Profiler::getTimeInMilis(s), 4*500.0d);
		}
	}
		
	Profiler::print();
	Profiler::clear();

}

