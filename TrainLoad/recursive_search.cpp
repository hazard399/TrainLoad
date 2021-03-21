
#include "recursive_search.h"

#include <vector>
#include <string>

vector<int> RecursiveSearch::result;

string RecursiveSearchRoot::GetStrResult() {
	string res;
	auto it = result.begin();
	for (int i = 0; i < Car::MAX_TYPE; ++i) {
		const int num = (Car::C40 == i) ? C40_MAX_UNITS : C80_MAX_UNITS;
		for (int j = 0; j < cars_all[i]; ++j) {
			res += to_string(i) + ":";
			for (int k = 0; k < num; ++k, ++it) {
				if (0 == *it) continue;
				res += " " + to_string(*it);
			}
			res += "\n";
		}
	}
	return res;
}

void RecursiveSearchRoot::CalcLefts(array<RecursiveSearchRoot, Car::MAX_TYPE>& groups) {
	RecursiveSearchRoot& bf40 = groups[Car::C40];
	RecursiveSearchRoot& bf80 = groups[Car::C80];
	bf80.containers_count = containers_count - bf40.containers_count;
	bf80.containers_units = containers_units - bf40.containers_units;
	transform(containers_all.cbegin(), containers_all.cend(), bf40.containers_all.cbegin(), bf80.containers_all.begin(), minus<decltype(containers_all)::value_type>());
}

bool RecursiveSearchRoot::CalcSubsets(array<RecursiveSearchRoot, Car::MAX_TYPE>& groups, const int offset, const Container *const currentContainer, const Container *const end, const int indexMin) {
	RecursiveSearchRoot& bf40 = groups[Car::C40];
	RecursiveSearchRoot& bf80 = groups[Car::C80];
	++bf40.containers_count;
	bool goFurther = containers_count - bf40.containers_count > bf80.cars_count;
	bool goCheck = 0 == indexMin;
	if (goCheck || goFurther) {
		Container* pos = bf40.containers + offset;
		int type = Container::MAX_TYPE;
		for (const Container* i = currentContainer; i < end - indexMin; ++i) {
			if (type != i->type) {
				if (Container::MAX_TYPE == type) {
					bf40.containers_units += Container::S20 == i->type ? 1 : 2;
				}
				else {
					--bf40.containers_all[type];
					bf40.containers_units += 1;
				}
				type = i->type;
				++bf40.containers_all[type];
				goCheck = goCheck && (containers_units - bf40.containers_units <= cars_units - bf40.cars_units);
				if (goCheck) CalcLefts(groups);
				goFurther = goFurther && (bf40.containers_units < bf40.cars_units);
				if (!goCheck && !goFurther) break;
			}
			*pos = *i;
			bf40.containers_weight += pos->weight;
			if (goCheck) {
				if (GetBestLoad40((const RecursiveSearch*)&bf40, bf40.containers)) {
					bf80.containers_weight = containers_weight - bf40.containers_weight;
					Container* p80 = bf80.containers;
					const Container *const end40 = bf40.containers + bf40.containers_count;
					for (const Container *p = containers, *p40 = bf40.containers; p < end; ++p) {
						if (p40 < end40 && p40->id == p->id) ++p40;
						else {
							*p80++ = *p;
						}
					}
					if (GetBestLoad80((const RecursiveSearch*)&bf80, bf80.containers)) {
						return true;
					}
				}
			}
			if (goFurther && i < end - 1) {
				if (CalcSubsets(groups, offset + 1, i + 1, end, 0 == indexMin ? 0 : indexMin - 1)) return true;
				if (goCheck) CalcLefts(groups);
			}
			bf40.containers_weight -= pos->weight;
		}
		--bf40.containers_all[type];
		bf40.containers_units -= Container::S20 == type ? 1 : 2;
	}
	--bf40.containers_count;
	return false;
}

bool RecursiveSearchRoot::Calc() {
	if (groups_left) {
		array<RecursiveSearchRoot, Car::MAX_TYPE> groups = { RecursiveSearchRoot{*this,Car::C40}, RecursiveSearchRoot{*this,Car::C80} };
		return CalcSubsets(groups, 0, containers, containers + containers_count, cars_all[Car::C40] - 1);
	}
	else {
		if (cars_all[Car::C40]) return GetBestLoad40((const RecursiveSearch*)this, containers);
		else /*if (cars_all[Car::C80])*/ return GetBestLoad80((const RecursiveSearch*)this, containers);
	}
}