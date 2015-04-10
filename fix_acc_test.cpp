/**
 * @file fix_acc_test.cpp
 * @date Feb 16, 2015
 *
 * @author Milos Subotic <milos.subotic.sm@gmail.com>
 * @license MIT
 *
 * @brief Test for fix_acc.
 *
 * @version 2.0
 * Changelog:
 * 1.0 - Initial version.
 * 2.0 - No main() in this file.
 *
 */

///////////////////////////////////////////////////////////////////////////////

#include "fix_acc_test.h"

#include <cassert>

///////////////////////////////////////////////////////////////////////////////


float fields_to_float(uint32_t sign, uint32_t exponent, uint32_t mantisa){
	using namespace fix_acc::detail;
	float_union fu;
	fu.fields.sign = sign;
	fu.fields.exponent = exponent;
	fu.fields.mantisa = mantisa;
	return fu.f;
}


typedef __int128_t int128_t;
typedef __uint128_t uint128_t;

// TODO Add to static_float.
std::ostream& operator<<(std::ostream& os, const int128_t& n) {
	if(os.flags() & std::ios::hex){
		if(os.flags() & std::ios::showbase){
			os.put('0').put('x');
		}
		bool non_zero_before = false;
		for(int i = 31; i >= 0; i--){
			uint8_t byte = (n >> 4*i) & 0xf;
			if(non_zero_before || byte != 0){
				non_zero_before = true;
				if(byte < 10){
					os.put(byte + '0');
				}else{
					os.put(byte + 'a' - 10);
				}
			}else{
				if(i < os.width()){
					os.put(os.fill());
				}
			}
		}
	}else if(os.flags() & std::ios::oct) {
		if(os.flags() & std::ios::showbase){
			os.put('0');
		}

		std::string buf; // Chars in reverse order.
		int128_t t = n;
		do{
			buf += t % 8 + '0';
			t /= 8;
		}while(t != 0);
		if(buf.size() < os.width()){
			for(int i = 0; i < os.width() - buf.size(); i++){
				os.put(os.fill());
			}
		}
		for(int i = buf.size()-1; i >= 0; i--){
			os.put(buf[i]);
		}
	}else{ // std::ios::dec
		std::string buf; // Chars in reverse order.
		int128_t t = n;
		do{
			buf += t % 10 + '0';
			t /= 10;
		}while(t != 0);
		if(buf.size() < os.width()){
			for(int i = 0; i < os.width() - buf.size(); i++){
				os.put(os.fill());
			}
		}
		for(int i = buf.size()-1; i >= 0; i--){
			os.put(buf[i]);
		}
	}

	return os;
}
std::ostream& operator<<(std::ostream& os, const uint128_t& n) {
	return os << int128_t(n);
}

void test0() {

	using namespace std;
	using namespace fix_acc;


#if 0
	{
		int x = 100;
		int128_t y = 100;

		cout << "int:" << endl;
		cout << setfill('0') << setw(5) << hex << x << endl;
		cout << oct << x << noshowbase << endl;
		cout << dec << x << endl << endl;

		cout << "int128_t:" << endl;
		cout << showbase << setfill('0') << setw(5) << hex << y << endl;
		cout << oct << y << noshowbase << setw(0) << endl;
		cout << dec << y << endl << endl;
	}
	{
		float zero = fields_to_float(0, 0, 0);
		float next_to_zero = fields_to_float(0, 0, 1);

		fix_acc_float fa;

		fa += next_to_zero;

		cout << hex << fa << dec << endl;
	}

	fix_acc_float fa;
	cout << float(fa) << endl;

#endif

}

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
	test0();
	test1();
	test2();
}

///////////////////////////////////////////////////////////////////////////////
