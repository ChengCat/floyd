//
//  floyd_parser.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 10/04/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_parser_h
#define floyd_parser_h

#include "quark.h"

#include "parser_types.h"
#include "parser_primitives.h"

namespace floyd_parser {
	struct expression_t;
	struct statement_t;


	//////////////////////////////////////////////////		ast_t


	struct ast_t {
		public: bool check_invariant() const {
			return true;
		}


		/////////////////////////////		STATE
		frontend_types_collector_t _types_collector;
		identifiers_t _identifiers;
		std::vector<statement_t> _top_level_statements;
	};


	ast_t program_to_ast(const identifiers_t& builtins, const std::string& program);


	/*
		Evaluates an expression as far as possible.
	*/
	expression_t evaluate3(const identifiers_t& identifiers, const expression_t& e);


	value_t run_function(const identifiers_t& identifiers, const function_def_expr_t& f, const std::vector<value_t>& args);

}	//	floyd_parser



#endif /* floyd_parser_h */
