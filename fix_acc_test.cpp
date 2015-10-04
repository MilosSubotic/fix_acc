/**
 *
 * @author Milos Subotic <milos.subotic.sm@gmail.com>
 * @license MIT
 *
 * @brief Test for fix_acc.
 *
 */

///////////////////////////////////////////////////////////////////////////////

#include "fix_acc_test.h"

#include <cassert>
#include <vector>
#include <iomanip>

///////////////////////////////////////////////////////////////////////////////

#define ENABLE_LOGGING 1

#define BIGGEST_EXPONENT 254
#define BIGGEST_MANTISA 0x7fffff
#define DEBUG(x) do{ std::cout << #x << " = " << x << std::endl; }while(0)

inline float fields_to_float(
		uint32_t sign,
		uint32_t exponent,
		uint32_t mantisa) {
	using namespace fix_acc::detail;
	float_union fu;
	fu.fields.sign = sign;
	fu.fields.exponent = exponent;
	fu.fields.mantisa = mantisa;
	return fu.f;
}


float fasp_2_float(fix_acc::fasp fa) {
	return float(fa);
}

float float_2_fasp_and_back(float f){
	fix_acc::fasp fa(f);
	//DEBUG_HEX(fa);
	return float(fa);
}

void check_float_2_fasp_and_back(float f0){
	float f1 = float_2_fasp_and_back(f0);
	assert(f0 == f1);
}

template<typename T>
T ordinary_sum(std::vector<T> to_sum) {
	T acc = 0;
	for(auto iter = to_sum.begin(); iter != to_sum.end(); iter++){
		acc += *iter;
	}
	return acc;
}

// Implementation of Kahan
template<typename T> 
T kahan_sum(std::vector<T> for_sum) {
	T sum = 0.0;
	T c = 0.0;
	for(auto iter = for_sum.begin(); iter != for_sum.end(); iter++) {
		T y = *iter - c;

		T t = sum + y;
		c = (t - sum) - y;
		sum = t;
	}
	return sum;
}
 
float fix_acc_sum(std::vector<float> f) {
	fix_acc::fasp acc = 0;
	for(auto iter = f.begin(); iter != f.end(); iter++) {
		acc += *iter;
	}
	
	//DEBUG_HEX(acc);
	return float(acc);
}


void test_problem() {
	// One big many smalls problem.
	float big = fields_to_float(0, 127, 0);
	float small = fields_to_float(0, 127-24, 0);
	float tiny = fields_to_float(0, 127-48, 0);

    const int num_smalls_in_big = 1 << 24;
    const int num_tinies_in_smalls = 1 << 24;

	std::vector<float> many_smalls((1 << 24) + 1);
	for(auto iter = many_smalls.begin(); iter != many_smalls.end(); iter++){
		*iter = small;
	}

    auto k = small/tiny;
	//DEBUG(k);

	std::vector<float> many_tinies(k);
	for(auto iter = many_tinies.begin(); iter != many_tinies.end(); iter++) {
		*iter = tiny;
	}

	std::vector<float> smalls_and_big(num_smalls_in_big + 1, small);
	smalls_and_big[smalls_and_big.size()-1] = big;

	// Because small is less that big's epsilon adding doesn't work.
	std::vector<float> big_and_smalls(1 + num_smalls_in_big, small);
	big_and_smalls[0] = big;

	// Aditional problem       
	std::vector<float> big_and_smalls_and_tinies(1 + num_smalls_in_big + 2*num_tinies_in_smalls, tiny);
	big_and_smalls_and_tinies[0] = big;
	for(int i = 1; i < num_smalls_in_big + 1; i++) {
		big_and_smalls_and_tinies[i] = small;
	}

#if ENABLE_LOGGING
    DEBUG(ordinary_sum(smalls_and_big));
	DEBUG(ordinary_sum(big_and_smalls));
	DEBUG(ordinary_sum(big_and_smalls_and_tinies));
	
	DEBUG(kahan_sum(smalls_and_big));
	DEBUG(kahan_sum(big_and_smalls));
	DEBUG(kahan_sum(big_and_smalls_and_tinies));
	
	DEBUG(fix_acc_sum(smalls_and_big));
	DEBUG(fix_acc_sum(big_and_smalls));
	DEBUG(fix_acc_sum(big_and_smalls_and_tinies));	
#endif

	assert(ordinary_sum(smalls_and_big) == 2.0);
	assert(ordinary_sum(big_and_smalls) == 1.0);
	assert(ordinary_sum(big_and_smalls_and_tinies) == 1.0);

	assert(kahan_sum(smalls_and_big) == 2.0);
	assert(kahan_sum(big_and_smalls) == 2.0);
	assert(kahan_sum(big_and_smalls_and_tinies) == 2.0);

	assert(fix_acc_sum(smalls_and_big) == 2.0);
	assert(fix_acc_sum(big_and_smalls) == 2.0);
	assert(fix_acc_sum(big_and_smalls_and_tinies) == 2.0 + 2*small);
}

void test_constructors_and_conversion() {

	using namespace std;
	using namespace fix_acc;

 	// Highest bit in a[0].
	float zero = fields_to_float(0, 0, 0);
	check_float_2_fasp_and_back(zero);

	float next_to_zero = fields_to_float(0, 0, 1);
	check_float_2_fasp_and_back(next_to_zero);

	float biggest_denormal = fields_to_float(0, 0, BIGGEST_MANTISA);
	check_float_2_fasp_and_back(biggest_denormal);

	float smallest_normal = fields_to_float(0, 1, 0);
	check_float_2_fasp_and_back(smallest_normal);

	check_float_2_fasp_and_back(fields_to_float(0, 2, 0));
	check_float_2_fasp_and_back(fields_to_float(0, 41, 0));

	// Highest bit in a[1].
	check_float_2_fasp_and_back(fields_to_float(0, 42, 0));


	// Highest bit in a[4].
	float biggest_normal = fields_to_float(
			0,
			BIGGEST_EXPONENT,
			BIGGEST_MANTISA);
	check_float_2_fasp_and_back(biggest_normal);

	float infinity = fields_to_float(
			0,
			BIGGEST_EXPONENT + 1,
			0);
	check_float_2_fasp_and_back(infinity);

	for(uint32_t e = 0; e < BIGGEST_EXPONENT + 1; e++){
		DEBUG(e);
		for(uint32_t m = 0; m < BIGGEST_MANTISA; m++){
			check_float_2_fasp_and_back(fields_to_float(0, e, m));
		}
	}
}

fix_acc::fasp acc;
void add_asgn(float f) {
	acc += f;
}

void test_addition_assignment() {
	using namespace std;
	using namespace fix_acc;

	// De-normal add.
	float next_to_zero = fields_to_float(0, 0, 1);
	acc = 0;
	for(int i = 0; i < 10; i++){
		acc += next_to_zero;
	}
	assert(float(acc) == 10*next_to_zero);

	// De-normal add carry.
	acc = fields_to_float(0, 41, BIGGEST_MANTISA);
	float a = fields_to_float(0, 0, 0x400000);
	for(int i = 0; i < (1 << 18); i++){
		acc += a;
	}
	assert(float(acc) == fields_to_float(0, 42, 0));
}

using namespace fix_acc::detail;

uint128_t shift_and_add(uint128_t acc, uint64_t mantisa, uint8_t shift) {

	using namespace std;
	using namespace fix_acc;
	shift &= 0xf;

	uint128_t mantisa128 = mantisa;
	mantisa128 <<= shift;
	acc += mantisa128;
	return acc;
}

void test1(){
	using namespace std;

	uint128_t u128 = shift_and_add(uint128_t(1), 0xf000000000000000L, 3);

	//cout << showbase << setfill('0') << setw(32) << hex << u128 << endl;

	assert(uint64_t(u128 >> 64) == 0x0000000000000007L);
	assert(uint64_t(u128 & 0xffffffffffffffffL) == 0x8000000000000001L);
}

void add_3_nums(
		uint64_t x0,
		uint64_t x1,
		uint64_t x2,
		uint64_t y0,
		uint64_t y1,
		uint64_t y2,
		uint64_t& z0,
		uint64_t& z1,
		uint64_t& z2) {
	uint64_t r0 = x0 + y0;
	uint64_t r1 = x1 + y1 + (x0 > r0);
	uint64_t r2 = x2 + y2 + (x1 > r1);

	z0 = r0;
	z1 = r1;
	z2 = r2;
}

void test2() {
	uint64_t z0;
	uint64_t z1;
	uint64_t z2;
	add_3_nums(1, 2, 3, 4, 5, 6, z0, z1, z2);
	assert(z0 == 5);
	assert(z1 == 7);
	assert(z2 == 9);
}

void fix_acc_test(){
    std::cout.precision(20);
	test_problem();
	//test_constructors_and_conversion();
	test_addition_assignment();
	test1();
	test2();
}

///////////////////////////////////////////////////////////////////////////////
