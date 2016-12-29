#pragma once

#include <iostream>
#include <ctime>
#include <map>
#include <stack>
#include <algorithm>
#include <mutex>
#include <thread>

#define ROOT_ID 0

class Profiler {

public:
	typedef std::string Key;

	class KeyMap {
	public:

		typedef std::map<Key, std::map< std::size_t, int> >::iterator  iter;
		
		void clear() {
			_map.clear();
		}
		
		bool exists(Key k) {
			if (_map.count(k))
				return true;

			return false;
		}
		
		bool exists(Key k, std::size_t tid) {
			if (_map.count(k))
				if (_map[k].count(tid)) return true;

			return false;
		}

		iter begin() {
			return _map.begin();
		}

		iter end() {
			return _map.end();
		}
		
		int & operator[](Key k) {
			std::hash<std::thread::id> hasher;
			std::size_t tid = hasher(std::this_thread::get_id());
			return _map[k][tid];
		}
		
		int & get(Key k, std::size_t thread_id) {
			return _map[k][thread_id];
		}
		
	private:
		std::map<Key, std::map< std::size_t, int> > _map;
	};


	enum SortingMode {
		TOTAL_ELAPSED,
		AVERAGE_ELAPSED,
		COUNT,
	};

	class Stats {
	public:
		Stats() {}
		Stats(Key k, timespec s, int p, std::size_t tid) : key(k),
								   start(s),
								   parent(p),
								   count(1),
								   paralel(false),
								   thread_id(tid) {}

		struct timespec start, finish;
		Key key;
		double total;
		long count;
	        bool paralel;
		int parent;
		std::size_t thread_id;

		double seconds_elapsed() {
			double elapsed;
			elapsed = (finish.tv_sec - start.tv_sec);
			elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
			return elapsed;
		}
	};

	Profiler(const Key & key) : _key(key) {
		
		std::unique_lock<std::mutex> lock(profile_mutex());

		timespec starting_time = Profiler::get_time();
		std::hash<std::thread::id> hasher;
		std::size_t tid = hasher(std::this_thread::get_id()); 

		if (!Profiler::keymap().exists("__root__")) {
			Profiler::stats().push_back(Stats("__root__", starting_time, -1, tid));
			Profiler::stats()[0].finish = starting_time;
			Profiler::keymap()["__root__"] = 0;
		}

		if (keymap().exists(key, tid)) {
			int id = keymap().get(_key, tid);
			Profiler::stats()[id].count++;
			Profiler::stats()[id].start = starting_time;
			Profiler::hierarchy()[tid].push(id);
			//assert should have the same parent
		}
		else {
			int id = Profiler::stats().size();
			keymap().get(_key,tid) = id;
			int parent = 0;
			if (hierarchy()[tid].size() > 0) parent = Profiler::hierarchy()[tid].top();
			Profiler::stats().push_back(Stats(key,
							  starting_time,
							  parent,
							  tid));
			Profiler::hierarchy()[tid].push(id);
		}
	}
	
	~Profiler() {
		std::unique_lock<std::mutex> lock(profile_mutex());

		//TODO refactor
		std::hash<std::thread::id> hasher;
		std::size_t tid = hasher(std::this_thread::get_id());

		timespec end_time = Profiler::get_time();
		Stats & s = Profiler::stats()[keymap()[_key]];
		s.finish = end_time;
		s.total += s.seconds_elapsed();		
		Profiler::hierarchy()[tid].pop();
		if (end_time.tv_sec > Profiler::stats()[ROOT_ID].finish.tv_sec) {
			Profiler::stats()[ROOT_ID].finish = end_time;
			Profiler::stats()[ROOT_ID].total = Profiler::stats()[ROOT_ID].seconds_elapsed();
		}
	}

	static std::vector<Stats> sortStats(const SortingMode mode, const std::vector<Stats> stats) {	

		std::vector<Stats> sorted = stats;
		
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

	static std::vector<Stats> getFusedStats() {

		std::map<Key, Stats> key_stats;	
		std::map<int,int> oldnewmapping;
		std::map<Key, int> old_idx;

		//accumulate times per key
		int idx = 0;
		for (auto s : stats()) {
			Key key = s.key;
			if (!key_stats.count(key)) {
				key_stats[key] = s;
				old_idx[key] = idx;		       
			}
			else {
				key_stats[key].count += s.count;
				key_stats[key].paralel = true;
				key_stats[key].total += s.seconds_elapsed();
			}
			idx++;
		}

		int count = 0;
		for (auto idx : old_idx) {
			oldnewmapping[idx.second] = count;
			count++;
		}

		std::vector<Stats> fstats;
		for (auto s : key_stats) {
			Stats fs = s.second;
			if (fs.parent >= 0)
				fs.parent = oldnewmapping[fs.parent];
			
			fstats.push_back(fs);
		}
		return fstats;
	}
	
	static void clear() {

		//TODO refactor
		std::hash<std::thread::id> hasher;
		std::size_t tid = hasher(std::this_thread::get_id());
		
		std::unique_lock<std::mutex> lock(profile_mutex());
		stats().clear();
		keymap().clear();
	}

	static std::map<int, Key> getInverseMap() {
		std::map<int, Key>  idToKey;
		for (auto k : keymap()) {
			for (auto t : k.second)
				idToKey[t.second] = k.first;
		}
		return idToKey;
	}

private:

	Key _key;
	
	static std::mutex & profile_mutex() { static std::mutex m; return m; }
	static std::map<std::size_t, std::stack<int> > & hierarchy() {
		static std::map<std::size_t, std::stack<int> > h; return h; }
	static std::vector<Stats> & stats() { static std::vector<Stats> s; return s; }
	static KeyMap & keymap() { static KeyMap k; return k; }       
	
	class ConsolePrinter {

	public:
		ConsolePrinter() {
			_colWidth = 20;
		}

		struct Node {
			Node(Profiler::Stats s) : name(s.key), stats(s) {}
			Profiler::Key name;
			Profiler::Stats stats;
			std::vector<Node*> children;
		};

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
			for (int j = 0; j < 4; j++) {
				for (int i = 0; i < _colWidth; i++)
					std::cout << "=";
				std::cout << "|";
			}			
		}
		void printBottomLine() {
			for (int i = 0; i < _colWidth*4 + 4; i++)
				std::cout << "=";
		}

		void print(Node *node, int level, float total_time) {

			std::string name = node->name;
			if (name.size() > _colWidth) 
				name.resize(_colWidth);
			
			printcol(name);

			char col[100];
			sprintf(col, "%d/%c (%03.3f sec.)", node->stats.count,
				node->stats.paralel?'P':'S',
				node->stats.total/node->stats.count);
			printcol(std::string(col));
			sprintf(col, "%03.3f sec.", node->stats.total);
			printcol(std::string(col));

			float perc;
			if (node->stats.paralel) {
				perc = ((node->stats.total/node->stats.count)/total_time) * 100;
			}
			else {
				perc = ((node->stats.total)/total_time) * 100;
			}
			
			sprintf(col, "%03d%% (%03d%% of total)",
				int((node->stats.total/total_time) * 100),
				int(perc));

			printcol(std::string(col));
			std::cout << std::endl;
			for (auto child : node->children)
				print(child, level+1, total_time);
		}

		void print() {
			std::vector<Profiler::Stats> fstats = Profiler::getFusedStats();
			
			std::vector<Node> hierarchy;
			for (int i = 0; i < fstats.size(); i++) {
				hierarchy.push_back(Node(fstats[i]));
			}

			for (int i = 0; i < hierarchy.size(); i++) {
				Profiler::Stats s = hierarchy[i].stats;
				if (s.parent >= 0) {
					hierarchy[s.parent].children.push_back(&hierarchy[i]);
				}
			}

			printTitle("Key");
			printTitle("Num (Time)");
			printTitle("Total Time");
			printTitle("Total %");
			std::cout << std::endl;
			printTopLine();
			std::cout << std::endl;

			print(&hierarchy[0], 0, fstats[0].total);

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
