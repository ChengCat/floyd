//
//  floyd_vm.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 10/04/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "floyd_vm.h"


#include "steady_vector.h"
#include <string>
#include <memory>
#include <map>
#include <iostream>
#include <cmath>


#include "parser_value.hpp"
#include "parser_statement.hpp"




using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;
using std::unique_ptr;
using std::make_shared;


shared_ptr<const floyd_parser::function_def_expr_t> find_global_function(const vm_t& vm, const string& name){
/*
	const auto it = std::find_if(vm._ast._top_level_statements.begin(), vm._ast._top_level_statements.end(), [=] (const statement_t& s) { return s._bind_statement != nullptr && s._bind_statement->_identifier == name; });
*/
	const auto it = vm._ast._identifiers._functions.find(name);
	if(it == vm._ast._identifiers._functions.end()){
		return nullptr;
	}

	return it->second;
}

struct vm_stack_frame {
	std::map<string, floyd_parser::value_t> locals;
	int _statement_index;
};

floyd_parser::value_t call_function(vm_t& vm, shared_ptr<const floyd_parser::function_def_expr_t> f, const vector<floyd_parser::value_t>& args){
	QUARK_ASSERT(vm.check_invariant());
	for(const auto i: args){ QUARK_ASSERT(i.check_invariant()); };

	const auto r = run_function(vm._ast, *f, args);
	return r;
}

QUARK_UNIT_TESTQ("call_function()", "minimal program"){
	auto ast = program_to_ast(floyd_parser::identifiers_t(),
		"int main(string args){\n"
		"	return 3 + 4;\n"
		"}\n"
	);
	auto vm = vm_t(ast);
	const auto f = find_global_function(vm, "main");
	const auto result = call_function(vm, f, vector<floyd_parser::value_t>{ floyd_parser::value_t("program_name 1 2 3") });
	QUARK_TEST_VERIFY(result == floyd_parser::value_t(7));
}


QUARK_UNIT_TESTQ("call_function()", "minimal program 2"){
	auto ast = floyd_parser::program_to_ast(floyd_parser::identifiers_t(),
		"int main(string args){\n"
		"	return \"123\" + \"456\";\n"
		"}\n"
	);
	auto vm = vm_t(ast);
	const auto f = find_global_function(vm, "main");
	const auto result = call_function(vm, f, vector<floyd_parser::value_t>{ floyd_parser::value_t("program_name 1 2 3") });
	QUARK_TEST_VERIFY(result == floyd_parser::value_t("123456"));
}

QUARK_UNIT_TESTQ("call_function()", "define additional function, call it several times"){
	auto ast = floyd_parser::program_to_ast(floyd_parser::identifiers_t(),
		"int myfunc(){ return 5; }\n"
		"int main(string args){\n"
		"	return myfunc() + myfunc() * 2;\n"
		"}\n"
	);
	auto vm = vm_t(ast);
	const auto f = find_global_function(vm, "main");
	const auto result = call_function(vm, f, vector<floyd_parser::value_t>{ floyd_parser::value_t("program_name 1 2 3") });
	QUARK_TEST_VERIFY(result == floyd_parser::value_t(15));
}





QUARK_UNIT_TESTQ("call_function()", "use function inputs"){
	auto ast = program_to_ast(floyd_parser::identifiers_t(),
		"int main(string args){\n"
		"	return \"-\" + args + \"-\";\n"
		"}\n"
	);
	auto vm = vm_t(ast);
	const auto f = find_global_function(vm, "main");
	const auto result = call_function(vm, f, vector<floyd_parser::value_t>{ floyd_parser::value_t("xyz") });
	QUARK_TEST_VERIFY(result == floyd_parser::value_t("-xyz-"));

	const auto result2 = call_function(vm, f, vector<floyd_parser::value_t>{ floyd_parser::value_t("Hello, world!") });
	QUARK_TEST_VERIFY(result2 == floyd_parser::value_t("-Hello, world!-"));
}

//### Check return value type.

QUARK_UNIT_TESTQ("call_function()", "use local variables"){
	auto ast = program_to_ast(floyd_parser::identifiers_t(),
		"string myfunc(string t){ return \"<\" + t + \">\"; }\n"
		"string main(string args){\n"
		"	 string a = \"--\"; string b = myfunc(args) ; return a + args + b + a;\n"
		"}\n"
	);
	auto vm = vm_t(ast);
	const auto f = find_global_function(vm, "main");
	const auto result = call_function(vm, f, vector<floyd_parser::value_t>{ floyd_parser::value_t("xyz") });
	QUARK_TEST_VERIFY(result == floyd_parser::value_t("--xyz<xyz>--"));

	const auto result2 = call_function(vm, f, vector<floyd_parser::value_t>{ floyd_parser::value_t("123") });
	QUARK_TEST_VERIFY(result2 == floyd_parser::value_t("--123<123>--"));
}





/*
	Quckie that compiles a program and calls its main() with the args.
*/
floyd_parser::value_t run_main(const string& source, const vector<floyd_parser::value_t>& args){
	QUARK_ASSERT(source.size() > 0);
	auto ast = program_to_ast(floyd_parser::identifiers_t(), source);
	auto vm = vm_t(ast);
	const auto f = find_global_function(vm, "main");
	const auto r = call_function(vm, f, args);
	return r;
}

QUARK_UNIT_TESTQ("run_main()", "minimal program 2"){
	const auto result = run_main(
		"int main(string args){\n"
		"	return \"123\" + \"456\";\n"
		"}\n",
		vector<floyd_parser::value_t>{floyd_parser::value_t("program_name 1 2 3 4")}
	);
	QUARK_TEST_VERIFY(result == floyd_parser::value_t("123456"));
}