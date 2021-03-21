// TrainLoad.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "trainload.h"

#include <string>
#include <fstream>

#include "recursive_search.h"
#include "tables.h"

bool LoadContainers(set<Car>& cars, vector<Container>& containers) {
	cars.clear();
	containers.clear();
	ifstream fi(INFILE);
	if (!fi) return false;
	int cars_size;
	fi >> cars_size;
	while (cars_size--) {
		Car c = {};
		int type;
		fi >> type >> c.count;
		c.type = (Car::Type)type;
		cars.emplace(c);
	}
	int containersSize;
	fi >> containersSize;
	while (containersSize--) {
		Container c = {};
		int type;
		fi >> c.id >> type >> c.weight;
		c.type = (Container::Type)type;
		containers.emplace_back(c);
	}
	if (cars.size() && containers.size()) return true;
	return false;
}

void PrintResult(const string& s) {
	ofstream fo(OUTFILE);
	fo << s;
}

void GetTrainInfo(const set<Car>& cars_empty, const vector<Container>& containers, TrainInfo* train_info) {	
	train_info->cars_count = 0;
	train_info->cars_units = 0;
	train_info->cars_weight = 0;
	train_info->cars_all.fill(0);
	int last_type = Car::MAX_TYPE;
	train_info->groups_left = -1;
	for (auto it = cars_empty.begin(); it != cars_empty.end(); ++it) {
		if (last_type != it->type) {
			last_type = it->type;
			++train_info->groups_left;
		}
		if (Car::C40 == it->type) {
			train_info->cars_weight += C40_MAX_WEIGHT * it->count;
			train_info->cars_units += C40_MAX_UNITS * it->count;
		}
		else if (Car::C80 == it->type) {
			train_info->cars_weight += C80_MAX_WEIGHT * it->count;
			train_info->cars_units += C80_MAX_UNITS * it->count;
		}
		train_info->cars_all[it->type] = it->count;
		train_info->cars_count += it->count;
	}
	train_info->containers_count = containers.size();
	train_info->containers_units = 0;
	train_info->containers_weight = 0;
	train_info->containers_all.fill(0);
	for (const auto& c : containers) {
		++train_info->containers_all[c.type];
		train_info->containers_weight += c.weight;
		if (Container::S20 == c.type) {
			train_info->containers_units += 1;
		}
		else if (Container::S40 == c.type) {
			train_info->containers_units += 2;
		}
	}
}

template <class T>
void AddBestCar(Car::Type car_type, int count_units, const BestContainers& best_containers, vector<T>* best_cars) {
	if (best_containers.weight) {
		best_cars->emplace_back(T{ car_type, count_units, best_containers.weight });
		for (int i = 0; i < best_containers.count; ++i) best_cars->back().container_ids.emplace_back(best_containers.containers_arr[i]->id);
		if constexpr (is_same_v<BestCarFreq, T>) best_cars->back().freq_min = best_containers.freq_arr;
	}
}
constexpr void AddBestCar(...) {}

constexpr bool IsRs(const RecursiveSearch*) {
	return true;
}
constexpr bool IsRs(...) {
	return false;
}

constexpr bool IsBc(vector<BestCar>*) {
	return true;
}
constexpr bool IsBc(...) {
	return false;
}

template <class T>
bool IsFit(const T* train_info, T* train_info_new, int removed_units, int removed20, int removed40) {
	int removedContainers = removed20 + removed40;
	// Caps checks
	int containersCountNew = train_info->containers_count - removedContainers;
	if (containersCountNew < train_info_new->cars_count) return false;
	int containersUnitsNew = train_info->containers_units - removed_units;
	if (containersUnitsNew > train_info_new->cars_units) return false;
	int new20, new40;
	if (removed20) {
		new20 = train_info->containers_all[Container::S20] - removed20;
		if (new20 < 0) return false;
	}
	if (removed40) {
		new40 = train_info->containers_all[Container::S40] - removed40;
		if (new40 < 0) return false;
	}
	if constexpr (is_same_v<RecursiveSearch, T>) {
		if (0 != containersCountNew) copy(((const RecursiveSearch*)train_info)->containers + removedContainers, ((const RecursiveSearch*)train_info)->containers + train_info->containers_count, ((RecursiveSearch*)train_info_new)->containers);
		for (int i = 0; i < removedContainers; ++i) {
			((RecursiveSearch*)train_info_new)->removed_last.containers_arr[i] = ((const RecursiveSearch*)train_info)->containers + i;
		}
		((RecursiveSearch*)train_info_new)->containers_count = containersCountNew;
		((RecursiveSearch*)train_info_new)->containers_units = containersUnitsNew;
		((RecursiveSearch*)train_info_new)->containers_all = ((const RecursiveSearch*)train_info)->containers_all;
		if (removed20) ((RecursiveSearch*)train_info_new)->containers_all[Container::S20] = new20;
		if (removed40) ((RecursiveSearch*)train_info_new)->containers_all[Container::S40] = new40;
	}
	return true;
}

template <class T, class U>
int CompareFreq(const int p, int maxp, const T& arr1, const U& arr2) {
	if (arr1[p] == arr2[p]) {
		if (--maxp) return CompareFreq(p + 1, maxp, arr1, arr2);
		else return 0;
	}
	else if (arr1[p] > arr2[p]) return -1;
	else return 1;
}

template <class T = void, class ...Args>
void CopyBest(BestContainers& best_containers, int new_weight, Args... args) {
	if constexpr (is_same_v<bool, T>) {
		(++args->freq, ...);
		return;
	}
	else if constexpr (is_same_v<vector<BestCar>*, T>) {
		if (best_containers.weight >= new_weight) return;
	}
	else if constexpr (is_same_v<vector<BestCarFreq>*, T>) {
		array <int, sizeof...(Args)> freq_arr_new = { args->freq ... };
		// Sorting networks depending on the containers count (4 is max count)
#define SWAP(i,j) if (freq_arr_new[j] < freq_arr_new[i]) swap(freq_arr_new[i], freq_arr_new[j]);
		if constexpr (1 == sizeof...(Args)) {
			if (freq_arr_new[0] > best_containers.freq_arr[0]) return;
		}
		else if constexpr (2 == sizeof...(Args)) {
			SWAP(0, 1);
			if (-1 == CompareFreq(0, 2, freq_arr_new, best_containers.freq_arr)) return;
		}
		else if constexpr (3 == sizeof...(Args)) {
			SWAP(0, 1);
			SWAP(0, 2);
			int eq;
			if (-1 == (eq = CompareFreq(0, 1, freq_arr_new, best_containers.freq_arr))) return;
			SWAP(1, 2);
			if (0 == eq) {
				if (-1 == CompareFreq(1, 2, freq_arr_new, best_containers.freq_arr)) return;
			}
		}
		else if constexpr (4 == sizeof...(Args)) {
			SWAP(0, 1);
			SWAP(2, 3);
			SWAP(0, 2);
			int eq;
			if (-1 == (eq = CompareFreq(0, 1, freq_arr_new, best_containers.freq_arr))) return;
			SWAP(1, 3);
			SWAP(1, 2);
			if (0 == eq) {
				if (-1 == CompareFreq(1, 3, freq_arr_new, best_containers.freq_arr)) return;;
			}
		}
		if (best_containers.weight >= new_weight) return;
		copy(freq_arr_new.begin(), freq_arr_new.end(), best_containers.freq_arr.begin());
#undef SWAP
	}
	best_containers.weight = new_weight;
	best_containers.containers_arr = { args ... };
}

// Enumerates all the possible combinations of available containers in 40-foot car
template <class T, class U>
bool GetBestLoad40(const T* train_info, const Container* containers, U var) {
	T train_info_new(*train_info);
	train_info_new.cars_count = train_info->cars_count - 1;
	train_info_new.cars_units = train_info->cars_units - C40_MAX_UNITS;
	train_info_new.cars_weight = train_info->cars_weight - C40_MAX_WEIGHT;
	const Container* containers20 = containers; 
	const Container* containers_end20 = containers20 + train_info->containers_all[Container::S20]; 
	if (train_info->containers_all[Container::S20]) {
		if (IsFit(train_info, &train_info_new, 1, 1, 0)) {
			BestContainers max401(1);
			if (IsBc(var)) {
				CopyBest(max401, (containers_end20 - 1)->weight, containers_end20 - 1);
			}
			else if (IsRs(train_info)) {
				CopyBest(max401, containers20->weight, containers20);
				if (((RecursiveSearch*)&train_info_new)->Remove<struct PROC_40>(&max401)) return true;
			}
			else for (const Container* i = containers20; i < containers_end20; ++i) {
				CopyBest<U>(max401, i->weight, i);
			}
			AddBestCar(Car::C40, 1, max401, var);
		}
		if (IsFit(train_info, &train_info_new, 2, 2, 0)) {
			BestContainers max402(2);
			for (const Container* i = containers20; i < containers_end20 - 1; ++i) {
				for (const Container* j = i + 1; j < containers_end20; ++j) {
					int sidesDiff = j->weight - i->weight;
					int sum2 = i->weight + j->weight;
					// Fast round up weight and check in the table for containers compatibility
					if (S40_20_20[(j->weight + 999) / 1000 * S40_20_20_SIZE + (i->weight + 999) / 1000]) {
						CopyBest<U>(max402, sum2, i, j);
						if (IsRs(train_info) && ((RecursiveSearch*)&train_info_new)->Remove<struct PROC_40>(&max402)) return true;
					}
				}
				if (IsRs(train_info)) break;
			}
			AddBestCar(Car::C40, 2, max402, var);
		}
	}
	if ((!IsRs(train_info) || (IsRs(train_info) && !train_info->containers_all[Container::S20])) &&
		 train_info->containers_all[Container::S40]) {
		if (IsFit(train_info, &train_info_new, 2, 0, 1)) {
			const Container* containers40 = containers_end20;
			const Container* containersEnd40 = containers40 + train_info->containers_all[Container::S40];
			BestContainers max40(1);
			if (IsBc(var)) {
				CopyBest(max40, (containersEnd40 - 1)->weight, containersEnd40 - 1);
			}
			else if (IsRs(train_info)) {
				CopyBest(max40, containers40->weight, containers40);
				if (((RecursiveSearch*)&train_info_new)->Remove<struct PROC_40>(&max40)) return true;
			}
			else for (const Container* i = containers40; i < containersEnd40; ++i) {
				CopyBest<U>(max40, i->weight, i);
			}
			AddBestCar(Car::C40, 2, max40, var);
		}
	}
	return false;
}

// Enumerates all the possible combinations of available containers in 80-foot car
template <class T, class U>
bool GetBestLoad80(const T* train_info, const Container* containers, U var) {
	T train_info_new(*train_info);
	train_info_new.cars_count = train_info->cars_count - 1;
	train_info_new.cars_units = train_info->cars_units - C80_MAX_UNITS;
	train_info_new.cars_weight = train_info->cars_weight - C80_MAX_WEIGHT;
	const Container* containers20 = containers;
	const Container* containers_end20 = containers20 + train_info->containers_all[Container::S20];
	const Container* containers40 = containers_end20;
	const Container* containers_end40 = containers40 + train_info->containers_all[Container::S40];
	if (train_info->containers_all[Container::S20]) {
		if (IsFit(train_info, &train_info_new, 1, 1, 0)) {
			BestContainers max801(1);
			if (IsBc(var)) {
				CopyBest(max801, (containers_end20 - 1)->weight, containers_end20 - 1);
			}
			else if (IsRs(train_info)) {
				CopyBest(max801, containers20->weight, containers20);
				if (((RecursiveSearch*)&train_info_new)->Remove(&max801)) return true;
			}
			else for (const Container* i = containers20; i < containers_end20; ++i) {
				CopyBest<U>(max801, i->weight, i);
			}
			AddBestCar(Car::C80, 1, max801, var);
		}
		if (IsFit(train_info, &train_info_new, 2, 2, 0)) {
			BestContainers max8021(2);
			for (const Container* i = containers20; i < containers_end20 - 1; ++i) {
				for (const Container* j = i + 1; j < containers_end20; ++j) {
					int sides_diff = j->weight - i->weight;
					if (sides_diff > C80_2SIDE_2_1_DIFF) break;
					int sum2 = i->weight + j->weight;
					// Type1 & type2				
					if (sides_diff <= C80_2SIDE_2_2_DIFF || sum2 <= 34000) {
						CopyBest<U>(max8021, sum2, i, j);
						if (IsRs(train_info) && ((RecursiveSearch*)&train_info_new)->Remove(&max8021)) return true;
					}
				}
				if (IsRs(train_info)) break;
			}
			AddBestCar(Car::C80, 2, max8021, var);
		}
		if (IsFit(train_info, &train_info_new, 3, 3, 0)) {
			BestContainers max8031(3);
			for (const Container* i = containers20; i < containers_end20 - 2; ++i) {
				for (const Container* j = i + 1; j < containers_end20 - 1; ++j) {
					int sides_diff1 = j->weight - i->weight;
					int sum2 = i->weight + j->weight;
					for (const Container* k = j + 1; k < containers_end20; ++k) {
						int sum3 = sum2 + k->weight;
						if (sum3 > C80_MAX_WEIGHT) break;
						int sidesDiff2 = k->weight - j->weight;
						if (sides_diff1 < sidesDiff2) {
							sidesDiff2 = sides_diff1;
						}						
						if (((sum3 > 66000 && sidesDiff2 <= C80_2SIDE_3_1_66_DIFF) ||
							 (sum3 > 63000 && sum3 <= 66000 && sidesDiff2 <= C80_2SIDE_3_1_63_DIFF) ||
							 (sum3 > 60000 && sum3 <= 63000 && sidesDiff2 <= C80_2SIDE_3_1_60_DIFF) ||
							 (sum3 > 16000 && sum3 <= 60000 && sidesDiff2 <= C80_2SIDE_3_1_16_DIFF) ||
							 (sum3 <= 16000 && sidesDiff2 <= C80_2SIDE_3_1_00_DIFF)) ||
							 ((sum3 > 66000 && sidesDiff2 <= C80_2SIDE_3_2_66_DIFF) ||
							  (sum3 > 64000 && sum3 <= 66000 && sidesDiff2 <= C80_2SIDE_3_2_64_DIFF) ||
						      (sum3 > 60000 && sum3 <= 64000 && sidesDiff2 <= C80_2SIDE_3_2_60_DIFF) ||
							  (sum3 > 16000 && sum3 <= 60000 && sidesDiff2 <= C80_2SIDE_3_2_16_DIFF) ||
							  (sum3 <= 16000 && sidesDiff2 <= C80_2SIDE_3_2_00_DIFF))) {
							CopyBest<U>(max8031, sum3, i, j, k);
							if (IsRs(train_info) && ((RecursiveSearch*)&train_info_new)->Remove(&max8031)) return true;							
						}
					}

				}
				if (IsRs(train_info)) break;
			}
			AddBestCar(Car::C80, 3, max8031, var);
		}
		if (IsFit(train_info, &train_info_new, 4, 4, 0)) {
			BestContainers max80(4);
			for (const Container* i = containers20; i < containers_end20 - 3; ++i) {
				if (i->weight > C80_S20_MAX_WEIGHT) break;
				for (const Container* j = i + 1; j < containers_end20 - 2; ++j) {
					if (j->weight - i->weight > C80_2MIDDLE_MAX_DIFF) break;
					int sum2 = i->weight + j->weight;
					for (const Container* k = j + 1; k < containers_end20 - 1; ++k) {
						int sum3 = sum2 + k->weight;
						for (const Container* l = k + 1; l < containers_end20; ++l) {
							int sides_diff = l->weight - k->weight;
							if (sides_diff > C80_2SIDE_4_MAX_DIFF) break;
							int sum4 = sum3 + l->weight;
							if (sum4 > C80_MAX_WEIGHT) break;
							if ((sum4 > 66000 && sides_diff > C80_2SIDE_4_66_DIFF) ||
								(sum4 > 63000 && sum4 <= 66000 && sides_diff > C80_2SIDE_4_63_DIFF) ||
								(sum4 > 60000 && sum4 <= 63000 && sides_diff > C80_2SIDE_4_60_DIFF) 
								) continue;
							CopyBest<U>(max80, sum4, i, j, k, l);
							if (IsRs(train_info) && ((RecursiveSearch*)&train_info_new)->Remove(&max80)) return true;							
						}
					}
				}
				if (IsRs(train_info)) break;
			}
			AddBestCar(Car::C80, 4, max80, var);
		}
	}
	if ((!IsRs(train_info) || (IsRs(train_info) && !train_info->containers_all[Container::S20])) &&
		 train_info->containers_all[Container::S40]) {
		if (IsFit(train_info, &train_info_new, 2, 0, 1)) {
			BestContainers max801(1);
			if (IsBc(var)) {
				CopyBest(max801, (containers_end40 - 1)->weight, containers_end40 - 1);
			}
			else if (IsRs(train_info)) {
				CopyBest(max801, containers40->weight, containers40);
				if (((RecursiveSearch*)&train_info_new)->Remove(&max801)) return true;
			}
			else for (const Container* i = containers40; i < containers_end40; ++i) {
				CopyBest<U>(max801, i->weight, i);
			}
			AddBestCar(Car::C80, 2, max801, var);
		}
		if (IsFit(train_info, &train_info_new, 4, 0, 2)) {
			BestContainers max802(2);
			for (const Container* i = containers40; i < containers_end40 - 1; ++i) {
				for (const Container* j = i + 1; j < containers_end40; ++j) {
					int sidesDiff = j->weight - i->weight;
					int sum2 = i->weight + j->weight;
					if ((sum2 > 60000 && sidesDiff <= C80_40_2SIDE_2_60_DIFF) ||
						(sum2 > 16000 && sum2 <= 60000 && sidesDiff <= C80_40_2SIDE_2_16_DIFF) ||
						(sum2 <= 16000 && sidesDiff <= C80_40_2SIDE_2_00_DIFF)) {
						CopyBest<U>(max802, sum2, i, j);
						if (IsRs(train_info) && ((RecursiveSearch*)&train_info_new)->Remove(&max802)) return true;
					}
				}
				if (IsRs(train_info)) break;
			}
			AddBestCar(Car::C80, 4, max802, var);
		}
	}
	if (train_info->containers_all[Container::S20] && train_info->containers_all[Container::S40]) {
		if (IsFit(train_info, &train_info_new, 3, 1, 1)) {
			BestContainers max802(2);
			for (const Container* i = containers20; i < containers_end20; ++i) {
				if (i->weight > C80_20_MAX_LOAD) break;
				for (const Container* j = containers40; j < containers_end40; ++j) {
					if (j->weight > C80_40_MAX_LOAD) break;
					int sum2 = i->weight + j->weight;
					// Fast round up weight and check in the table for containers compatibility
					if (S80_40_20[(j->weight + 999) / 1000 * S80_40_20_SIZE + (i->weight + 999) / 1000]) {
						CopyBest<U>(max802, sum2, i, j);
						if (IsRs(train_info) && ((RecursiveSearch*)&train_info_new)->Remove(&max802)) return true;
					}
				}
				if (IsRs(train_info)) break;
			}
			AddBestCar(Car::C80, 3, max802, var);
		}
		if (IsFit(train_info, &train_info_new, 4, 2, 1)) {
			BestContainers max803(3);
			for (const Container* i = containers20; i < containers_end20 - 1; ++i) {
				for (const Container* j = i + 1; j < containers_end20; ++j) {
					int sidesDiff = j->weight - i->weight;
					if (sidesDiff > C80_2SIDE_3_1_MAX_DIFF) break;
					int sum2 = i->weight + j->weight;
					for (const Container* k = containers40; k < containers_end40; ++k) {
						int sum3 = sum2 + k->weight;
						if (sum3 > C80_MAX_WEIGHT) break;
						if ((sum3 > 66000 && sidesDiff <= C80_2SIDE_3_1_66_DIFF) ||
							(sum3 > 63000 && sum3 <= 66000 && sidesDiff <= C80_2SIDE_3_1_63_DIFF) ||
							(sum3 > 60000 && sum3 <= 63000 && sidesDiff <= C80_2SIDE_3_1_60_DIFF) ||
							(sum3 > 16000 && sum3 <= 60000 && sidesDiff <= C80_2SIDE_3_1_16_DIFF) ||
							(sum3 <= 16000 && sidesDiff <= C80_2SIDE_3_1_00_DIFF)) {
							CopyBest<U>(max803, sum3, i, j, k);
							if (IsRs(train_info) && ((RecursiveSearch*)&train_info_new)->Remove(&max803)) return true;
						}
					}
				}
				if (IsRs(train_info)) break;
			}
			AddBestCar(Car::C80, 4, max803, var);
		}
	}
	return false;
}

void GetLoadRatio(const set<Car>& cars, vector<Container> containers, double& load_weight, double& load_cap) {
	int containers_weight = 0;
	int containers_cap = 0;
	for (const auto& s : containers) {
		containers_weight += s.weight;
		containers_cap += (Container::S40 == s.type) ? 2 : 1;
	}
	int cars_weight = 0;
	int cars_cap = 0;
	for (const auto& c : cars) {
		for (int i = 0; i < c.count; ++i) {
			if (Car::C40 == c.type) {
				cars_weight += C40_MAX_WEIGHT;
				cars_cap += C40_MAX_UNITS;
			}
			else if (Car::C80 == c.type) {
				cars_weight += C80_MAX_WEIGHT;
				cars_cap += C80_MAX_UNITS;
			}
		}
	}
	load_weight = (double)containers_weight / cars_weight;
	load_cap = (double)containers_cap / cars_cap;
}

int main() {
	string result;
	set<Car> cars_empty;
	vector<Container> containers;
	if (!LoadContainers(cars_empty, containers)) {
		PrintResult("Wrong file");
		return 0;
	}
	sort(containers.begin(), containers.end());
	double load_weight, load_cap;
	GetLoadRatio(cars_empty, containers, load_weight, load_cap);
	if (load_weight > 1 || load_cap > 1) {
		PrintResult("Doesn't fit");
		return 0;
	}

	while (containers.size()) {
		TrainInfo train_info;
		GetTrainInfo(cars_empty, containers, &train_info);
		// If there are not much containers left, we can finish the task with brute-force approach 
		if (containers.size() <= 16 || containers.size() - train_info.cars_count < 3) {
			RecursiveSearchRoot recursion(train_info, containers);
			// If solution found
			if (recursion.Calc()) {
				result += recursion.GetStrResult();
				PrintResult(result);
				return 0;
			}
			break;
		}
		// Calculating frequency of occurrence containers in all possible combinations
		for (auto& c : containers) c.freq = 0;
		if (train_info.cars_all[Car::C40]) GetBestLoad40(&train_info, containers.data(), false);
		if (train_info.cars_all[Car::C80]) GetBestLoad80(&train_info, containers.data(), false);
		bool zero_freq = false;
		for (const auto& c : containers) {
			if (0 == c.freq) {
				zero_freq = true;
				break;
			}
		}
		// There is container that cannot fit anywhere, so no solution
		if (zero_freq) break;
		// Choosing the best loaded car, depending on capacity and frequency calculated above
		vector<BestCarFreq> best_cars;
		for (const auto& c : cars_empty) {
			if (Car::C40 == c.type) {
				GetBestLoad40(&train_info, containers.data(), &best_cars);
			}
			else /*if (Car::C80 == c.type)*/ {
				GetBestLoad80(&train_info, containers.data(), &best_cars);
			}
		}
		// There are no container combinations found, so no solution
		if (best_cars.size() == 0) break;
		sort(best_cars.begin(), best_cars.end(), [](const BestCarFreq& mc1, const BestCarFreq& mc2) { 
			return mc1.load_cap > mc2.load_cap ||
				  (mc1.load_cap == mc2.load_cap && mc1.freq_min < mc2.freq_min); 
		});
		// Removing the best car and its containers from the set
		for (auto it = cars_empty.begin(); it != cars_empty.end(); ++it) {
			if (it->type == best_cars.begin()->car_type) {
				result += to_string(best_cars.begin()->car_type) + ":";
				if (--it->count == 0) {
					cars_empty.erase(it);
				}
				break;
			}
		}
		for (int id : best_cars.begin()->container_ids) {
			for (auto it = containers.begin(); it != containers.end(); ++it) {
				if (it->id == id) {
					result += " " + to_string(id);
					containers.erase(it);
					break;
				}
			}
		}
		result += "\n";
	}	
	PrintResult("No answer");
	return 0;
}
