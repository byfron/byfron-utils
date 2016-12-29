#pragma once

#include <iostream>
#include <ctime>
#include <map>
#include <stack>
#include <algorithm>

class Profiler {

public:
	typedef std::string Key;

	enum SortingMode {
		TOTAL_ELAPSED,
		AVERAGE_ELAPSED,
		COUNT,
	};

	class Stats {
	public:
		Stats() {}
		Stats(timespec s, int p) : start(s),
					   parent(p),
					   count(0) {}

		struct timespec start, finish;

		double total;
		long count;
		int parent;

		double seconds_elapsed() {
			double elapsed;
			elapsed = (finish.tv_sec - start.tv_sec);
			elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
			return elapsed;
		}
	};

	Profiler(const Key & key) : _key(key) {

		timespec starting_time = Profiler::get_time();

		if (Profiler::hierarchy().size() == 0) {
			Profiler::stats().push_back(Stats(starting_time, -1));
			Profiler::keyToId()["__root__"] = 0;
			Profiler::hierarchy().push(0);
		}

		if (keyExists(key)) {
			int id = keyToId()[_key];
			Profiler::stats()[id].count++;
			Profiler::stats()[id].start = starting_time;
			Profiler::stats()[id].parent = Profiler::hierarchy().top();
			Profiler::hierarchy().push(id);
		}
		else {
			int id = Profiler::stats().size();
			Profiler::keyToId()[key] = id;
			Profiler::stats().push_back(Stats(starting_time, Profiler::hierarchy().top()));
			Profiler::hierarchy().push(id);
		}
	}

	~Profiler() {
		timespec end_time = Profiler::get_time();
		Stats & s = Profiler::stats()[keyToId()[_key]];
		s.finish = end_time;
		s.total += s.seconds_elapsed();
		Profiler::hierarchy().pop();
		Profiler::stats()[0].finish = end_time;
		Profiler::stats()[0].total = Profiler::stats()[0].seconds_elapsed();
	}

	static std::vector<Stats> getStatsSorted(SortingMode mode) {

		std::vector<Stats> sorted = Profiler::stats();

		switch(mode) {
		case TOTAL_ELAPSED: {
		        struct {
				bool operator()(const Stats & a, const Stats & b) {
					return a.total < b.total;
				}
			} comp;
			std::sort(sorted.begin(), sorted.end(), comp);
			break;
		}

		case AVERAGE_ELAPSED: {
			struct {
				bool operator()(const Stats & a, const Stats & b) {
					return a.total/a.count < b.total/b.count;
				}
			} comp;
			std::sort(sorted.begin(), sorted.end(), comp);
			break;
		}
		case COUNT:
			struct {
				bool operator()(const Stats & a, const Stats & b) {
					return a.count < b.count;
				}
			} comp;
			std::sort(sorted.begin(), sorted.end(), comp);
			break;
		};

		return sorted;
	}

	static timespec get_time() {
		struct timespec time;
		clock_gettime(CLOCK_MONOTONIC, &time);
		return time;
	}

	static void clear() {

		stats().clear();
		keyToId().clear();
		hierarchy().pop();
	}

	static std::map<int, Key> getInverseMap() {
		std::map<int, Key>  idToKey;
		for (auto k : keyToId()) {
			idToKey[k.second] = k.first;
		}
		return idToKey;
	}

private:

	inline bool keyExists(const Key & key) {
		return keyToId().count(key);
	}

	Key _key;

	static std::stack<int> & hierarchy() { static std::stack<int> h; return h; }
	static std::vector<Stats> & stats() { static std::vector<Stats> s; return s; }
	static std::map<Key, int> & keyToId() { static std::map<Key, int> k; return k; }

	
	class ConsolePrinter {

	public:
		ConsolePrinter() {
			_colWidth = 20;
		}

		struct Node {
			Node(Profiler::Key n, Profiler::Stats s) : name(n), stats(s) {}
			Profiler::Key name;
			Profiler::Stats stats;
			std::vector<Node*> children;
		};

		std::map<int, Profiler::Key> idToKey = Profiler::getInverseMap();

		void printcol(std::string n) {

			if (n.length() > _colWidth) {
				n[_colWidth-1] = '\0';

			}
			int spaces = _colWidth - n.length();
			std::cout << n;
			for(int i = 0; i < spaces; i++) std::cout << " ";
			std::cout << "|";
		}

		void printTitle(std::string title) {
			int spaces_left = _colWidth - title.size();
			for(int i = 0; i < spaces_left/2; i++) std::cout << " ";
			std::cout << title;
			for(int i = 0; i < spaces_left/2 + spaces_left%2; i++) std::cout << " ";
			std::cout << "|";

		}

		void printTopLine() {
			for (int j = 0; j < 3; j++) {
				for (int i = 0; i < _colWidth; i++)
					std::cout << "=";
				std::cout << "|";
			}			
		}
		void printBottomLine() {
			for (int i = 0; i < _colWidth*3 + 3; i++)
				std::cout << "=";
		}

		void print(Node *node, int level, float total_time) {

			printcol(node->name);

			char col[100];
			sprintf(col, "%f sec.", node->stats.total);
			printcol(std::string(col));
			sprintf(col, "%03d %% of total", int((node->stats.total/total_time) * 100));
			printcol(std::string(col));
			std::cout << std::endl;
			for (auto child : node->children)
				print(child, level+1, total_time);
		}

		void print() {
			std::vector<Node> hierarchy;
			for (int i = 0; i < Profiler::stats().size(); i++) {
				hierarchy.push_back(Node(idToKey[i], Profiler::stats()[i]));
			}

			for (int i = 0; i < hierarchy.size(); i++) {
				Profiler::Stats s = hierarchy[i].stats;
				if (s.parent >= 0) {
					hierarchy[s.parent].children.push_back(&hierarchy[i]);
				}
			}

			printTitle("Key");
			printTitle("Time");
			printTitle("Percentage");
			std::cout << std::endl;
			printTopLine();
			std::cout << std::endl;

			print(&hierarchy[0], 0, Profiler::stats()[0].total);

			printBottomLine();
			std::cout << std::endl;
		}

	private:

		int _colWidth;

	};

public:
	static void print() {
	 	ConsolePrinter printer;
	 	printer.print();
	}
};
