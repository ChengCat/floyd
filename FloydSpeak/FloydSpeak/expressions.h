//
//  expressions.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 03/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef expressions_hpp
#define expressions_hpp



#include "quark.h"
#include <vector>
#include <string>


namespace floyd_parser {
	struct expression_t;
	struct value_t;

	//////////////////////////////////////////////////		constant expression
	
	//	Has no type, uses value_t.
	

	//////////////////////////////////////////////////		math_operation1_expr_t


	struct math_operation1_expr_t {
		bool operator==(const math_operation1_expr_t& other) const;


		enum operation {
			negate = 20
		};

		const operation _operation;
		const std::shared_ptr<expression_t> _input;
	};

	//////////////////////////////////////////////////		math_operation2_expr_t


	struct math_operation2_expr_t {
		bool operator==(const math_operation2_expr_t& other) const;


		enum operation {
			add = 10,
			subtract,
			multiply,
			divide
		};

		const operation _operation;
		const std::shared_ptr<expression_t> _left;
		const std::shared_ptr<expression_t> _right;
	};


	//////////////////////////////////////////////////		function_call_expr_t


	struct function_call_expr_t {
		bool operator==(const function_call_expr_t& other) const{
			return _function_name == other._function_name && _inputs == other._inputs;
		}

		//??? Change for a path.
		const std::string _function_name;
		const std::vector<std::shared_ptr<expression_t>> _inputs;
	};


	//////////////////////////////////////////////////		load_expr_t

	/*
		Supports reading a named variable, like "int a = 10; print(a);"
	*/
	struct load_expr_t {
		bool operator==(const load_expr_t& other) const;


		std::shared_ptr<expression_t> _address;
	};


	//////////////////////////////////////////////////		resolve_member_expr_t



	/*
		Supports reading a named variable, like "int a = 10; print(a);"
	*/
	struct resolve_member_expr_t {
		bool operator==(const resolve_member_expr_t& other) const;

		std::shared_ptr<expression_t> _parent_address;
		const std::string _member_name;
	};


	//////////////////////////////////////////////////		lookup_element_expr_t

	/*
		Looks up using a key. They key can be a sub-expression. Can be any type: index, string etc.
	*/
	struct lookup_element_expr_t {
		bool operator==(const lookup_element_expr_t& other) const;

		std::shared_ptr<expression_t> _parent_address;
		std::shared_ptr<expression_t> _lookup_key;
	};


	std::string to_string(const expression_t& e);

	//////////////////////////////////////////////////		expression_t


	struct expression_t {
		public: bool check_invariant() const;


		public: expression_t(const std::shared_ptr<value_t>& a) :
			_constant(a)
		{
			_debug_aaaaaaaaaaaaaaaaaaaaaaa = to_string(*this);
			QUARK_ASSERT(check_invariant());
		}

		public: expression_t(const std::shared_ptr<math_operation1_expr_t>& a) :
			_math1(a)
		{
			_debug_aaaaaaaaaaaaaaaaaaaaaaa = to_string(*this);
			QUARK_ASSERT(check_invariant());
		}

		public: expression_t(const std::shared_ptr<math_operation2_expr_t>& a) :
			_math2(a)
		{
			_debug_aaaaaaaaaaaaaaaaaaaaaaa = to_string(*this);
			QUARK_ASSERT(check_invariant());
		}

		public: expression_t(const std::shared_ptr<function_call_expr_t>& a) :
			_call(a)
		{
			_debug_aaaaaaaaaaaaaaaaaaaaaaa = to_string(*this);
			QUARK_ASSERT(check_invariant());
		}

		public: expression_t(const std::shared_ptr<load_expr_t>& a) :
			_load(a)
		{
			_debug_aaaaaaaaaaaaaaaaaaaaaaa = to_string(*this);
			QUARK_ASSERT(check_invariant());
		}

		public: expression_t(const std::shared_ptr<resolve_member_expr_t>& a) :
			_resolve_member(a)
		{
			_debug_aaaaaaaaaaaaaaaaaaaaaaa = to_string(*this);
			QUARK_ASSERT(check_invariant());
		}

		public: expression_t(const std::shared_ptr<lookup_element_expr_t>& a) :
			_lookup_element(a)
		{
			_debug_aaaaaaaaaaaaaaaaaaaaaaa = to_string(*this);
			QUARK_ASSERT(check_invariant());
		}



		public: bool operator==(const expression_t& other) const;


		//////////////////////////		STATE
		public: std::string _debug_aaaaaaaaaaaaaaaaaaaaaaa;

		/*
			Only one of there are used at any time.
		*/
		public: std::shared_ptr<value_t> _constant;
		public: std::shared_ptr<math_operation1_expr_t> _math1;
		public: std::shared_ptr<math_operation2_expr_t> _math2;
		public: std::shared_ptr<function_call_expr_t> _call;
		public: std::shared_ptr<load_expr_t> _load;
		public: std::shared_ptr<resolve_member_expr_t> _resolve_member;
		public: std::shared_ptr<lookup_element_expr_t> _lookup_element;
	};



	//////////////////////////////////////////////////		make specific expressions

	expression_t make_constant(const value_t& value);
	expression_t make_constant(const std::string& s);
	expression_t make_constant(const int i);
	expression_t make_constant(const float f);

	expression_t make_math_operation1(math_operation1_expr_t::operation op, const expression_t& input);
	expression_t make_math_operation2(math_operation2_expr_t::operation op, const expression_t& left, const expression_t& right);

	expression_t make_function_call(const std::string& function_name, const std::vector<expression_t>& inputs);
	expression_t make_function_call(const std::string& function_name, const std::vector<std::shared_ptr<expression_t>>& inputs);

	expression_t make_load(const expression_t& address_expression);
	expression_t make_load_variable(const std::string& name);

	/*
		parent_address is optional.
	*/
	expression_t make_resolve_member(const std::shared_ptr<expression_t>& parent_address, const std::string& member_name);
	expression_t make_resolve_member(const std::string& member_name);

	expression_t make_lookup(const expression_t& parent_address, const expression_t& lookup_key);


	//////////////////////////////////////////////////		trace()


	void trace(const math_operation1_expr_t& e);
	void trace(const math_operation2_expr_t& e);
	void trace(const function_call_expr_t& e);
	void trace(const load_expr_t& e);
	void trace(const resolve_member_expr_t& e);
	void trace(const lookup_element_expr_t& e);
	void trace(const expression_t& e);



	/*
	"hello[\"troll\"].kitty[10].cat" =>
	"(@load (@resolve (@lookup (@resolve (@lookup (@resolve nullptr 'hello') (@k <string>'troll')) 'kitty') (@k <int>10)) 'cat'))"


	### Encode using JSON! Easy to copy-paste, user JSON validators etc:

	["@resolve",
		["@lookup",
			["@resolve",
				["@lookup",
					["@resolve", "nullptr", "hello"],
					["@k", "<string>", "troll"]
				],
				"kitty"
			],
			["@k", "<int>", "10"]
		],
		"cat"
	]
	*/

	std::string to_string(const expression_t& e);

}	//	floyd_parser


#endif /* expressions_hpp */