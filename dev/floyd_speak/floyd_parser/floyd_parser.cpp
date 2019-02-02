//
//  main.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 27/03/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "floyd_parser.h"

#include "parse_prefixless_statement.h"
#include "parse_statement.h"
#include "parse_expression.h"
#include "parse_function_def.h"
#include "parse_struct_def.h"
#include "parse_protocol_def.h"
#include "parser_primitives.h"
#include "json_parser.h"
#include "utils.h"


namespace floyd {


using namespace std;

/*
	AST ABSTRACT SYNTAX TREE

https://en.wikipedia.org/wiki/Abstract_syntax_tree

https://en.wikipedia.org/wiki/Parsing_expression_grammar
https://en.wikipedia.org/wiki/Parsing
*/


std::pair<ast_json_t, seq_t> parse_statement(const seq_t& s){
	const auto pos = skip_whitespace(s);
	if(is_first(pos, "{")){
		return parse_block(pos);
	}
	else if(is_first(pos, keyword_t::k_return)){
		return parse_return_statement(pos);
	}
	else if(is_first(pos, keyword_t::k_struct)){
		return parse_struct_definition(pos);
	}
	else if(is_first(pos, keyword_t::k_protocol)){
		return parse_protocol_definition(pos);
	}
	else if(is_first(pos, keyword_t::k_if)){
		return  parse_if_statement(pos);
	}
	else if(is_first(pos, keyword_t::k_for)){
		return parse_for_statement(pos);
	}
	else if(is_first(pos, keyword_t::k_while)){
		return parse_while_statement(pos);
	}
	else if(is_first(pos, keyword_t::k_func)){
		return parse_function_definition2(pos);
	}
	else if(is_first(pos, keyword_t::k_let)){
		return parse_bind_statement(pos);
	}
	else if(is_first(pos, keyword_t::k_mutable)){
		return parse_bind_statement(pos);
	}
	else if(is_first(pos, keyword_t::k_software_system)){
		return parse_software_system(pos);
	}
	else if(is_first(pos, keyword_t::k_container_def)){
		return parse_container_def(pos);
	}
	else {
		return parse_prefixless_statement(pos);
	}
}

QUARK_UNIT_TEST("", "parse_statement()", "", ""){
	ut_compare_jsons(
		parse_statement(seq_t("let int x = 10;")).first._value,
		parse_json(seq_t(R"([0, "bind", "^int", "x", ["k", 10, "^int"]])")).first
	);
}
QUARK_UNIT_TEST("", "parse_statement()", "", ""){
	ut_compare_jsons(
		parse_statement(seq_t("func int f(string name){ return 13; }")).first._value,
		parse_json(seq_t(R"(
			[
				0,
				"def-func",
				{
					"args": [{ "name": "name", "type": "^string" }],
					"name": "f",
					"return_type": "^int",
					"statements": [
						[25, "return", ["k", 13, "^int"]]
					],
					"impure": false
				}
			]
		)")).first
	);
}

QUARK_UNIT_TEST("", "parse_statement()", "", ""){
	ut_compare_jsons(
		parse_statement(seq_t("let int x = f(3);")).first._value,
		parse_json(seq_t(R"([0, "bind", "^int", "x", ["call", ["@", "f"], [["k", 3, "^int"]]]])")).first
	);
}


//	"a = 1; print(a)"
parse_result_t parse_statements_no_brackets(const seq_t& s){
	vector<json_t> statements;

	auto pos = skip_whitespace(s);

	while(pos.empty() == false){
		const auto statement_pos = parse_statement(pos);
		QUARK_ASSERT(statement_pos.second.pos() >= pos.pos());

		statements.push_back(statement_pos.first._value);

		auto pos2 = skip_whitespace(statement_pos.second);

		//	Skip optional ;
		while(pos2.empty() == false && pos2.first1_char() == ';'){
			pos2 = pos2.rest1();
			pos2 = skip_whitespace(pos2);
		}

		QUARK_ASSERT(pos2.pos() >= pos.pos());
		pos = pos2;
	}
	return { ast_json_t::make(statements), pos };
}

//	"{ a = 1; print(a) }"
parse_result_t parse_statements_bracketted(const seq_t& s){
	vector<json_t> statements;

	auto pos = skip_whitespace(s);
	pos = read_required(pos, "{");
	pos = skip_whitespace(pos);

	while(pos.empty() == false && pos.first() != "}"){
		const auto statement_pos = parse_statement(pos);
		QUARK_ASSERT(statement_pos.second.pos() >= pos.pos());

		statements.push_back(statement_pos.first._value);

		auto pos2 = skip_whitespace(statement_pos.second);

		//	Skip optional ;
		while(pos2.empty() == false && pos2.first1_char() == ';'){
			pos2 = pos2.rest1();
			pos2 = skip_whitespace(pos2);
		}

		QUARK_ASSERT(pos2.pos() >= pos.pos());
		pos = pos2;
	}
	if(pos.first() == "}"){
		return { ast_json_t::make(statements), pos.rest() };
	}
	else{
		throw std::runtime_error("Missing end bracket \'}\'.");
	}
}

QUARK_UNIT_TEST("", "parse_statements_bracketted()", "", ""){
	ut_compare_jsons(
		parse_statement_body(seq_t(" { } ")).ast._value,
		parse_json(seq_t(
			R"(
				[]
			)"
		)).first
	);
}
QUARK_UNIT_TEST("", "parse_statements_bracketted()", "", ""){
	ut_compare_jsons(
		parse_statement_body(seq_t(" { let int x = 1; let int y = 2; } ")).ast._value,
		parse_json(seq_t(
			R"(
				[
					[3, "bind", "^int", "x", ["k", 1, "^int"]],
					[18, "bind", "^int", "y", ["k", 2, "^int"]]
				]
			)"
		)).first
	);
}





/*

extern const std::string k_builtin_types_and_constants___xxx = R"(
	let double cmath_pi = 3.14159265358979323846

	struct cpu_address_t {
		int address
	}

	struct size_t {
		int address
	}

	struct file_pos_t {
		int pos
	}

	struct time_ms_t {
		int pos
	}

	struct uuid_t {
		int high64
		int low64
	}


	struct ip_address_t {
		int high64
		int low_64_bits
	}


	struct url_t {
		string absolute_url
	}

	struct url_parts_t {
		string protocol
		string domain
		string path
		[string:string] query_strings
		int port
	}

	struct quick_hash_t {
		int hash
	}

	struct key_t {
		quick_hash_t hash
	}

	struct date_t {
		string utc_date
	}

	struct sha1_t {
		string ascii40
	}


	struct relative_path_t {
		string relative_path
	}

	struct absolute_path_t {
		string absolute_path
	}

	struct binary_t {
		string bytes
	}

	struct text_location_t {
		absolute_path_t source_file
		int line_number
		int pos_in_line
	}

	struct seq_t {
		string str
		size_t pos
	}

	struct text_t {
		binary_t data
	}

	struct text_resource_id {
		quick_hash_t id
	}

	struct image_id_t {
		int id
	}

	struct color_t {
		double red
		double green
		double blue
		double alpha
	}

	struct vector2_t {
		double x
		double y
	}



	let color__black = color_t(0.0, 0.0, 0.0, 1.0)
	let color__white = color_t(1.0, 1.0, 1.0, 1.0)


	func color_t add_colors(color_t a, color_t b){
		return color_t(
			a.red + b.red,
			a.green + b.green,
			a.blue + b.blue,
			a.alpha + b.alpha
		)
	}


	////////////////////////////		FILE SYSTEM TYPES


	struct fsentry_t {
		string type	//	"dir" or "file"
		string abs_parent_path
		string name
	}

	struct fsentry_info_t {
		string type	//	"file" or "dir"
		string name
		string abs_parent_path

		string creation_date
		string modification_date
		int file_size
	}

	struct fs_environment_t {
		string home_dir
		string documents_dir
		string desktop_dir

		string hidden_persistence_dir
		string preferences_dir
		string cache_dir
		string temp_dir

		string executable_dir
	}
)";


QUARK_UNIT_TEST_VIP("", "parse_statements_bracketted()", "", ""){
	ut_compare_jsons(
		parse_program2(k_builtin_types_and_constants___xxx).ast._value,
		parse_json(seq_t(
			R"(
				[]
			)"
		)).first
	);
}
*/


parse_result_t parse_program2(const string& program){
	const auto statements_pos = parse_statements_no_brackets(seq_t(program));
/*
	const auto line_numbers2 = mapf<int>(
		statements_pos.line_numbers,
		[&](const auto& e){
			return e - pre_line_count;
		}
	);
*/

	return { statements_pos.ast, statements_pos.pos };
}


//////////////////////////////////////////////////		Test programs






const std::string k_test_program_0_source = "func int main(){ return 3; }";
const std::string k_test_program_0_parserout = R"(
	[
		[
			0,
			"def-func",
			{
				"args": [],
				"name": "main",
				"return_type": "^int",
				"statements": [
					[ 17, "return", [ "k", 3, "^int" ] ]
				],
				"impure": false
			}
		]
	]
)";

QUARK_UNIT_TEST("", "parse_program1()", "k_test_program_0_source", ""){
	ut_compare_jsons(
		parse_program2(k_test_program_0_source).ast._value,
		parse_json(seq_t(k_test_program_0_parserout)).first
	);
}



const std::string k_test_program_1_source =
	"func int main(string args){\n"
	"	return 3;\n"
	"}\n";
const std::string k_test_program_1_parserout = R"(
	[
		[
			0,
			"def-func",
			{
				"args": [
					{ "name": "args", "type": "^string" }
				],
				"name": "main",
				"return_type": "^int",
				"statements": [
					[ 29, "return", [ "k", 3, "^int" ] ]
				],
				"impure": false
			}
		]
	]
)";

QUARK_UNIT_TEST("", "parse_program1()", "k_test_program_1_source", ""){
	ut_compare_jsons(
		parse_program2(k_test_program_1_source).ast._value,
		parse_json(seq_t(k_test_program_1_parserout)).first
	);
}



const char k_test_program_100_parserout[] = R"(
	[
		[
			5,
			"def-struct",
			{
				"members": [
					{ "name": "red", "type": "^double" },
					{ "name": "green", "type": "^double" },
					{ "name": "blue", "type": "^double" }
				],
				"name": "pixel"
			}
		],
		[
			65,
			"def-func",
			{
				"args": [{ "name": "p", "type": "#pixel" }],
				"name": "get_grey",
				"return_type": "^double",
				"statements": [
					[
						96,
						"return",
						[
							"/",
							[
								"+",
								["+", ["->", ["@", "p"], "red"], ["->", ["@", "p"], "green"]],
								["->", ["@", "p"], "blue"]
							],
							["k", 3.0, "^double"]
						]
					]
				],
				"impure": false
			}
		],
		[
			144,
			"def-func",
			{
				"args": [],
				"name": "main",
				"return_type": "^double",
				"statements": [
					[
						169,
						"bind",
						"#pixel",
						"p",
						["call", ["@", "pixel"], [["k", 1, "^int"], ["k", 0, "^int"], ["k", 0, "^int"]]]
					],
					[204, "return", ["call", ["@", "get_grey"], [["@", "p"]]]]
				],
				"impure": false
			}
		]
	]
)";

QUARK_UNIT_TEST("", "parse_program2()", "k_test_program_100_source", ""){
	ut_compare_jsons(
		parse_program2(
			R"(
				struct pixel { double red; double green; double blue; }
				func double get_grey(pixel p){ return (p.red + p.green + p.blue) / 3.0; }

				func double main(){
					let pixel p = pixel(1, 0, 0);
					return get_grey(p);
				}
			)"
		).ast._value,
		parse_json(seq_t(k_test_program_100_parserout)).first
	);
}

}	//	namespace floyd
