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

#include "TimeMeasure.h"

#include <cassert>
#include <vector>
#include <iomanip>
#include <cmath>

///////////////////////////////////////////////////////////////////////////////

#define ENABLE_LOGGING 0
#define ENABLE_MESUREMENT 1
#define TIME_MEASUREMENT_ITERS 1

#define LARGEST_EXPONENT 254
#define LARGEST_MANTISA 0x7fffff
//#define DEBUG(x) do{ std::cout << #x << " = " << x << std::endl; }while(0)

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
T ordinary_sum(std::vector<T> fv) {
	T acc = 0;
	for(auto iter = fv.begin(); iter != fv.end(); iter++){
		acc += *iter;
	}
	return acc;
}

// Implementation of Kahan
template<typename T>
T kahan_sum(std::vector<T> fv) {
	T sum = 0.0;
	T c = 0.0;
	for(auto iter = fv.begin(); iter != fv.end(); iter++){
		T y = *iter - c;

		T t = sum + y;
		c = (t - sum) - y;
		sum = t;
	}
	return sum;
}

float cascade_acc_sum(const std::vector<float>& fv) {
	/* IN ORDER TO EXTRACT THE EXPONENT OF THE FLOATING POINT NUMBERS,
	 WE PRETEND THAT THEY ARE INTEGERS                                 */
	union intorfloat {
		int ier;
		float w;
	} ierw;
	float sum_float;
	double r[64], /* DOUBLE PRECISION CASCADING ACCUMULATORS         */
	sum_double, /* THE SUM OF ALL THE ACCUMULATORS                 */
	doublex, /* DOUBLE PRECISION VERSION OF NUMBER BEING ADDED  */
	delta; /* FINAL CORRECTION TERM                           */
	int iexp, /* EXPONENT OF THE FLOATING POINT NUMBER           */
	i; /* INDEX OF LOOPS                                  */
	/* SET THE ACCUMULATORS TO ZERO                                       */
	for(i = 0; i < 64; i++)
		r[i] = 0.0;
	/* LOOP TO PROCESS ALL THE NUMBERS                                    */
	for(auto iter = fv.begin(); iter != fv.end(); iter++){
		/* REMOVE THE SIGN BIT                                                */
		ierw.w = fabs(*iter);
		/* SHIFT THE NUMBER, REGARDED AS AN INTEGER, 23 PLACES RIGHT TO       */
		/*   REMOVE THE MANTISSA AND LEAVE THE EXPONENT AT RIGHT OF 32-BIT    */
		/*   FIELD. THEN DIVIDE BY 4 (SHIFT RIGHT 2 PLACES) AS THERE WILL BE  */
		/*   4 CONSECUTIVE EXPONENTS SHARING 1 ACCUMULATOR                    */
		iexp = ierw.ier >> 25;
		/* ADD THE ORIGINAL NUMBER TO THE APPROPRIATE ACCUMULATOR             */
		doublex = (double) *iter;
		r[iexp] += doublex;
	} /* END LOOP ON NUMBERS                                              */
	/* ADD THE ACCUMULATORS IN DECREASING ORDER                           */
	sum_double = 0.0;
	for(i = 0; i < 64; i++)
		sum_double += r[63 - i];
	sum_float = (float) sum_double;
	/* FIND THE EXPONENT OF THE SUM, READY FOR CORRECTION                 */
	ierw.w = fabs(sum_float);
	iexp = ierw.ier >> 25;
	/* SUBTRACT THE DOUBLE SUM FROM APPROPRIATE ACCUMULATOR               */
	r[iexp] -= sum_double;
	/* ADD ALL THE ACCUMULATORS (INCLUDING THE MODIFIED ONE)              */
	delta = 0.0;
	for(i = 0; i < 64; i++)
		delta += r[63 - i];
	/* ADD THE CORRECTION TO THE SUM                                      */
	sum_double += delta;
	/* EXIT THE SUM FUNCTION, RETURNING THE SUM IN SINGLE PRECISION       */
	return ((float) sum_double);
}

float fix_acc_sum(const std::vector<float>& fv) {
	fix_acc::fasp acc(0.0);
	for(auto iter = fv.begin(); iter != fv.end(); iter++){
		acc += *iter;
	}

	return float(acc);
}


void test_problem() {
	// One large many smalls problem.
	float large = fields_to_float(0, 127, 0);
	float small = fields_to_float(0, 127-24, 0);
	float tiny = fields_to_float(0, 127-48, 0);

    const int num_smalls_in_large = 1 << 24;
    const int num_tinies_in_smalls = 1 << 24;

	std::vector<float> many_smalls((1 << 24) + 1);
	for(auto iter = many_smalls.begin(); iter != many_smalls.end(); iter++){
		*iter = small;
	}

	std::vector<float> smalls_and_large(num_smalls_in_large + 1, small);
	smalls_and_large[smalls_and_large.size()-1] = large;

	// Because small is less that large's epsilon adding doesn't work.
	std::vector<float> large_and_smalls(1 + num_smalls_in_large, small);
	large_and_smalls[0] = large;

	// Aditional problem
	std::vector<float> large_and_smalls_and_tinies(
			1 + num_smalls_in_large + 4*num_tinies_in_smalls, tiny);
	large_and_smalls_and_tinies[0] = large;
	for(int i = 1; i < 1 + num_smalls_in_large; i++) {
		large_and_smalls_and_tinies[i] = small;
	}

#if ENABLE_LOGGING
    DEBUG(ordinary_sum(smalls_and_large));
	DEBUG(ordinary_sum(large_and_smalls));
	DEBUG(ordinary_sum(large_and_smalls_and_tinies));

	DEBUG(kahan_sum(smalls_and_large));
	DEBUG(kahan_sum(large_and_smalls));
	DEBUG(kahan_sum(large_and_smalls_and_tinies));

	DEBUG(cascade_acc_sum(smalls_and_large));
	DEBUG(cascade_acc_sum(large_and_smalls));
	DEBUG(cascade_acc_sum(large_and_smalls_and_tinies));

	DEBUG(fix_acc_sum(smalls_and_large));
	DEBUG(fix_acc_sum(large_and_smalls));
	DEBUG(fix_acc_sum(large_and_smalls_and_tinies));
#endif

#if 1
	assert(ordinary_sum(smalls_and_large) == 2.0);
	assert(ordinary_sum(large_and_smalls) == 1.0);
	assert(ordinary_sum(large_and_smalls_and_tinies) == 1.0);

	assert(kahan_sum(smalls_and_large) == 2.0);
	assert(kahan_sum(large_and_smalls) == 2.0);
	assert(kahan_sum(large_and_smalls_and_tinies) == 2.0);

	assert(cascade_acc_sum(smalls_and_large) == 2.0);
	assert(cascade_acc_sum(large_and_smalls) == 2.0);
	assert(cascade_acc_sum(large_and_smalls_and_tinies) == 2.0 + 4*small);

	assert(fix_acc_sum(smalls_and_large) == 2.0);
	assert(fix_acc_sum(large_and_smalls) == 2.0);
	assert(fix_acc_sum(large_and_smalls_and_tinies) == 2.0 + 4*small);
#endif

#if ENABLE_MESUREMENT

	TimeMeasure ordinary_sum_smalls_and_large_time;
	for(int i = 0; i < TIME_MEASUREMENT_ITERS; i++){
		assert(ordinary_sum(smalls_and_large) == 2.0);
	}
	PRINT_MEASURED_TIME(ordinary_sum_smalls_and_large_time);

	TimeMeasure ordinary_sum_large_and_smalls_time;
	for(int i = 0; i < TIME_MEASUREMENT_ITERS; i++){
		assert(ordinary_sum(large_and_smalls) == 1.0);
	}
	PRINT_MEASURED_TIME(ordinary_sum_large_and_smalls_time);

	TimeMeasure ordinary_sum_large_and_smalls_and_tinies_time;
	for(int i = 0; i < TIME_MEASUREMENT_ITERS; i++){
		assert(ordinary_sum(large_and_smalls_and_tinies) == 1.0);
	}
	PRINT_MEASURED_TIME(ordinary_sum_large_and_smalls_and_tinies_time);


	TimeMeasure kahan_sum_smalls_and_large_time;
	for(int i = 0; i < TIME_MEASUREMENT_ITERS; i++){
		assert(kahan_sum(smalls_and_large) == 2.0);
	}
	PRINT_MEASURED_TIME(kahan_sum_smalls_and_large_time);

	TimeMeasure kahan_sum_large_and_smalls_time;
	for(int i = 0; i < TIME_MEASUREMENT_ITERS; i++){
		assert(kahan_sum(large_and_smalls) == 2.0);
	}
	PRINT_MEASURED_TIME(kahan_sum_large_and_smalls_time);

	TimeMeasure kahan_sum_large_and_smalls_and_tinies_time;
	for(int i = 0; i < TIME_MEASUREMENT_ITERS; i++){
		assert(kahan_sum(large_and_smalls_and_tinies) == 2.0);
	}
	PRINT_MEASURED_TIME(kahan_sum_large_and_smalls_and_tinies_time);


	TimeMeasure cascade_acc_sum_smalls_and_large_time;
	for(int i = 0; i < TIME_MEASUREMENT_ITERS; i++){
		assert(cascade_acc_sum(smalls_and_large) == 2.0);
	}
	PRINT_MEASURED_TIME(cascade_acc_sum_smalls_and_large_time);

	TimeMeasure cascade_acc_sum_large_and_smalls_time;
	for(int i = 0; i < TIME_MEASUREMENT_ITERS; i++){
		assert(cascade_acc_sum(large_and_smalls) == 2.0);
	}
	PRINT_MEASURED_TIME(cascade_acc_sum_large_and_smalls_time);

	TimeMeasure cascade_acc_sum_large_and_smalls_and_tinies_time;
	for(int i = 0; i < TIME_MEASUREMENT_ITERS; i++){
		assert(cascade_acc_sum(large_and_smalls_and_tinies) == 2.0 + 4*small);
	}
	PRINT_MEASURED_TIME(cascade_acc_sum_large_and_smalls_and_tinies_time);


	TimeMeasure fix_acc_sum_smalls_and_large_time;
	for(int i = 0; i < TIME_MEASUREMENT_ITERS; i++){
		assert(fix_acc_sum(smalls_and_large) == 2.0);
	}
	PRINT_MEASURED_TIME(fix_acc_sum_smalls_and_large_time);

	TimeMeasure fix_acc_sum_large_and_smalls_time;
	for(int i = 0; i < TIME_MEASUREMENT_ITERS; i++){
		assert(fix_acc_sum(large_and_smalls) == 2.0);
	}
	PRINT_MEASURED_TIME(fix_acc_sum_large_and_smalls_time);

	TimeMeasure fix_acc_sum_large_and_smalls_and_tinies_time;
	for(int i = 0; i < TIME_MEASUREMENT_ITERS; i++){
		assert(fix_acc_sum(large_and_smalls_and_tinies) == 2.0 + 4*small);
	}
	PRINT_MEASURED_TIME(fix_acc_sum_large_and_smalls_and_tinies_time);

#endif
}

void test_constructors_and_conversion() {

	using namespace std;
	using namespace fix_acc;

 	// Highest bit in a[0].
	float zero = fields_to_float(0, 0, 0);
	check_float_2_fasp_and_back(zero);

	float next_to_zero = fields_to_float(0, 0, 1);
	check_float_2_fasp_and_back(next_to_zero);

	float largest_denormal = fields_to_float(0, 0, LARGEST_MANTISA);
	check_float_2_fasp_and_back(largest_denormal);

	float smallest_normal = fields_to_float(0, 1, 0);
	check_float_2_fasp_and_back(smallest_normal);

	check_float_2_fasp_and_back(fields_to_float(0, 2, 0));
	check_float_2_fasp_and_back(fields_to_float(0, 41, 0));

	// Highest bit in a[1].
	check_float_2_fasp_and_back(fields_to_float(0, 42, 0));


	// Highest bit in a[4].
	float largest_normal = fields_to_float(
			0,
			LARGEST_EXPONENT,
			LARGEST_MANTISA);
	check_float_2_fasp_and_back(largest_normal);

	float infinity = fields_to_float(
			0,
			LARGEST_EXPONENT + 1,
			0);
	check_float_2_fasp_and_back(infinity);

	for(uint32_t e = 0; e < LARGEST_EXPONENT + 1; e++){
		DEBUG(e);
		for(uint32_t m = 0; m < LARGEST_MANTISA; m++){
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
	acc = fields_to_float(0, 41, LARGEST_MANTISA);
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
#if 0
	//test_constructors_and_conversion();
	test_addition_assignment();
	test1();
	test2();
#endif
}

///////////////////////////////////////////////////////////////////////////////
