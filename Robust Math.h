#include <compare>
#include <limits>
#include <math>
#include <complex>

//NumericBase is needed for a NumericType concept, defined as any class derived from it
struct NumericBase
{
};

enum NumericErrorCode
{
    NEC_SUCCESS,
    ...
    NEC_UINT_OVERFLOW,
    ...
    NEC_ZERO_DIVIDE,
    ...
};

//this structure is similar to std::contract_violation but will require additional compiler support 
template<NumericType T > struct numeric_exception
{
    //file name of the file where the exception happened (1 level up the stack from Robust Math.h)
    wchar_t file_name[SOME_REASONABLE_LENGTH];
    //line number where the exception happened
    std::uint_least32_t line_number;
    //character offset inside that line where the exception was thrown
    //useful in case of expressions such as sqrt(x*x+y*y+z*z)
    std::uint_least32_t character_offset;
    //the numeric values that caused the exception
    T lhs;
    T rhs;
};


//IntegerType is a concept constraing T to fundumental integer types

template <IntegerType T> class Int : public NumericBase
{
    T value;
public:
    constexpr inline static T min = std::numeric_limits<T>::min();
    constexpr inline static T max = std::numeric_limits<T>::max();
    constexpr inline static bool is_signed = std::numeric_limits<T>::is_signed;
    ...
    //one of the several test functions that validate results of possible arithmetic operations without actually evaluating them
    NumericErrorCode test_addition(const T& rhs);
    
    //this will entail a good deal of additional compiler support. The compiler will have to (optionally) enforce 
    //that operator+= can be called only from functions with the same attribute or by functions that catch numeric_exception<T>
    T& operator+=(const T& rhs) [[throws:numeric_exception<T>]];

};

template <FloatingType T> class Real : public NumericBase
{
    T value;
public:
    constexpr inline static T min = std::numeric_limits<T>::min();
    constexpr inline static T max = std::numeric_limits<T>::max();
    ....
    constexpr inline static T pi = std::pi_v<T>;
    ...
};

template <FloatingType T> class Complex : public NumericBase, public std::complex<T>
{
public:
    ...
};

/*  Now, the robust operator+= for unsigned integers. 
    The robustness is achieved either through contract enforcement or through exceptions.
    The former case will be more typical when calculations are performed in batches that can be safely aborted; error logging will in that case be done in contract violation handlers.
    The mission-critical computations will rely on exceptions.
 */

template<> NumericErrorCode Int<unsigned int>::test_addition(const unsigned int& rhs)
{
    switch ((value >> 1) + (rhs >> 1) <=> (max >> 1))
    {
        case std::strong_ordering::less: [[likely]]
            return NEC_SUCCESS;
        case std::strong_ordering::greater: 
            return NEC_UINT_OVERFLOW;
        default: //i.e. std::strong_ordering::equal
            return ((value & 1) && (rhs & 1)) ? NEC_UINT_OVERFLOW : NEC_SUCCESS;
    }
}

template<> unsigned int& Int<unsigned unt>::operator+=(const unsigned int& rhs) [[throws:numeric_exception<unsigned int>]]
[[expects: Int<unsigned int>::test_addition(rhs) == NEC_SUCCESS]]
{
    if (test_addition(rhs) == NEC_SUCCESS) [[likely]]
    {
        return value += rhs;
    }
    else [[unlikely]]
    {//the first 3 constructor parameters are macros to be provided by the compiler
        throw numeric_exception<unsigned int>{CALLER_FILE_NAME,CALLER_LINE_NUMBER,CALLER_CHARACTER_OFFSET,value,rhs};
    }
}

// and let's not forget about math functions

template<NumericType T> constexpr T sin(T x) noexcept; //one declaration covers all numeric types
template<NumericType T> constexpr T tan(T x) [[throws:numeric_exception<T>]]; //tan() can throw NEC_ZERO_DIVIDE

