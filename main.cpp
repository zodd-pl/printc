
#include <cstddef>
#include <stdexcept>
#include <array>
#include <type_traits>

class str_const 
{ 
public:
	const char* const p_;
	const std::size_t sz_;
public:
	template<std::size_t N>
	constexpr str_const(const char (&a)[N]) : 
      p_(a), sz_(N - 1) 
	{
	}
	constexpr char operator[](std::size_t n) const
	{ 
		return n < sz_ ? p_[n] : throw std::out_of_range("");
	}
	constexpr std::size_t size() const
	{
		return sz_;
	} 
};


#define CONSTANT(...) \
  union { static constexpr auto value() { return __VA_ARGS__; } }

#define CONSTANT_VALUE(...) \
  [] { using R = CONSTANT(__VA_ARGS__); return R{}; }()

#define CSTR(s) CONSTANT_VALUE(str_const(s))


constexpr std::uint8_t next_arg(str_const str,std::uint8_t pos)
{
    while(pos < str.size() && str[pos]!='%')
    {
        pos++;
    }
    return pos;
}

constexpr std::uint8_t last_pos(str_const str )
{
    return str.size()-1;
}

constexpr bool is_last(str_const str,std::uint8_t pos )
{
    return pos + 1 == str.size();
}

constexpr std::uint8_t count_args(str_const str)
{
    std:uint8_t args =0;
    
    for (std::uint8_t pos = 0 ; pos < str.size(); pos++)
    {
         if (str[pos]=='%')
        {
            args++;
        }       
    }

    return args;
}

constexpr bool contains(str_const str, char c)
{
    for (std::uint8_t pos = 0 ; pos < str.size(); pos++)
    {
         if (str[pos]==c)
        {
            return true;
        }       
    }
    return false;
}


template<char C,typename FROM, std::enable_if_t< C =='c' ,int > = 0>
constexpr void check_conversion(FROM && from)
{
    volatile const char & t = from;
}

template<char C,typename FROM, std::enable_if_t< C =='d' ,int > = 0>
constexpr void check_conversion(FROM && from)
{
    volatile const int & t = from;
}


template <typename FORMAT, typename T>
constexpr bool check_args(FORMAT , T && t)
{
    constexpr str_const conversion_specifiers("csdio");
    constexpr auto str = FORMAT::value();

    if ( count_args(str)!=1)
    {
        throw std::out_of_range("wrong number of arguments");  
    }

    if ( str.size() == 0)
    {
        throw std::invalid_argument("format string has size 0");  
    }
    
    constexpr auto pos = next_arg(str,0);
    if ( pos < last_pos(str) )
    {
        constexpr auto spec = pos+1;
        constexpr auto arg = str[spec];
        constexpr auto known_arg = contains(conversion_specifiers,arg);
        if (!known_arg)
        {
  		    throw std::invalid_argument("unknown conversion specifier");               
        }
        check_conversion<arg,T>(std::forward<T>(t));
       
       return true;
    }
    else
    {
 		throw std::invalid_argument("");       
    }

    return false;
}


template <typename FORMAT,typename T>
constexpr std::size_t printc(FORMAT f, T && t)
{
	constexpr bool test = check_args<FORMAT,T>(f,std::forward<T>(t));
	if (test)
	{
		printf(FORMAT::value().p_,t);
	}
	else
	{
		
	}

	return 1;
}


int main()
{
	volatile char c = 'c';
	const volatile int i = 0;
	volatile long long  ll = 0;
//    constexpr auto lam  = [](){ return str_const("%c\n");};

	printc(CSTR("%c\n"),c);
	printc(CSTR("dupa%d\n"),i);

	return 0;
}

