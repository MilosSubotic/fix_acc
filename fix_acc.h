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
		std::cout << #x << " = " \
			<< std::showbase << std::hex << x \
			<< std::dec << std::noshowbase << std::endl; \
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
			a[0] = 1;
			a[1] = 2;
			a[2] = 3;
			a[3] = 0; a[4] = 0b1000000000000000000000; // 1.70141e+38
			//a[3] = ~0L; a[4] = 0b1111111111111111111111; // Inf
			//a[3] = 0; a[4] = 1;

			a[3] = 3L << 62;
			a[4] = 0b1111111111111111111111; // __FLT_MAX__
		}

		explicit operator float() const {
#if __x86_64__
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
#else
			// TODO Implement.
#error "Not implemented."
			return 0;
#endif
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
