
#ifndef TRAINLOAD_RECURSIVE_SEARCH_H_
#define TRAINLOAD_RECURSIVE_SEARCH_H_

#include "trainload.h"

// Recursive search brute-force helper class
// One instance for one car. Works only with one type of cars
class RecursiveSearch : public TrainInfo {
public:	
	// Recursive calls		
	RecursiveSearch(const RecursiveSearch& p) : parent(&p) {
		containers = p.containers + p.containers_count;
	}
	RecursiveSearch& operator=(const RecursiveSearch&) = delete;
	RecursiveSearch(RecursiveSearch&&) = delete;
	RecursiveSearch(const RecursiveSearch*) = delete;
	RecursiveSearch(const TrainInfo& train_info) : TrainInfo{ train_info } {}
	RecursiveSearch() {}
	
	template <class T>
	friend bool IsFit(const T* train_info, T* train_info_new, int removed_units, int removed20, int removed40);
	template <class T, class U>
	friend bool GetBestLoad40(const T* ci, const Container* containers, U var);
	template <class T, class U>
	friend bool GetBestLoad80(const T* ci, const Container* containers, U var);
	friend class RecursiveSearchRoot;

private:
	// Just a template marker
	Container* containers;
	struct PROC_80;
	template <class U = PROC_80>
	bool Remove(const BestContainers* remove);

	const RecursiveSearch *const parent = nullptr;		
	BestContainers removed_last;
	static vector<int> result;
};

// Removes selected containers from buffer and process it further in a next car if needed, depending on a car type
template <class U>
bool RecursiveSearch::Remove(const BestContainers* remove) {
	containers_weight = parent->containers_weight - remove->weight;
	if (containers_weight > cars_weight) return false;
	// Ok we are done
	else if (0 == containers_weight) {
		if constexpr (!(is_same_v<PROC_80, U>)) result.clear();
		constexpr int num = (is_same_v<PROC_80, U>) ? C80_MAX_UNITS : C40_MAX_UNITS;
		for (int i = 0; i < num; ++i) {
			if (i < remove->count) result.emplace_back(remove->containers_arr[i]->id);
			else result.emplace_back(0);
		}
		for (const RecursiveSearch* o = parent; o->removed_last.count > 0; o = o->parent) {
			for (int i = 0; i < num; ++i) {
				if (i < o->removed_last.count) result.emplace_back(o->removed_last.containers_arr[i]->id);
				else result.emplace_back(0);
			}
		}
		return true;
	}
	// Since containers are sorted and always processed from light to heavy, it doesn't need to copy and check all the containers every time
	// Determines begin and end of the range to copy and check, depending on what containers were copied before
	int begin = -1;
	int end;
	for (int i = 0; i < remove->count; ++i) {
		if (removed_last.containers_arr[i] != remove->containers_arr[i]) {
			if (-1 == begin) begin = i;
			end = i;
		}
	}
	if (-1 != begin) {
		const Container* c_old = removed_last.containers_arr[begin];
		const Container* c_old_end = max(remove->containers_arr[end], removed_last.containers_arr[end]);
		for (Container *c_new = containers + (c_old - parent->containers) - begin; c_old <= c_old_end; ++c_old) {
			if (begin < remove->count && c_old == remove->containers_arr[begin]) {
				++begin;
			}
			else {
				*c_new++ = *c_old;
			}
		}
		removed_last.containers_arr = remove->containers_arr;
	}
	removed_last.count = remove->count;
	if constexpr (is_same_v<PROC_80, U>) {
		return GetBestLoad80(this, containers);
	}
	else {
		return GetBestLoad40(this, containers);
	}
}

class RecursiveSearchRoot : public RecursiveSearch {
public:
	// First call. Buffer allocates for all possible recursive queue of classes - one chunk for every car
	RecursiveSearchRoot(const TrainInfo& train_info, const vector<Container>& cs) : RecursiveSearch(train_info), buffer(new Container[train_info.containers_count * train_info.cars_count]) {
		result.clear();
		result.reserve(cars_units);
		containers = buffer.get();
		copy(cs.begin(), cs.end(), containers);		
		removed_last.count = 0;
	}
	// Set up class from an existing class with only one type of cars
	RecursiveSearchRoot(const RecursiveSearch& p, int type) : buffer(new Container[p.containers_count * p.cars_all[type]]) {
		containers_count = 0;
		containers_weight = 0;
		containers_units = 0;
		containers_all.fill(0);
		cars_count = cars_all[type] = p.cars_all[type];
		cars_weight = 0;
		cars_units = 0;
		cars_all.fill(0);
		if (Car::C40 == type) {
			cars_weight += C40_MAX_WEIGHT * cars_count;
			cars_units += C40_MAX_UNITS * cars_count;
		}
		else /*if (Car::C80 == type)*/ {
			cars_weight += C80_MAX_WEIGHT * cars_count;
			cars_units += C80_MAX_UNITS * cars_count;
		}
		containers = buffer.get();
		removed_last.count = 0;
	}

	bool Calc();
	string GetStrResult();

private:
	void CalcLefts(array<RecursiveSearchRoot, Car::MAX_TYPE>& groups);
	bool CalcSubsets(array<RecursiveSearchRoot, Car::MAX_TYPE>& groups, const int offset, const Container *const currentContainer, const Container *const end, const int indexMin);
	unique_ptr<Container[]> buffer;
};

#endif // !TRAINLOAD_RECURSIVE_SEARCH_H_


