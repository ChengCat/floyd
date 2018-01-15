//
//  parse_implicit_statement.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2018-01-15.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#ifndef parse_implicit_statement_hpp
#define parse_implicit_statement_hpp

#include "quark.h"

struct json_t;
struct seq_t;

namespace floyd {

	std::pair<json_t, seq_t> parse_prefixless_statement(const seq_t& s);

}

#endif /* parse_implicit_statement_hpp */
