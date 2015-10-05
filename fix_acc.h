/**
 *
 * @author Milos Subotic <milos.subotic.sm@gmail.com>
 * @license MIT
 *
 * @brief Fixed-point implementation of accumulator for accurate summation.
 *
 */

#ifndef FIX_ACC_H_
#define FIX_ACC_H_

///////////////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <limits>
#include <cassert>
#include <iostream>
#include <iomanip>

///////////////////////////////////////////////////////////////////////////////

// TODO Debug.
#define DEBUG(x) do{ std::cout << #x << " = " << x << std::endl; }while(0)
#define DEBUG_HEX(x) \
	do{ \
		std::cout << #x << " = 0x" \
			<< std::hex << x << std::dec << std::endl; \
	}while(0)


///////////////////////////////////////////////////////////////////////////////

namespace fix_acc {

	////////////////////////////////////

	namespace detail {

		struct float_fields {
			unsigned mantisa :23;
			unsigned exponent :8;
			unsigned sign :1;
		};

		union float_union {
			float_fields fields;
			float f;
			uint32_t bits;
		};

		struct double_fields {
			unsigned long mantisa :52;
			unsigned exponent :11;
			unsigned sign :1;
		};

		union double_union {
			double_fields fields;
			double d;
			uint64_t bits;
		};


		typedef __int128_t int128_t;
		typedef __uint128_t uint128_t;

		inline std::ostream& operator<<(std::ostream& os, const uint128_t& n) {
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
				uint128_t t = n;
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
				uint128_t t = n;
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

		inline std::ostream& operator<<(std::ostream& os, const int128_t& n) {
			return os << uint128_t(n);
		}

		union uint128_t_union {
			uint128_t i128;
			uint64_t i64[2];
		};


		inline uint64_t highest_bit(uint64_t i) {
			// TODO Platform independed C++ code.
			uint64_t result = 64;
			asm(
					"	bsr   %1, %0     \n"
					: "=r"(result)
					: "r"(i)
			);
			return result;
		}

	} // namespace detail

	////////////////////////////////////

	/**
	 * @class fasp
	 * @brief Accumulator with single-precision float numeric range.
	 */
	class fasp {
	public:
		/*
		 * Non-biased exponents if highest 1 is:
		 * 0th-22nd bit of a[0]: 0, denormal numbers
		 * 23tr bit of a[0]: 1
		 * 0th bit of a[1]: 1*64 - 22 = 42
		 * 0th bit of a[2]: 2*64 - 22 = 106
		 * 0th bit of a[3]: 3*64 - 22 = 170
		 * 0th bit of a[4]: 4*64 - 22 = 234
		 * 21st bit of a[4]: 255
		 * 22nd bit of a[4]: infinity
		 */
		uint64_t a[5];

		////////////////////////////////
	public:
		fasp() {
			a[0] = 0;
			a[1] = 0;
			a[2] = 0;
			a[3] = 0;
			a[4] = 0;
		}

		explicit fasp(
				uint64_t a0,
				uint64_t a1,
				uint64_t a2,
				uint64_t a3,
				uint64_t a4) {
			a[0] = a0;
			a[1] = a1;
			a[2] = a2;
			a[3] = a3;
			a[4] = a4;
		}

		explicit fasp(float f) {
			asgn(f);
		}

		fasp& operator=(float f) {			//!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			asgn(f);
			return *this;
		}

		explicit operator float() const {
//#if __x86_64__
#if 0

#if 0
			uint32_t result = 0;
			if(a[4] < 0){
				// Negative number.
				// TODO Implement.
			}else{
				// Non-negative number.
				// TODO Maybe try some other register instead %rbx for result.
				// FIXME This is wrong thing because I just cut off mantisa,
				// I do not do rounding. Could for example do double shift
				// to 32-bit reg with highest bit on 32th bit and then convert
				// it with CVTSI2SS to float and correct its exponent.
				asm(
						"	bsr   %5, %%rcx          \n" // %5 is a[4]
						"	jz    a3_bsr             \n"
						"	cmp   $22, %%cl          \n"
						"	jae   inf                \n"
						"	add   $23,  %%cl         \n"// Correct for shift.
						"	mov   %5, %%rbx          \n"
						"	shld  %%cl, %4, %%rbx    \n"
						"	and   $0x7fffff, %%ebx   \n"
						"	add   $(4*64-2*23), %%cl \n"// Exponent correction.
						"	shl   $23, %%ecx         \n"// Shift exponent
						"	or    %%ecx, %%ebx       \n"// Merge exponent.
						"	jmp   end                \n"
						"a3_bsr:                     \n"
						"	bsr   %4, %%rcx          \n"// %4 is a[3]
						"	jz    a2_bsr             \n"
						"	add   $23,  %%cl         \n"// Correct for shift.
						"	mov   %4, %%rbx          \n"
						"	shld  %%cl, %3, %%rbx    \n"
						"	and   $0x7fffff, %%ebx   \n"
						"	add   $(3*64-2*23), %%cl \n"// Exponent correction.
						"	shl   $23, %%ecx         \n"// Shift exponent
						"	or    %%ecx, %%ebx       \n"// Merge exponent.
						"	jmp   end                \n"
						"a2_bsr:                     \n"
						"	bsr   %3, %%rcx          \n"// %3 is a[2]
						"	jz    a1_bsr             \n"
						"	add   $23,  %%cl         \n"// Correct for shift.
						"	mov   %3, %%rbx          \n"
						"	shld  %%cl, %2, %%rbx    \n"
						"	and   $0x7fffff, %%ebx   \n"
						"	add   $(2*64-2*23), %%cl \n"// Exponent correction.
						"	shl   $23, %%ecx         \n"// Shift exponent
						"	or    %%ecx, %%ebx       \n"// Merge exponent.
						"	jmp   end                \n"
						"a1_bsr:                     \n"
						"	bsr   %2, %%rcx          \n"// %2 is a[1]
						"	jz    a0_bsr             \n"
						"	add   $23,  %%cl         \n"// Correct for shift.
						"	mov   %2, %%rbx          \n"
						"	shld  %%cl, %1, %%rbx    \n"
						"	and   $0x7fffff, %%ebx   \n"
						"	add   $(1*64-2*23), %%cl \n"// Exponent correction.
						"	shl   $23, %%ecx         \n"// Shift exponent
						"	or    %%ecx, %%ebx       \n"// Merge exponent.
						"	jmp   end                \n"
						"a0_bsr:                     \n"
						"	bsr   %1, %%rcx          \n"// %1 is a[0]
						"	jz    zero               \n"
						"	mov   %1, %%rbx          \n"
						"	cmp   $23, %%cl          \n"
						"	jb    end                \n"// Denormal number.
						"	sub   $23,  %%cl         \n"// Correct for shift.
						"	shr   %%cl, %%rbx        \n"
						"	and   $0x7fffff, %%ebx   \n"
						"	add   $1, %%cl           \n"// Exponent correction.
						"	shl   $23, %%ecx         \n"// Shift exponent
						"	or    %%ecx, %%ebx       \n"// Merge exponent.
						"	jmp   end                \n"
						"inf:                        \n"// Handle overflow.
						"	mov   $0x7f800000, %0    \n"// Positive infinity.
						"	jmp   end                \n"
						"zero:                       \n"// Handle zero.
						"	xor   %0, %0             \n"
						"end:                        \n"
						: "=b"(result)
						: "r"(a[0]), "r"(a[1]), "r"(a[2]), "r"(a[3]), "r"(a[4])
						: "%rcx"
				);
			}

			using namespace detail;
			float_union fu;
			fu.bits = result;
			return fu.f;
#else
			if(a[4] < 0){
				// Negative number.
				// TODO Implement.
				return 0;
			}else{
				// Non-negative number.

				// -23-127-31 = -181
				uint64_t u;
				int8_t exp_corr;
				asm(
						"	mov   $63, %%cl          \n"// 63 to cl for later.
						"	bsr   %6, %%rbx          \n"// %6 is a[4]
						"	jz    a3_bsr             \n"
						"	cmp   $22, %%bl          \n"
						"	jae   inf                \n"
						"	sub   %%bl, %%cl         \n"// shift = 63 - pos.
						"	mov   %6, %0             \n"// Doing all in u var.
						"	shld  %%cl, %5, %0       \n"
						"	add   $(4*64-181), %%bl  \n"// Exponent correction.
						"	jmp   end                \n"
						"a3_bsr:                     \n"
						// TODO Implement.
						"	jmp   end                \n"
						"inf:                        \n"// Handle overflow.
						"	mov   $0x7f800000, %0    \n"// Positive infinity.
						"	jmp   end                \n"
						"zero:                       \n"// Handle zero.
						//"	xor   %0, %0             \n"
						"end:                        \n"
						: "=r"(u), "=b"(exp_corr)
						: "r"(a[0]), "r"(a[1]), "r"(a[2]), "r"(a[3]), "r"(a[4])
						: "0", "%rbx", "%rcx", "cc"
				);

				using namespace detail;
				float_union fu;
				DEBUG_HEX(u);
				fu.f = u >> 32;
				DEBUG_HEX(fu.fields.mantisa);
				DEBUG(fu.fields.exponent);
				fu.fields.exponent += exp_corr;
				DEBUG(uint32_t(exp_corr));
				DEBUG(fu.fields.exponent);
				return fu.f;
			}
#endif
#else // __x86_64__
			using namespace detail;
			float_union fu;
			fu.fields.sign = 0;
			// TODO Negative numbers.

			if(a[4] != 0){
				uint8_t hb = highest_bit(a[4]);
				uint8_t shift = hb + 41;
				uint128_t_union iu;
				iu.i64[0] = a[3];
				iu.i64[1] = a[4];
				iu.i128 >>= shift;
				fu.fields.mantisa = iu.i64[0];
				fu.fields.exponent = hb + 42 + 64*3;
			}else if(a[3] != 0){
				uint8_t hb = highest_bit(a[3]);
				uint8_t shift = hb + 41;
				uint128_t_union iu;
				iu.i64[0] = a[2];
				iu.i64[1] = a[3];
				iu.i128 >>= shift;
				fu.fields.mantisa = iu.i64[0];
				fu.fields.exponent = hb + 42 + 64*2;
			}else if(a[2] != 0){
				uint8_t hb = highest_bit(a[2]);
				uint8_t shift = hb + 41;
				uint128_t_union iu;
				iu.i64[0] = a[1];
				iu.i64[1] = a[2];
				iu.i128 >>= shift;
				fu.fields.mantisa = iu.i64[0];
				fu.fields.exponent = hb + 42 + 64*1;
			}else if(a[1] != 0){
				uint8_t hb = highest_bit(a[1]);
				uint8_t shift = hb + 41;
				uint128_t_union iu;
				iu.i64[0] = a[0];
				iu.i64[1] = a[1];
				iu.i128 >>= shift;
				fu.fields.mantisa = iu.i64[0];
				fu.fields.exponent = hb + 42;
			}else if(a[0] != 0){
				uint8_t hb = highest_bit(a[0]);
				if(hb < 23){
					// De-normals.
					fu.fields.mantisa = a[0];
					fu.fields.exponent = 0;
				}else{
					uint8_t shift = hb - 23;
					fu.fields.mantisa = a[0] >> shift;
					fu.fields.exponent = hb - 22;
				}
			}

			return fu.f;
#endif // __x86_64__
		}

		inline fasp& operator+=(float f) {
			if(f < 0){
				sub_asgn(-f);
			}else{
				add_asgn(f);
			}
			return *this;
		}

		inline fasp& operator-=(float f) {
			if(f < 0){
				add_asgn(-f);
			}else{
				sub_asgn(f);
			}
			return *this;
		}


		////////////////////////////////

	private:

		inline void asgn(float f) {
			using namespace detail;
			float_union fu;
			fu.f = f;

			// TODO Negative numbers.

			if(fu.fields.exponent == 0){
				// De-normals.
				a[0] = fu.fields.mantisa;
				a[1] = 0;
				a[2] = 0;
				a[3] = 0;
				a[4] = 0;
			}else{
				fu.fields.exponent -= 1;
				uint8_t a_index = fu.fields.exponent >> 6;				//index are first two bits
				uint8_t shift = fu.fields.exponent & 0x3f;				//exponent: take the first 6 bits
				uint128_t_union iu;
				iu.i64[0] = uint32_t(fu.fields.mantisa) | 0x800000;		//adding hidden one
				iu.i64[1] = 0;
				iu.i128 <<= shift;
				switch(a_index){
					case 0:
						a[0] = iu.i64[0];
						a[1] = iu.i64[1];
						a[2] = 0;
						a[3] = 0;
						a[4] = 0;
						break;
					case 1:
						a[0] = 0;
						a[1] = iu.i64[0];
						a[2] = iu.i64[1];
						a[3] = 0;
						a[4] = 0;
						break;
					case 2:
						a[0] = 0;
						a[1] = 0;
						a[2] = iu.i64[0];
						a[3] = iu.i64[1];
						a[4] = 0;
						break;
					case 3:
						a[0] = 0;
						a[1] = 0;
						a[2] = 0;
						a[3] = iu.i64[0];
						a[4] = iu.i64[1];
						break;
				}
			}
		}

		/**
		 * *this += fabs(f).
		 * @param f positive float.
		 */
		inline void add_asgn(float f) {
			using namespace detail;
			float_union fu;
			fu.f = f;
			assert(fu.fields.sign == 0);

//#if __x86_64__
#if 0

			// TODO
			// Have switch on exponent and then jump to code
			// with inline assembly what you need.
			// Use %0, %1 to optimize out variables to registers if possible.
			// Use shld to shift over two regs.
#else
			uint64_t old;
			bool carry;
			if(fu.fields.exponent == 0){
				// De-normals.
				old = a[0];
				a[0] += fu.fields.mantisa;
				carry = a[0] < old;

				old = a[1];
				a[1] += carry;
				carry = a[1] < old;

				old = a[2];
				a[2] += carry;
				carry = a[2] < old;

				old = a[3];
				a[3] += carry;
				carry = a[3] < old;

				old = a[4];
				a[4] += carry;
			}else{
				fu.fields.exponent -= 1;
				uint8_t a_index = fu.fields.exponent >> 6;
				uint8_t shift = fu.fields.exponent & 0x3f;

#if 1

				uint128_t_union iu;
				iu.i64[0] = uint32_t(fu.fields.mantisa) | 0x800000;
				iu.i64[1] = 0;
				iu.i128 <<= shift;

				switch(a_index){
					case 0:
						old = a[0];
						a[0] += iu.i64[0];
						carry = a[0] < old;

						old = a[1];
						a[1] += iu.i64[1] + carry;
						carry = a[1] < old;

						old = a[2];
						a[2] += carry;
						carry = a[2] < old;

						old = a[3];
						a[3] += carry;
						carry = a[3] < old;

						old = a[4];
						a[4] += carry;

						break;
					case 1:
						a[0] = 0;

						old = a[1];
						a[1] += iu.i64[0];
						carry = a[1] < old;

						old = a[2];
						a[2] += iu.i64[1] + carry;
						carry = a[2] < old;

						old = a[3];
						a[3] += carry;
						carry = a[3] < old;

						old = a[4];
						a[4] += carry;

						break;
					case 2:
						a[0] = 0;

						a[1] = 0;

						old = a[2];
						a[2] += iu.i64[0];
						carry = a[2] < old;

						old = a[3];
						a[3] += iu.i64[1] + carry;
						carry = a[3] < old;

						old = a[4];
						a[4] += carry;

						break;
					case 3:
						a[0] = 0;

						a[1] = 0;

						a[2] = 0;

						old = a[3];
						a[3] += iu.i64[0];
						carry = a[3] < old;

						old = a[4];
						a[4] += iu.i64[1] + carry;

						break;
				}

#elif 1
				// Nothing faster than the switch.

				uint64_t m = uint32_t(fu.fields.mantisa) | 0x800000;
				uint64_t j = a_index << 2; // *4

				asm(
						"   lea   1f(%6, %6, 4), %6         \n" // *5.

						"	mov   %5, %%rbx                 \n"
						"	shl   %%cl, %5                  \n"
						"	add   $-64, %%cl                \n"
						"	neg   %%cl                      \n"
						"	shr   %%cl, %%rbx               \n"

						"	xor   %%rcx, %%rcx              \n"
						"   jmpq  *%6                       \n"
						"1:                                 \n"
						"	add   %5, %0                    \n" // case 0
						"	adc   %%rbx, %1                    \n"
						"	adc   %%rcx, %2                 \n"
						"	adc   %%rcx, %3                 \n"
						"	adc   %%rcx, %4                 \n"
						"	jmp   3f                        \n"
						"	nop                             \n"
						"	nop                             \n"
						"	nop                             \n"
						"2:                                 \n"
						"	add   %5, %1                    \n" // case 1
						"	adc   %%rbx, %2                 \n"
						"	adc   %%rcx, %3                 \n"
						"	adc   %%rcx, %4                 \n"
						"	jmp   3f                        \n"
						"   nop                             \n"
						"	nop                             \n"
						"	nop                             \n"
						"	nop                             \n"
						"3:                                 \n"
						// TODO Rest of cases.
						: "+r"(a[0]), "+r"(a[1]), "+r"(a[2]),
						  	  "+r"(a[3]), "+r"(a[4])
						: "r"(m), "r"(j), "c"(shift)
						: "rbx", "rcx", "cc"
				);

#else
				// Nothing faster than the switch.

				uint128_t_union iu;
				iu.i64[0] = uint32_t(fu.fields.mantisa) | 0x800000;
				iu.i64[1] = 0;
				iu.i128 <<= shift;

				static void* table[4] = {
					&&case_0,
					&&case_1,
					&&case_2,
					&&case_3
				};

				goto *table[a_index];

				case_0:
						old = a[0];
						a[0] += iu.i64[0];
						carry = a[0] < old;

						old = a[1];
						a[1] += iu.i64[1] + carry;
						carry = a[1] < old;

						old = a[2];
						a[2] += carry;
						carry = a[2] < old;

						old = a[3];
						a[3] += carry;
						carry = a[3] < old;

						old = a[4];
						a[4] += carry;

						goto end;

					case_1:
						a[0] = 0;

						old = a[1];
						a[1] += iu.i64[0];
						carry = a[1] < old;

						old = a[2];
						a[2] += iu.i64[1] + carry;
						carry = a[2] < old;

						old = a[3];
						a[3] += carry;
						carry = a[3] < old;

						old = a[4];
						a[4] += carry;

						goto end;

					case_2:
						a[0] = 0;

						a[1] = 0;

						old = a[2];
						a[2] += iu.i64[0];
						carry = a[2] < old;

						old = a[3];
						a[3] += iu.i64[1] + carry;
						carry = a[3] < old;

						old = a[4];
						a[4] += carry;

						goto end;

					case_3:
						a[0] = 0;

						a[1] = 0;

						a[2] = 0;

						old = a[3];
						a[3] += iu.i64[0];
						carry = a[3] < old;

						old = a[4];
						a[4] += iu.i64[1] + carry;

				end:
				;

#endif

			}

#endif
		}

		inline void sub_asgn(float f) {

		}

	};

	inline std::ostream& operator<<(std::ostream& os, const fasp& n) {
		if(os.flags() & std::ios::hex){
			if(os.flags() & std::ios::showbase){
				os.put('0').put('x');
			}
			auto flags_backup = os.flags();

			os << std::setfill('0');
			os << std::setw(16) << n.a[4];
			for(int i = 3; i >= 0; i--){
				os << '_';
				os << std::setw(16) << n.a[i];
			}

			os.flags(flags_backup);
		}else if(os.flags() & std::ios::oct){
			// TODO Implement.
		}else{ // std::ios::dec
			  // TODO Implement.
		}

		return os;
	}

} // namespace fix_acc

///////////////////////////////////////////////////////////////////////////////

#endif // FIX_ACC_H_
