//
//  main.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 27/03/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "floyd_parser.h"

/*
#define QUARK_ASSERT_ON true
#define QUARK_TRACE_ON true
#define QUARK_UNIT_TESTS_ON true
*/

#include "parser_primitives.h"
#include "text_parser.h"
#include "steady_vector.h"
#include "parse_statement.h"
#include "parser_expression.h"
#include "parser_statement.h"
#include "parser_function.h"
#include "parser_struct.h"
#include "parser_ast.h"
#include "ast_utils.h"
#include "json_support.h"

#include <string>
#include <memory>
#include <map>
#include <iostream>
#include <cmath>

namespace floyd_parser {


using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;
using std::unique_ptr;
using std::make_shared;

/*
	AST ABSTRACT SYNTAX TREE

https://en.wikipedia.org/wiki/Abstract_syntax_tree

https://en.wikipedia.org/wiki/Parsing_expression_grammar
https://en.wikipedia.org/wiki/Parsing
*/



struct test_value_class_a {
	int _a = 10;
	int _b = 10;

	bool operator==(const test_value_class_a& other){
		return _a == other._a && _b == other._b;
	}
};

QUARK_UNIT_TESTQ("test_value_class_a", "what is needed for basic operations"){
	test_value_class_a a;
	test_value_class_a b = a;

	QUARK_TEST_VERIFY(b._a == 10);
	QUARK_TEST_VERIFY(a == b);
}






//////////////////////////////////////////////////		read_statement()


	/*
		Read one statement, including any expressions it uses.
		Supports all statments:
			- return statement
			- struct-definition
			- function-definition
			- define constant, with initializating.

		Never simplifes expressions- the parser is non-lossy.

		Returns:
			_statement
				[return_statement]
					[expression]
				...
			_ast
				May have been updated to hold new function-definition or struct-definition.
			_rest
				will never start with whitespace.
				trailing ";" will be consumed.



		Example return statement:
			#1	"return 3;..."
			#2	"return f(3, 4) + 2;..."

		Example function definitions:
			#1	"int test_func1(){ return 100; };..."
			#2	"string test_func2(int a, float b){ return "sdf" };..."

		Example struct definitions:
			"struct my_image { int width; int height; };"
			"struct my_sprite { string name; my_image image; };..."

		Example variable definitions
			"int a = 10;..."
			"int b = f(a);..."


		FUTURE
		- Add local scopes / blocks.
		- Include comments
		- Add mutable variables
	*/

    struct statement_result_t {
        statement_t _statement;
        std::string _rest;
    };

statement_result_t read_statement(scope_ref_t scope_def, const string& pos){
	QUARK_ASSERT(scope_def->check_invariant());

	const auto token_pos = read_until(pos, whitespace_chars);

	//	return statement?
	if(token_pos.first == "return"){
		const auto return_statement_pos = parse_return_statement(pos);
		return { make__return_statement(return_statement_pos.first), skip_whitespace(return_statement_pos.second) };
	}

	//	struct definition?
	else if(token_pos.first == "struct"){
		const auto a = parse_struct_definition(scope_def, pos);
		return { define_struct_statement_t{ a.first }, skip_whitespace(a.second) };
	}

	else {
		const auto type_pos = read_required_type_identifier(pos);
		const auto identifier_pos = read_required_single_symbol(type_pos.second);

		//	Check for "a = 3"-type assignment, with inferred type.

		/*
			Function definition?
			"int xyz(string a, string b){ ... }
		*/
		if(peek_string(skip_whitespace(identifier_pos.second), "(")){
			const auto function = parse_function_definition(scope_def, pos);
			{
				QUARK_SCOPED_TRACE("FUNCTION DEF");
				QUARK_TRACE(json_to_compact_string(scope_def_to_json(*function.first)));
			}
            return { define_function_statement_t{ function.first }, skip_whitespace(function.second) };
		}

		//	Define variable?
		/*
			"int a = 10;"
			"string hello = f(a) + \"_suffix\";";
		*/

		else if(peek_string(skip_whitespace(identifier_pos.second), "=")){
			pair<statement_t, string> assignment_statement = parse_assignment_statement(pos);
			return { assignment_statement.first, skip_whitespace(assignment_statement.second) };
		}

		else{
			throw std::runtime_error("syntax error");
		}
	}
}

QUARK_UNIT_TESTQ("read_statement()", ""){
	try{
		auto global = scope_def_t::make_global_scope();
		const auto result = read_statement(global, "int f()");
		QUARK_TEST_VERIFY(false);
	}
	catch(...){
	}
}

QUARK_UNIT_TESTQ("read_statement()", ""){
	auto global = scope_def_t::make_global_scope();
	const auto result = read_statement(global, "float test = testx(1234);\n\treturn 3;\n");
}

QUARK_UNIT_TESTQ("read_statement()", ""){
	auto global = scope_def_t::make_global_scope();
	const auto result = read_statement(global, test_function1);
	QUARK_TEST_VERIFY(result._statement._define_function);
//	QUARK_TEST_VERIFY(*result._statement._define_function->_function_def == *make_test_function1(global));
	QUARK_TEST_VERIFY(result._statement._define_function->_function_def);
	QUARK_TEST_VERIFY(result._rest == "");
}

QUARK_UNIT_TESTQ("read_statement()", ""){
	auto global = scope_def_t::make_global_scope();
	const auto result = read_statement(global, "struct test_struct0 " + k_test_struct0_body + ";");
	QUARK_TEST_VERIFY(result._statement._define_struct);
	QUARK_TEST_VERIFY(*result._statement._define_struct->_struct_def == *make_test_struct0(global));
}





//////////////////////////////////////////////////		program_to_ast()



struct alloc_struct_param : public host_data_i {
	public: virtual ~alloc_struct_param(){};

	alloc_struct_param(const scope_ref_t& struct_def) :
		_struct_def(struct_def)
	{
	}

	scope_ref_t _struct_def;
};


value_t hosts_function__alloc_struct(const std::shared_ptr<host_data_i>& param, const std::vector<value_t>& args){
	const alloc_struct_param& a = dynamic_cast<const alloc_struct_param&>(*param.get());

	const auto instance = make_default_value(a._struct_def);
	return instance;
}

/*
	Take struct definition and creates all types, member variables, constructors, member functions etc.
	??? add constructors and generated stuff.
*/
void install_struct_support(scope_ref_t scope_def, const scope_ref_t& struct_def){
	QUARK_ASSERT(scope_def->check_invariant());
	QUARK_ASSERT(struct_def && struct_def->check_invariant());

	const std::string struct_name = struct_def->_name.to_string();
	const auto struct_name_ident = type_identifier_t::make(struct_name);

	//	Define struct type in current scope.
	auto types_collector2 = define_struct_type(scope_def->_types_collector, struct_name, struct_def);
	scope_ref_t s = resolve_struct_type(types_collector2, struct_name);

	//	Make constructor with same name as struct.
	{
		const auto constructor_name = type_identifier_t::make(struct_name + "_constructor");
		const auto executable = executable_t(hosts_function__alloc_struct, make_shared<alloc_struct_param>(s));
		const auto a = make_function_def(constructor_name, struct_name_ident, {}, scope_def, executable, {});
		types_collector2 = define_function_type(types_collector2, constructor_name.to_string(), a);
	}

	scope_def->_types_collector = types_collector2;
}

//??? Track when / order of definitions and binds so statements can't access them before they're in scope.

std::string read_statements_into_scope_def(scope_ref_t scope_def, const string& s){
	QUARK_ASSERT(scope_def && scope_def->check_invariant());

	auto pos = skip_whitespace(s);
	std::vector<std::shared_ptr<statement_t> > statements;
	while(!pos.empty()){
		const auto statement_pos = read_statement(scope_def, pos);

		//	Definition statements are immediately removed from AST and the types are defined instead.
		if(statement_pos._statement._define_struct){
			const auto a = statement_pos._statement._define_struct;
			install_struct_support(scope_def, a->_struct_def);
		}
		else if(statement_pos._statement._define_function){
			auto function_def = statement_pos._statement._define_function->_function_def;
			const auto function_name = function_def->_name;
			scope_def->_types_collector = define_function_type(scope_def->_types_collector, function_name.to_string(), function_def);
		}
		else{
			scope_def->_executable._statements.push_back(make_shared<statement_t>(statement_pos._statement));
		}
		pos = skip_whitespace(statement_pos._rest);
	}

	return pos;
}


ast_t program_to_ast(const string& program){
	ast_t ast;

	auto pos = program;
	const auto statements_pos = read_statements_into_scope_def(ast._global_scope, program);

	QUARK_ASSERT(ast.check_invariant());
	trace(ast);
	return ast;
}

QUARK_UNIT_TEST("", "program_to_ast()", "kProgram1", ""){
	const string kProgram1 =
		"int main(string args){\n"
		"	return 3;\n"
		"}\n";

	const auto result = program_to_ast(kProgram1);
	QUARK_TEST_VERIFY(result._global_scope->_executable._statements.size() == 0);

	const auto f = make_function_def(
		type_identifier_t::make("main"),
		type_identifier_t::make_int(),
		{
			{ type_identifier_t::make_string(), "args" }
		},
		result._global_scope,
		executable_t({
			make_shared<statement_t>(make__return_statement(expression_t::make_constant(3)))
		}),
		{}
	);

	QUARK_TEST_VERIFY(resolve_function_type(result._global_scope->_types_collector, "main"));
//	QUARK_TEST_VERIFY((*resolve_function_type(result._global_scope->_types_collector, "main") == *f));
}

QUARK_UNIT_TEST("", "program_to_ast()", "three arguments", ""){
	const string kProgram =
		"int f(int x, int y, string z){\n"
		"	return 3;\n"
		"}\n";

	const auto result = program_to_ast(kProgram);
	QUARK_TEST_VERIFY(result._global_scope->_executable._statements.size() == 0);

	const auto f = make_function_def(
		type_identifier_t::make("f"),
		type_identifier_t::make_int(),
		{
			member_t{ type_identifier_t::make_int(), "x" },
			member_t{ type_identifier_t::make_int(), "y" },
			member_t{ type_identifier_t::make_string(), "z" }
		},
		result._global_scope,
		executable_t({
			make_shared<statement_t>(make__return_statement(expression_t::make_constant(3)))
		}),
		{}
	);

//	QUARK_TEST_VERIFY((*resolve_function_type(result._global_scope->_types_collector, "f") == *f));
	QUARK_TEST_VERIFY(resolve_function_type(result._global_scope->_types_collector, "f"));
}

QUARK_UNIT_TEST("", "program_to_ast()", "two functions", ""){
	const string kProgram =
		"string hello(int x, int y, string z){\n"
		"	return \"test abc\";\n"
		"}\n"
		"int main(string args){\n"
		"	return 3;\n"
		"}\n";

	QUARK_TRACE(kProgram);

	const auto result = program_to_ast(kProgram);
	QUARK_TEST_VERIFY(result._global_scope->_executable._statements.size() == 0);

	const auto f = make_function_def(
		type_identifier_t::make("hello"),
		type_identifier_t::make_string(),
		{
			member_t{ type_identifier_t::make_int(), "x" },
			member_t{ type_identifier_t::make_int(), "y" },
			member_t{ type_identifier_t::make_string(), "z" }
		},
		result._global_scope,
		executable_t({
			make_shared<statement_t>(make__return_statement(expression_t::make_constant("test abc")))
		}),
		{}
	);
//	QUARK_TEST_VERIFY((*resolve_function_type(result._global_scope->_types_collector, "hello") == *f));
	QUARK_TEST_VERIFY(resolve_function_type(result._global_scope->_types_collector, "hello"));

	const auto f2 = make_function_def(
		type_identifier_t::make("main"),
		type_identifier_t::make_int(),
		{
			member_t{ type_identifier_t::make_string(), "args" }
		},
		result._global_scope,
		executable_t({
			make_shared<statement_t>(make__return_statement(expression_t::make_constant(3)))
		}),
		{}
	);
//	QUARK_TEST_VERIFY((*resolve_function_type(result._global_scope->_types_collector, "main") == *f2));
	QUARK_TEST_VERIFY(resolve_function_type(result._global_scope->_types_collector, "main"));
}


QUARK_UNIT_TESTQ("program_to_ast()", "Call function a from function b"){
	const string kProgram2 =
	"float testx(float v){\n"
	"	return 13.4;\n"
	"}\n"
	"int main(string args){\n"
	"	float test = testx(1234);\n"
	"	return 3;\n"
	"}\n";
	auto result = program_to_ast(kProgram2);
	QUARK_TEST_VERIFY(result._global_scope->_executable._statements.size() == 0);

	const auto f = make_function_def(
		type_identifier_t::make("testx"),
		type_identifier_t::make_float(),
		{
			member_t{ type_identifier_t::make_float(), "v" }
		},
		result._global_scope,
		executable_t({
			make_shared<statement_t>(make__return_statement(expression_t::make_constant(13.4f)))
		}),
		{}
	);
//	QUARK_TEST_VERIFY((*resolve_function_type(result._global_scope->_types_collector, "testx") == *f));
	QUARK_TEST_VERIFY(resolve_function_type(result._global_scope->_types_collector, "testx"));

/*
	QUARK_TEST_VERIFY((*result._types_collector.resolve_function_type("main") ==
		make_function_def(
			type_identifier_t::make_int(),
			vector<member_t>{
				member_t{ type_identifier_t::make_string(), "arg" }
			},
			{
				make__return_statement(bind_statement_t{"test", function_call_expr_t{"testx", }})
				make__return_statement(make_constant(value_t(13.4f)))
			}
		)
	));
*/
}

////////////////////////////		STRUCT SUPPORT




QUARK_UNIT_TESTQ("program_to_ast()", "Proves we can instantiate a struct"){
	const auto result = program_to_ast(
		"struct pixel { string s; }"
		"string main(){\n"
		"	return \"\";"
		"}\n"
	);


	QUARK_TEST_VERIFY(result._global_scope->_executable._statements.size() == 0);

/*
	QUARK_TEST_VERIFY((*result._types_collector.resolve_function_type("main") ==
		make_function_def(
			type_identifier_t::make_string(),
			vector<member_t>{},
			{
				make__return_statement(make_constant(value_t(3)))
			}
		)
	));
*/
}

QUARK_UNIT_TESTQ("program_to_ast()", "Proves we can address a struct member variable"){
	const auto result = program_to_ast(
		"string main(){\n"
		"	return p.s + a;"
		"}\n"
	);
	QUARK_TEST_VERIFY(result._global_scope->_executable._statements.size() == 0);

/*
	QUARK_TEST_VERIFY((*result._types_collector.resolve_function_type("main") ==
		make_function_def(
			type_identifier_t::make_string(),
			vector<member_t>{},
			{
				make__return_statement(make_constant(value_t(3)))
			}
		)
	));
*/
}



}	//	floyd_parser



