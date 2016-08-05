//
//  parser_value.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parser_value.h"

#include "parser_statement.h"


namespace floyd_parser {



void trace(const value_t& e){
	QUARK_TRACE("value_t: " + e.value_and_type_to_string());
}


QUARK_UNIT_TESTQ("value_t()", "null"){
	QUARK_TEST_VERIFY(value_t().plain_value_to_string() == "<null>");
}

QUARK_UNIT_TESTQ("value_t()", "bool"){
	QUARK_TEST_VERIFY(value_t(true).plain_value_to_string() == "true");
}

QUARK_UNIT_TESTQ("value_t()", "int"){
	QUARK_TEST_VERIFY(value_t(13).plain_value_to_string() == "13");
}

QUARK_UNIT_TESTQ("value_t()", "float"){
	QUARK_TEST_VERIFY(value_t(13.5f).plain_value_to_string() == "13.500000");
}

QUARK_UNIT_TESTQ("value_t()", "string"){
	QUARK_TEST_VERIFY(value_t("hello").plain_value_to_string() == "'hello'");
}

//??? more


}	//	floyd_parser
