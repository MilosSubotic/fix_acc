/**
 * @file fix_acc_test.cpp
 * @date Feb 16, 2015
 *
 * @author Milos Subotic <milos.subotic.sm@gmail.com>
 * @license MIT
 *
 * @brief
 *
 * @version 1.0
 * Changelog:
 * 1.0 - Initial version.
 *
 */

///////////////////////////////////////////////////////////////////////////////

#include "fix_acc.h"

///////////////////////////////////////////////////////////////////////////////


float fields_to_float(uint32_t sign, uint32_t exponent, uint32_t mantisa){
	using namespace fix_acc::detail;
	float_union fu;
	fu.fields.sign = sign;
	fu.fields.exponent = exponent;
	fu.fields.mantisa = mantisa;
	return fu.f;
}

// TODO Move to static_float.
typedef __int128_t int128_t;
typedef __uint128_t uint128_t;

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


int main() {

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
#endif

	fix_acc_float fa;
	cout << float(fa) << endl;


	cout << "End!" << endl;

	return 0;


}


///////////////////////////////////////////////////////////////////////////////
