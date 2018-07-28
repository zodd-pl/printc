
#include <cstddef>
#include <stdexcept>
#include <array>
#include <type_traits>
#include <tuple>

class str_const 
{ 
private:
	const char* const p_;
	const std::size_t sz_;

public:

	constexpr str_const()
	: p_(nullptr), sz_(0)
	{
	}
	
	constexpr str_const(const char* const p, const std::size_t sz)
	: p_(p), sz_(sz)
	{
	}	
	
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

	constexpr auto get() const
	{
		return p_;
	}

	constexpr auto sub_str(std::size_t pos, std::size_t sub_size ) const
	{
		if ( ! ( pos < size() ) )
		{
			return str_const();
		}
		else
		{
			return str_const(get()+pos,std::min(sub_size,size()-pos));
		}
	}

	constexpr auto sub_str(std::size_t pos ) const
	{
		return sub_str(pos,size()-pos);
	}
	
	constexpr auto equal(const str_const & other) const
	{
		if ( size() == other.size() )
		{
			bool same = true;
			for ( int i=0; i < size() ; i++)
			{
				same = same && (*this)[i] == other[i];
			}
			return same;
		}
		return false;
	}
};


#define CONSTANT(...) \
  union { static constexpr auto value() { return __VA_ARGS__; } }

#define CONSTANT_VALUE(...) \
  [] { using R = CONSTANT(__VA_ARGS__); return R{}; }()

#define CSTR(s) CONSTANT_VALUE(str_const(s))


constexpr int next_arg(str_const str,int pos)
{
    while(pos < str.size() && str[pos]!='%')
    {
        pos++;
    }
    return ++pos;
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
    std::uint8_t args =0;
    
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


struct X
{
};


struct compile_error
{
};

template<typename T>
constexpr bool is_compile_error = std::is_same<T,const compile_error>::value;

constexpr auto make_conversion_table()
{
	constexpr auto table = std::make_tuple(
		std::make_pair(str_const("d"),int{}),
		std::make_pair(str_const("c"),char{})
		);
		
	return table;
}

constexpr auto conversion_table = make_conversion_table();
constexpr auto conversion_table_size = std::tuple_size<decltype(conversion_table)>::value;

static_assert(std::tuple_size<decltype(conversion_table)>::value==2);


template<int INDEX = 0>
constexpr int find_conversion_index(str_const sub_format)
{
	if constexpr ( INDEX < conversion_table_size )
	{
		constexpr auto conversion = std::get<INDEX>(conversion_table).first;
		auto arg = sub_format.sub_str(0,conversion.size());
		if (conversion.equal(arg))
		{
			return INDEX;
		}
		else
		{
			return find_conversion_index<INDEX+1>(sub_format);
		}
	}
	else
	{
		return INDEX;
	}
}

template<int INDEX,int SIZE>
constexpr auto is_in_range()
{
	if constexpr ( ! (INDEX < SIZE) )
	{
		throw std::invalid_argument("conversion not found");		
		return compile_error{}; 
	}
	else
	{
		return true;
	}	
}


template<int POS = 0, typename F, typename T >
constexpr auto scan(F,T)
{
	constexpr auto format = F::value();
	constexpr auto arg_pos = next_arg(format,POS);
	
	if constexpr (arg_pos < format.size() )
	{
		constexpr auto sub_format = format.sub_str(arg_pos);
		constexpr auto index = find_conversion_index(sub_format);
		constexpr auto index_in_range = is_in_range<index,conversion_table_size>();

		if constexpr ( ! is_compile_error<decltype(index_in_range)> )
		{
			constexpr auto conversion = std::get<index>(conversion_table).second;
			constexpr auto next_tuple = std::tuple_cat(T{}, std::make_tuple(conversion));
			return scan<arg_pos+1>(F{},next_tuple);
		}
	}
	else
	{
		return T{};
	}
}

template<std::size_t N0, std::size_t N1>
constexpr auto is_same_size()
{
	if constexpr (N0 != N1 )
	{
		throw std::invalid_argument("arguments size does not match format");		
		return compile_error{}; 
	}
	else
	{
		return true;
	}
}



template<typename... ARG >
void call_printf_impl(const char * format, ARG... arg) // arg by value triggers automatic conversions
{
	printf(format,arg...);
}

template<std::size_t... INDEX, typename FORMAT_TYPE, typename... ARG >
void call_printf(const char * format, std::index_sequence<INDEX...>, FORMAT_TYPE, ARG&&... arg)
{
	call_printf_impl<typename std::tuple_element<INDEX,FORMAT_TYPE>::type...>
		(format,std::forward<ARG>(arg)...);
}



template <typename FORMAT,typename... ARG>
constexpr void printc(FORMAT, ARG&&... arg)
{
	constexpr auto format = scan(FORMAT{},std::tuple<>());
	using format_type = decltype(format);
	
	if constexpr ( !is_compile_error<format_type> ) 
	{
		constexpr auto arg_size_match_format = is_same_size<std::tuple_size<format_type>::value,sizeof...(arg)>();
		if constexpr ( !is_compile_error<decltype(arg_size_match_format)> )
		{
			constexpr auto format_cstr = FORMAT::value().get();
			call_printf(format_cstr,std::index_sequence_for<ARG...>{},format,std::forward<ARG>(arg)...);
			
		}
	}
}



int main()
{
	volatile char c = 'c';
	const volatile int i = 0;
	volatile long long  ll = 111;
//    constexpr auto lam  = [](){ return str_const("%c\n");};

	const int & a1 = ll;

	printc(CSTR("%z\n"),c);
	printc(CSTR("dupa%d\n"),ll);

	constexpr auto ret1 = scan(CSTR("test %d %d %d"),std::tuple<const char *>());
	// ret1.x;
	
	return 0;
}

