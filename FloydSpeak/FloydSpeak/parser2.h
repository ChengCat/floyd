//
//  parser2.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 25/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef parser2_h
#define parser2_h


#include "quark.h"

#include <string>
#include <memory>

/*
	C99-language constants.
	http://en.cppreference.com/w/cpp/language/operator_precedence
*/
const std::string k_c99_number_chars = "0123456789.";
const std::string k_c99_whitespace_chars = " \n\t\r";
	const std::string k_identifier_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";

/*
	//	a::b
	k_scope_resolution = 1,

	//	~a
	k_bitwize_not = 3,
*/


/*
	White-space policy:
	All function inputs SUPPORTS whitespace.
	Filter at function input. No need to filter when you return for next function.
	Why: ony one function entry, often many function exists.
*/



///////////////////////////////////			eoperator_precedence


/*
	Operator precedence is the same as C99.
	Lower number gives stronger precedence.
*/
enum class eoperator_precedence {
	k_super_strong = 0,

	//	(xyz)
	k_parentesis = 0,

	//	a()
	k_function_call = 2,

	//	a[], aka subscript
	k_looup = 2,

	//	a.b
	k_member_access = 2,


	k_multiply_divider_remainder = 5,

	k_add_sub = 6,

	//	<   <=	For relational operators < and ≤ respectively
	//	>   >=
	k_larger_smaller = 8,


	k_equal__not_equal = 9,

	k_logical_and = 13,
	k_logical_or = 14,

	k_comparison_operator = 15,

	k_super_weak
};



///////////////////////////////////			eoperator_precedence


/*
	These are the operations generated by parsing the C-style expression.
	The order of constants inside enum not important.

	The number tells how many operands it needs.
*/
enum class eoperation {
	k_0_number_constant = 100,

	//	This is string specifying a local variable, member variable, argument, global etc. Only the first entry in a chain.
	k_0_resolve,

	k_0_string_literal,

	k_x_member_access,

	k_2_looup,

	k_2_add,
	k_2_subtract,
	k_2_multiply,
	k_2_divide,
	k_2_remainder,

	k_2_smaller_or_equal,
	k_2_smaller,

	k_2_larger_or_equal,
	k_2_larger,

	k_2_logical_equal,
	k_2_logical_nonequal,
	k_2_logical_and,
	k_2_logical_or,

	k_3_conditional_operator,

	k_n_call,

	k_1_logical_not,
	k_1_load
};



///////////////////////////////////			constant_value_t


/*
	Used to store a constant, of one of the built-in C-types.
*/
struct constant_value_t {
	explicit constant_value_t(bool value) :
		_type(etype::k_bool),
		_bool(value)
	{
	}

	explicit constant_value_t(int value) :
		_type(etype::k_int),
		_int(value)
	{
	}

	explicit constant_value_t(float value) :
		_type(etype::k_float),
		_float(value)
	{
	}

	explicit constant_value_t(const std::string& value) :
		_type(etype::k_string),
		_string(value)
	{
	}

	enum class etype {
		k_bool,
		k_int,
		k_float,
		k_string
	};


	/////////////////		STATE
	etype _type;

	bool _bool = false;
	int _int = 0;
	float _float = 0.0f;
	std::string _string;
};



//??? IDEA: Remove templetization and make concerete expression_t we return instead.


///////////////////////////////////			eoperator_precedence


/*
	Parser calls this interface to handle expressions it has found.
	Make a new expression using inputs.

	It supports recursion.
	Your implementation of maker-interface should not have side effects. Check output when it's done.
*/
template<typename EXPRESSION> struct maker {
	public: virtual ~maker(){};
	public: virtual const EXPRESSION maker__make_identifier(const std::string& s) const = 0;
	public: virtual const EXPRESSION maker__make1(const eoperation op, const EXPRESSION& expr) const = 0;
	public: virtual const EXPRESSION maker__make2(const eoperation op, const EXPRESSION& lhs, const EXPRESSION& rhs) const = 0;
	public: virtual const EXPRESSION maker__make3(const eoperation op, const EXPRESSION& e1, const EXPRESSION& e2, const EXPRESSION& e3) const = 0;
	public: virtual const EXPRESSION maker__call(const EXPRESSION& f, const std::vector<EXPRESSION>& args) const = 0;
	public: virtual const EXPRESSION maker__member_access(const EXPRESSION& address, const std::string& member_name) const = 0;
	public: virtual const EXPRESSION maker__make_constant(const constant_value_t& value) const = 0;
};



template<typename EXPRESSION>
std::pair<EXPRESSION, seq_t> parse_operation(const maker<EXPRESSION>& helper, const seq_t& p1, const EXPRESSION& lhs, const eoperator_precedence precedence);



seq_t skip_whitespace(const seq_t& p);

std::pair<std::string, seq_t> parse_string_literal(const seq_t& p);

// [0-9] and "."  => numeric constant.
std::pair<constant_value_t, seq_t> parse_numeric_constant(const seq_t& p);




/*
	Constant literal
		"3"
		"3.0"
		"\"three\""
		"true"
		"false"

	Function call
		"f ()"
		"f(g())"
		"f(a + "xyz")"
		"f(a + "xyz", 1000 * 3)"

	Variable read
		"x1"
		"hello2"
		"hello.member"

		x[10 + f()]
*/
template<typename EXPRESSION>
std::pair<EXPRESSION, seq_t> parse_single(const maker<EXPRESSION>& helper, const seq_t& p) {
	QUARK_ASSERT(p.check_invariant());

	// ???
	QUARK_ASSERT(p.first() != " ");

	//	String literal?
	if(p.first() == "\""){
		const auto s = read_while_not(p.rest(), "\"");
		const auto result = helper.maker__make_constant(constant_value_t(s.first));
		return { result, s.second.rest() };
	}

	//	Number constant?
	// [0-9] and "."  => numeric constant.
	else if(k_c99_number_chars.find(p.first()) != std::string::npos){
		const auto value_p = parse_numeric_constant(p);
		const auto result = helper.maker__make_constant(value_p.first);
		return { result, value_p.second };
	}

	else if(peek(p, "true").first){
		const auto result = helper.maker__make_constant(constant_value_t(true));
		return { result, peek(p, "true").second };
	}
	else if(peek(p, "false").first){
		const auto result = helper.maker__make_constant(constant_value_t(false));
		return { result, peek(p, "false").second };
	}

	//	Identifier?
	{
		const auto identifier_s = read_while(p, k_identifier_chars);
		if(!identifier_s.first.empty()){
			const auto resolve = helper.maker__make_identifier(identifier_s.first);
			return { resolve, identifier_s.second };
		}
	}

	throw std::runtime_error("Expected constant or identifier.");
}

/*
	Parse a single constant or an expression in parenthesis
	number
	string literal
	somethinge within ().
	function call
	a ? b : c
	-a


	Examples:
		"123"
		"-123"
		"--+-123"
		"(123 + 123 * x + f(y*3))"

*/
template<typename EXPRESSION>
std::pair<EXPRESSION, seq_t> parse_atom(const maker<EXPRESSION>& helper, const seq_t& p){
	QUARK_ASSERT(p.check_invariant());

    const auto p2 = skip_whitespace(p);
	if(p2.empty()){
		throw std::runtime_error("Unexpected end of string");
	}

	const char ch1 = p2.first_char();

	//	Negate? "-xxx"
	if(ch1 == '-'){
		const auto a = parse_expression(helper, p2.rest(), eoperator_precedence::k_super_strong);
		const auto value2 = helper.maker__make1(eoperation::k_1_logical_not, a.first);
		return { value2, a.second };
	}
	else if(ch1 == '+'){
		const auto a = parse_expression(helper, p2.rest(), eoperator_precedence::k_super_strong);
		return a;
	}
	//	Expression within paranthesis? "(yyy)xxx"
	else if(ch1 == '('){
		const auto a = parse_expression(helper, p2.rest(), eoperator_precedence::k_super_weak);
		const auto p3 = skip_whitespace(a.second);
		if (p3.first() != ")"){
			throw std::runtime_error("Expected ')'");
		}
		return { a.first, p3.rest() };
	}

	//	Single constant number, string literal, function call, variable access, lookup or member access. Can be a chain.
	//	"1234xxx" or "my_function(3)xxx"
	else {
		const auto a = parse_single(helper, p2);
		return a;
	}
}

/*
	"lhs()"
	"lhs(1)"
	"lhs(a + b)"
*/
template<typename EXPRESSION>
std::pair<EXPRESSION, seq_t> parse_function_call(const maker<EXPRESSION>& helper, const seq_t& p1, const EXPRESSION& lhs, const eoperator_precedence prev_precedence){
	QUARK_ASSERT(p1.check_invariant());
	QUARK_ASSERT(p1.first() == "(");
	QUARK_ASSERT(prev_precedence > eoperator_precedence::k_function_call);

	const auto p = p1;
	const auto pos3 = skip_whitespace(p.rest());

	//	No arguments.
	if(pos3.first() == ")"){
		const auto result = helper.maker__call(lhs, {});
		return parse_operation(helper, pos3.rest(), result, prev_precedence);
	}
	//	1-many arguments.
	else{
		auto pos_loop = skip_whitespace(pos3);
		std::vector<EXPRESSION> arg_exprs;
		bool more = true;
		while(more){
			const auto a = parse_expression(helper, pos_loop, eoperator_precedence::k_super_weak);
			arg_exprs.push_back(a.first);

			const auto pos5 = skip_whitespace(a.second);
			const auto ch = pos5.first();
			if(ch == ","){
				more = true;
			}
			else if(ch == ")"){
				more = false;
			}
			else{
				throw std::runtime_error("Unexpected char");
			}
			pos_loop = pos5.rest();
		}

		const auto result = helper.maker__call(lhs, arg_exprs);
		return parse_operation(helper, pos_loop, result, prev_precedence);
	}
}

/*
	hello.func(x)
	lhs = ["@", "hello"]

	return = ["->", [], "kitty"]
*/
template<typename EXPRESSION>
std::pair<EXPRESSION, seq_t> parse_member_access_operator(const maker<EXPRESSION>& helper, const seq_t& p, const EXPRESSION& lhs, const eoperator_precedence prev_precedence){
	QUARK_ASSERT(p.check_invariant());
	QUARK_ASSERT(p.first() == ".");
	QUARK_ASSERT(prev_precedence > eoperator_precedence::k_member_access);

	const auto identifier_s = read_while(skip_whitespace(p.rest()), k_identifier_chars);
	if(identifier_s.first.empty()){
		throw std::runtime_error("Expected ')'");
	}
	const auto value2 = helper.maker__member_access(lhs, identifier_s.first);
	return parse_operation(helper, identifier_s.second, value2, prev_precedence);
}

/*
	lhs[<expression>]...
	lhs[10 + z]
	lhs[f(3)]
	lhs["troll"]

	Chains to right.
*/
template<typename EXPRESSION>
std::pair<EXPRESSION, seq_t> parse_lookup(const maker<EXPRESSION>& helper, const seq_t& p, const EXPRESSION& lhs, const eoperator_precedence prev_precedence){
	QUARK_ASSERT(p.check_invariant());
	QUARK_ASSERT(p.first() == "[");
	QUARK_ASSERT(prev_precedence > eoperator_precedence::k_looup);

	const auto p2 = skip_whitespace(p.rest());
	const auto key = parse_expression(helper, p2, eoperator_precedence::k_super_weak);
	const auto result = helper.maker__make2(eoperation::k_2_looup, lhs, key.first);
	const auto p3 = skip_whitespace(key.second);

	// Closing "]".
	if(p3.first() != "]"){
		throw std::runtime_error("Expected closing \"]\"");
	}
	return parse_operation(helper, p3.rest(), result, prev_precedence);
}

template<typename EXPRESSION>
std::pair<EXPRESSION, seq_t> parse_operation(const maker<EXPRESSION>& helper, const seq_t& p0, const EXPRESSION& lhs, const eoperator_precedence precedence){
	QUARK_ASSERT(p0.check_invariant());

	const auto p = skip_whitespace(p0);
	if(!p.empty()){
		const auto op1 = p.first();
		const auto op2 = p.first(2);

		//	Ending parantesis
		if(op1 == ")" && precedence > eoperator_precedence::k_parentesis){
			return { lhs, p0 };
		}

		//	Function call
		//	EXPRESSION (EXPRESSION +, EXPRESSION)
		else if(op1 == "(" && precedence > eoperator_precedence::k_function_call){
			return parse_function_call(helper, p, lhs, precedence);
		}

		//	Member access
		//	EXPRESSION . EXPRESSION +
		else if(op1 == "."  && precedence > eoperator_precedence::k_member_access){
			return parse_member_access_operator(helper, p, lhs, precedence);
		}

		//	Lookup / subscription
		//	EXPRESSION [ EXPRESSIONS ] +
		else if(op1 == "["  && precedence > eoperator_precedence::k_looup){
			return parse_lookup(helper, p, lhs, precedence);
		}

		//	EXPRESSION + EXPRESSION +
		else if(op1 == "+"  && precedence > eoperator_precedence::k_add_sub){
			const auto rhs = parse_expression(helper, p.rest(), eoperator_precedence::k_add_sub);
			const auto value2 = helper.maker__make2(eoperation::k_2_add, lhs, rhs.first);
			return parse_operation(helper, rhs.second, value2, precedence);
		}

		//	EXPRESSION - EXPRESSION -
		else if(op1 == "-" && precedence > eoperator_precedence::k_add_sub){
			const auto rhs = parse_expression(helper, p.rest(), eoperator_precedence::k_add_sub);
			const auto value2 = helper.maker__make2(eoperation::k_2_subtract, lhs, rhs.first);
			return parse_operation(helper, rhs.second, value2, precedence);
		}

		//	EXPRESSION * EXPRESSION *
		else if(op1 == "*" && precedence > eoperator_precedence::k_multiply_divider_remainder) {
			const auto rhs = parse_expression(helper, p.rest(), eoperator_precedence::k_multiply_divider_remainder);
			const auto value2 = helper.maker__make2(eoperation::k_2_multiply, lhs, rhs.first);
			return parse_operation(helper, rhs.second, value2, precedence);
		}
		//	EXPRESSION / EXPRESSION /
		else if(op1 == "/" && precedence > eoperator_precedence::k_multiply_divider_remainder) {
			const auto rhs = parse_expression(helper, p.rest(), eoperator_precedence::k_multiply_divider_remainder);
			const auto value2 = helper.maker__make2(eoperation::k_2_divide, lhs, rhs.first);
			return parse_operation(helper, rhs.second, value2, precedence);
		}

		//	EXPRESSION % EXPRESSION %
		else if(op1 == "%" && precedence > eoperator_precedence::k_multiply_divider_remainder) {
			const auto rhs = parse_expression(helper, p.rest(), eoperator_precedence::k_multiply_divider_remainder);
			const auto value2 = helper.maker__make2(eoperation::k_2_remainder, lhs, rhs.first);
			return parse_operation(helper, rhs.second, value2, precedence);
		}


		//	EXPRESSION ? EXPRESSION : EXPRESSION
		else if(op1 == "?" && precedence > eoperator_precedence::k_comparison_operator) {
			const auto true_expr_p = parse_expression(helper, p.rest(), eoperator_precedence::k_comparison_operator);

			const auto pos2 = skip_whitespace(true_expr_p.second);
			const auto colon = pos2.first();
			if(colon != ":"){
				throw std::runtime_error("Expected \":\"");
			}

			const auto false_expr_p = parse_expression(helper, pos2.rest(), precedence);
			const auto value2 = helper.maker__make3(eoperation::k_3_conditional_operator, lhs, true_expr_p.first, false_expr_p.first);

			//	End this precedence level.
			return { value2, false_expr_p.second };
		}


		//	EXPRESSION == EXPRESSION
		else if(op2 == "==" && precedence > eoperator_precedence::k_equal__not_equal){
			const auto rhs = parse_expression(helper, p.rest(2), eoperator_precedence::k_equal__not_equal);
			const auto value2 = helper.maker__make2(eoperation::k_2_logical_equal, lhs, rhs.first);

			//	End this precedence level.
			return { value2, rhs.second.rest() };
		}
		//	EXPRESSION != EXPRESSION
		else if(op2 == "!=" && precedence > eoperator_precedence::k_equal__not_equal){
			const auto rhs = parse_expression(helper, p.rest(2), eoperator_precedence::k_equal__not_equal);
			const auto value2 = helper.maker__make2(eoperation::k_2_logical_nonequal, lhs, rhs.first);

			//	End this precedence level.
			return { value2, rhs.second.rest() };
		}

		//	!!! Check for "<=" before we check for "<".
		//	EXPRESSION <= EXPRESSION
		else if(op2 == "<=" && precedence > eoperator_precedence::k_larger_smaller){
			const auto rhs = parse_expression(helper, p.rest(2), eoperator_precedence::k_larger_smaller);
			const auto value2 = helper.maker__make2(eoperation::k_2_smaller_or_equal, lhs, rhs.first);

			//	End this precedence level.
			return { value2, rhs.second.rest() };
		}

		//	EXPRESSION < EXPRESSION
		else if(op1 == "<" && precedence > eoperator_precedence::k_larger_smaller){
			const auto rhs = parse_expression(helper, p.rest(2), eoperator_precedence::k_larger_smaller);
			const auto value2 = helper.maker__make2(eoperation::k_2_smaller, lhs, rhs.first);

			//	End this precedence level.
			return { value2, rhs.second.rest() };
		}


		//	!!! Check for ">=" before we check for ">".
		//	EXPRESSION >= EXPRESSION
		else if(op2 == ">=" && precedence > eoperator_precedence::k_larger_smaller){
			const auto rhs = parse_expression(helper, p.rest(2), eoperator_precedence::k_larger_smaller);
			const auto value2 = helper.maker__make2(eoperation::k_2_larger_or_equal, lhs, rhs.first);

			//	End this precedence level.
			return { value2, rhs.second.rest() };
		}

		//	EXPRESSION > EXPRESSION
		else if(op1 == ">" && precedence > eoperator_precedence::k_larger_smaller){
			const auto rhs = parse_expression(helper, p.rest(2), eoperator_precedence::k_larger_smaller);
			const auto value2 = helper.maker__make2(eoperation::k_2_larger, lhs, rhs.first);

			//	End this precedence level.
			return { value2, rhs.second.rest() };
		}


		//	EXPRESSION && EXPRESSION
		else if(op2 == "&&" && precedence > eoperator_precedence::k_logical_and){
			const auto rhs = parse_expression(helper, p.rest(2), eoperator_precedence::k_logical_and);
			const auto value2 = helper.maker__make2(eoperation::k_2_logical_and, lhs, rhs.first);
			return parse_operation(helper, rhs.second, value2, precedence);
		}

		//	EXPRESSION || EXPRESSION
		else if(op2 == "||" && precedence > eoperator_precedence::k_logical_or){
			const auto rhs = parse_expression(helper, p.rest(2), eoperator_precedence::k_logical_or);
			const auto value2 = helper.maker__make2(eoperation::k_2_logical_or, lhs, rhs.first);
			return parse_operation(helper, rhs.second, value2, precedence);
		}

		else{
			return { lhs, p0 };
		}
	}
	else{
		return { lhs, p0 };
	}
}

template<typename EXPRESSION>
std::pair<EXPRESSION, seq_t> parse_expression(const maker<EXPRESSION>& helper, const seq_t& p, const eoperator_precedence precedence){
	QUARK_ASSERT(p.check_invariant());

	auto lhs = parse_atom(helper, p);
	return parse_operation<EXPRESSION>(helper, lhs.second, lhs.first, precedence);
}


template<typename EXPRESSION>
std::pair<EXPRESSION, seq_t> parse_expression(const maker<EXPRESSION>& helper, const seq_t& p){
	return parse_expression<EXPRESSION>(helper, p, eoperator_precedence::k_super_weak);
}


#endif /* parser2_h */
