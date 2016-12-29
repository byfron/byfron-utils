#include "gtest.h"
#include "Profiler.hpp"
#include <unistd.h>

unsigned int microseconds;

TEST(TestProfiler, Profiler) {

	int mcs = 2000000;
	
	{
	Profiler p("P1");
	usleep(mcs);
	}

	Profiler::print();
}

// TEST(TestProfiler, Paralel) {

// 	#pragma omp parallel for	
// 	for (int i = 0; i < 3; i++)
// 	{
// 		Profiler p("testing no par");
// 		sleep(1);
// 	}

// #pragma omp parallel for	
// 	for (int i = 0; i < 4; i++)
// 	{
// 		Profiler p("testing");
// 		sleep(1);

// 		{
// 			Profiler p2("son of testing sadasd asdf asdf adsf");
// 			sleep(2);
// 		}

// 		{
// 			Profiler p3("brother son of t");
// 			sleep(1);
// 		}
// 	}

// 	// {
// 	// 	Profiler p("testing");
// 	// 	sleep(2);
// 	// }
	
// 	{
// 		Profiler p("testing second");
// 		sleep(1);
// 	}

// 	Profiler::print();
// 	Profiler::clear();
// }

