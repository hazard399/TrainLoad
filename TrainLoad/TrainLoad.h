
#ifndef TRAINLOAD_TRAINLOAD_H_
#define TRAINLOAD_TRAINLOAD_H_

#include <array>
#include <vector>
#include <set>

using namespace std;

#define INFILE R"(containers.txt)"
#define OUTFILE R"(out.txt)"

#define CAR_MAX_CONTAINERS 4

#define CONTAINER_MIN_LOAD 2370
#define CONTAINER_MAX_LOAD 30480

//#define C40_MAX_WEIGHT 72000
#define C40_MAX_WEIGHT CONTAINER_MAX_LOAD * 2
#define C40_MAX_UNITS 2

#define C80_MAX_WEIGHT 68000
#define C80_MAX_UNITS 4
#define C80_S20_MAX_WEIGHT 24000

#define C80_2MIDDLE_MAX_DIFF 3000
#define C80_2SIDE_4_MAX_DIFF 3000
#define C80_2SIDE_4_66_DIFF 500
#define C80_2SIDE_4_63_DIFF 1000
#define C80_2SIDE_4_60_DIFF 2500

#define C80_2SIDE_3_1_MAX_DIFF 4000
#define C80_2SIDE_3_1_66_DIFF 1000
#define C80_2SIDE_3_1_63_DIFF 2000
#define C80_2SIDE_3_1_60_DIFF 3500
#define C80_2SIDE_3_1_16_DIFF 4000
#define C80_2SIDE_3_1_00_DIFF 3500

#define C80_2SIDE_3_2_66_DIFF 3000
#define C80_2SIDE_3_2_64_DIFF 4000
#define C80_2SIDE_3_2_60_DIFF 5500
#define C80_2SIDE_3_2_16_DIFF 6000
#define C80_2SIDE_3_2_00_DIFF 5500

#define C80_2SIDE_2_1_DIFF 11000
#define C80_2SIDE_2_2_DIFF 4000

#define C80_40_2SIDE_2_60_DIFF 5500
#define C80_40_2SIDE_2_16_DIFF 6000
#define C80_40_2SIDE_2_00_DIFF 5500

#define C80_40_MAX_LOAD 30480
#define C80_20_MAX_LOAD 19000

struct Container {
	mutable int freq;
	int weight;
	enum Type { S20, S40, /*S45,*/ MAX_TYPE };
	Type type;
	int id;
	//bool operator < (const Container& c) const {
	//	return weight < c.weight;
	//}
	bool operator < (const Container& c) const {
		return type < c.type ||
			(type == c.type && weight < c.weight);
	}

};

struct Car {
	enum Type { C40, C80, MAX_TYPE };
	Type type;
	bool operator < (const Car& c) const {
		return type < c.type;
	}
	// For change in set 
	mutable int count = 0;
};

typedef array<int, Car::MAX_TYPE> CARS_ALL;
typedef array<int, Container::MAX_TYPE> CONTAINERS_ALL;

struct TrainInfo {
	int cars_count;
	int cars_units;
	int cars_weight;
	int containers_count;
	int containers_units;
	int containers_weight;
	int groups_left;
	CARS_ALL cars_all;
	CONTAINERS_ALL containers_all;
};

struct BestContainers {
	int weight;
	array<const Container*, CAR_MAX_CONTAINERS> containers_arr;
	BestContainers(int c) : count(c), weight(0) {
		freq_arr.fill(numeric_limits<int>::max());
	}
	BestContainers() {}
	int count;
	array<int, CAR_MAX_CONTAINERS> freq_arr;
};

struct BestCar {
	BestCar(Car::Type ct, int cu, int w) :car_type(ct), count_units(cu), weight(w) {
		if (Car::C40 == ct) {
			max_units = C40_MAX_UNITS;
			max_weight = C40_MAX_WEIGHT;
		}
		else if (Car::C80 == ct) {
			max_units = C80_MAX_UNITS;
			max_weight = C80_MAX_WEIGHT;
		}
		load_cap = (double)count_units / max_units;
		load_weight = (double)weight / max_units;
		container_ids.reserve(CAR_MAX_CONTAINERS);
	}
	int weight = 0;
	vector<int> container_ids;
	int max_units;
	int max_weight;
	int count_units;
	double load_cap;
	double load_weight;
	Car::Type car_type;
};

struct BestCarFreq : public BestCar {
	using BestCar::BestCar;
	array<int, CAR_MAX_CONTAINERS> freq_min;
};

// Very busy place here, so we use templates to implement three fully optimized approaches (calculating probability of occurrence container in all possible combinations, 
// brute-force recursive train loading and best car combination look up) and don't repeat code
template <class T, class U = void*> bool GetBestLoad40(const T* ci, const Container* containers, U var = 0);
template <class T, class U = void*> bool GetBestLoad80(const T* ci, const Container* containers, U var = 0);

#endif // !TRAINLOAD_TRAINLOAD_H_
