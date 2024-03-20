//
//  emscr_async_bridge_index.hpp
//  Copyright (c) 2014-2019, MyMonero.com
// Copyright (c)      2023, The Beldex Project
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without modification, are
//  permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice, this list of
//	conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above copyright notice, this list
//	of conditions and the following disclaimer in the documentation and/or other
//	materials provided with the distribution.
//
//  3. Neither the name of the copyright holder nor the names of its contributors may be
//	used to endorse or promote products derived from this software without specific
//	prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
//  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
//  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
//  THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
//  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
//  THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//

#ifndef emscr_async_bridge_index_hpp
#define emscr_async_bridge_index_hpp
//

#include <string>
#include <boost/optional.hpp>

#include "cryptonote_config.h" 
#include "SendFundsFormSubmissionController.hpp"
#include "walletf2.h"
#include "tx_construction_data.h"
#include "cryptonote_tx_utils.h"
#include "cryptonote_basic.h"
#include "master_node_list.h"





//
namespace emscr_SendFunds_bridge
{
	using namespace std;
	using namespace boost;
	using namespace cryptonote;
	using namespace beldex_transfer_utils;
	//
	// Bridging Functions - these take and return JSON strings plus unique ids that are used internally for matching up calls from C++ to JS over emscripten, with the cbs that will fulfill the promises and futures being used to coordinate e.g. async routines like send_funds.
	//
	// To use these functions, the appropriate emscripten-side JS fn handlers must exist, which must be hooked up to perform the e.g. networking or transport requests they are specced to perform, then upon the async completion of those requests, call the appropate "cb_I+"-named function to allow the internal evaluation of the routine entrypoint to complete.
	//
	// Public interface:
	void send_funds(const string &args_string);
	std::string register_funds(const string &args_string);
	void send_cb__authentication(const string &args_string);
	void send_cb_I__got_unspent_outs(const string &args_string);
	void send_cb_II__got_random_outs(const string &args_string);
	void send_cb_III__submitted_tx(const string &args_string);
	//
	// Internal
	void send_app_handler__error_json(const string &ret_json_string);
	void send_app_handler__error_msg(const string &err_msg);
	void send_app_handler__error_code(
		SendFunds::PreSuccessTerminalCode code,
		boost::optional<string> msg,
		boost::optional<CreateTransactionErrorCode> createTx_errCode,
		// for display / information purposes on errCode=needMoreMoneyThanFound during step1:
		boost::optional<uint64_t> spendable_balance,
		boost::optional<uint64_t> required_balance
	);
}

#endif /* serial_bridge_index_hpp */
