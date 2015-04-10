/**
 * @file fix_acc.h
 * @date Feb 16, 2015
 *
 * @author Milos Subotic <milos.subotic.sm@gmail.com>
 * @license MIT
 *
 * @brief Fixed-point implementation of accumulator for accurate summation.
 *
 * @version 1.0
 * Changelog:
 * 1.0 - Initial version.
 *
 */

#ifndef FIX_ACC_H_
#define FIX_ACC_H_

///////////////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <limits>
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

		inline std::ostream& operator<<(std::ostream& os, const int128_t& n) {
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

		inline std::ostream& operator<<(std::ostream& os, const uint128_t& n) {
			return os << int128_t(n);
		}

		union int128_t_union {
			int128_t i128;
			int64_t i64[2];
		};


		inline int64_t highest_bit(int64_t i) {
			int64_t result = 64;
			asm(
					"	bsr   %1, %0     \n"
					: "=r"(result)
					: "r"(i)
			);
			return result;
		}

	} // namespace detail

	////////////////////////////////////

	/*
	 // Non-optimized for speed, but more flexible with range.
	 template<>
	 class fix_acc {
	 };
	 */

	/**
	 * @class fix_acc_float
	 * @brief Accumulator with single-precision float numeric range.
	 */
	class fix_acc_float {
	public:
		int64_t a[5];
		/*
		 * Non-biased exponents if highest 1 is:
		 * 0th-22nd bit of a[0]: 0, denormal numbers
		 * 23tr bit of a[0]: 1
		 * 0th bit of a[1]: 1*64 - 23 = 41
		 * 0th bit of a[2]: 2*64 - 23 = 105
		 * 0th bit of a[3]: 3*64 - 23 = 169
		 * 0th bit of a[4]: 4*64 - 23 = 233
		 * 21st bit of a[4]: 254
		 * 22nd bit of a[4]: infinity
		 */

		////////////////////////////////
	public:
		fix_acc_float() {
			a[0] = 0;
			a[1] = 0;
			a[2] = 0;
			a[3] = 0;
			a[4] = 0;
		}

		fix_acc_float(
				int64_t a0,
				int64_t a1,
				int64_t a2,
				int64_t a3,
				int64_t a4) {
			a[0] = a0;
			a[1] = a1;
			a[2] = a2;
			a[3] = a3;
			a[4] = a4;
		}

		explicit fix_acc_float(float f) {
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
				uint8_t a_index = fu.fields.exponent >> 6;
				uint8_t shift = fu.fields.exponent & 0x3f;
				int128_t_union iu;
				iu.i64[0] = uint32_t(fu.fields.mantisa) | 0x800000;
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
					case 4:
						a[0] = 0;
						a[1] = 0;
						a[2] = 0;
						a[3] = 0;
						a[4] = iu.i64[0];
						break;
				}
			}
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
				int128_t_union iu;
				iu.i64[0] = a[3];
				iu.i64[1] = a[4];
				iu.i128 >>= shift;
				fu.fields.mantisa = iu.i64[0];
				fu.fields.exponent = hb + 42 + 64*3;
			}else if(a[3] != 0){
				uint8_t hb = highest_bit(a[3]);
				uint8_t shift = hb + 41;
				int128_t_union iu;
				iu.i64[0] = a[2];
				iu.i64[1] = a[3];
				iu.i128 >>= shift;
				fu.fields.mantisa = iu.i64[0];
				fu.fields.exponent = hb + 42 + 64*2;
			}else if(a[2] != 0){
				uint8_t hb = highest_bit(a[2]);
				uint8_t shift = hb + 41;
				int128_t_union iu;
				iu.i64[0] = a[1];
				iu.i64[1] = a[2];
				iu.i128 >>= shift;
				fu.fields.mantisa = iu.i64[0];
				fu.fields.exponent = hb + 42 + 64*1;
			}else if(a[1] != 0){
				uint8_t hb = highest_bit(a[1]);
				uint8_t shift = hb + 41;
				int128_t_union iu;
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

		fix_acc_float& operator+=(float f) {
			// TODO
			// Have switch on exponent and then jump to code
			// with inline assembly what you need.
			// Use %0, %1 to optimize out variables to registers if possible.
			// Use shld to shift over two regs.
#if __x86_64__
			using namespace detail;
			float_union fu;
			fu.f = f;
			// If exponent field is 0 then there is no invisible 1.
#else
			// TODO Implement.
#error "Not implemented."
#endif

			return *this;
		}

		////////////////////////////////
	};

	inline std::ostream& operator<<(std::ostream& os, const fix_acc_float& n) {
		if(os.flags() & std::ios::hex){
			if(os.flags() & std::ios::showbase){
				os.put('0').put('x');
			}
			auto flags_backup = os.flags();

			os << std::setfill('0');
			for(int i = 4; i >= 0; i--){
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
