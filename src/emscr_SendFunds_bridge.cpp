//
//  emscr_async_bridge_index.cpp
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
//
#include "emscr_SendFunds_bridge.hpp"
//
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <emscripten.h>
#include <unordered_map>
#include <memory>
//
#include "epee/string_tools.h"
#include "wallet_errors.h"
//

#include "serial_bridge_utils.hpp"
#include "SendFundsFormSubmissionController.hpp"
#include "beldex_economy.h"

// #include "nlohmann/json.hpp" // Include jsoncpp library

// using json = nlohmann::json;

// #include "walletf2.h"
// #include "wallet2.h" // this header file cannot be include as it uses cpr as submodule that is responsible for calling rpc

//
//
using namespace std;
using namespace boost;
using namespace SendFunds;
//
using namespace serial_bridge_utils;
using namespace beldex_send_routine;
using namespace beldex_transfer_utils;
using namespace emscr_SendFunds_bridge;
//
// Runtime - Memory
//
SendFunds::FormSubmissionController *controller_ptr = NULL;
//
// To-JS fn decls - Status updates and routine completions
static void send_app_handler__status_update(ProcessStep code)
{
	boost::property_tree::ptree root;
	root.put("code", code); // not 64bit so sendable in JSON
	auto ret_json_string = ret_json_from_root(root);
	EM_ASM_(
		{
			const JS__req_params_string = Module.UTF8ToString($0);
			const JS__req_params = JSON.parse(JS__req_params_string);
			Module.fromCpp__SendFundsFormSubmission__status_update(JS__req_params); // Module must implement this!
		},
		ret_json_string.c_str());
}
void emscr_SendFunds_bridge::send_app_handler__error_json(const string &ret_json_string)
{
	EM_ASM_(
		{
			const JS__req_params_string = Module.UTF8ToString($0);
			const JS__req_params = JSON.parse(JS__req_params_string);
			Module.fromCpp__SendFundsFormSubmission__error(JS__req_params); // Module must implement this!
		},
		ret_json_string.c_str());
	THROW_WALLET_EXCEPTION_IF(controller_ptr == NULL, error::wallet_internal_error, "expected non-NULL controller_ptr");
	delete controller_ptr; // having finished
	controller_ptr = NULL;
}
void emscr_SendFunds_bridge::send_app_handler__error_msg(const string &err_msg)
{
	send_app_handler__error_json(error_ret_json_from_message(std::move(err_msg)));
}
void emscr_SendFunds_bridge::send_app_handler__error_code(
	SendFunds::PreSuccessTerminalCode code,
	boost::optional<string> msg,
	boost::optional<CreateTransactionErrorCode> createTx_errCode,
	// for display / information purposes on errCode=needMoreMoneyThanFound during step1:
	boost::optional<uint64_t> spendable_balance,
	boost::optional<uint64_t> required_balance)
{
	boost::property_tree::ptree root;
	root.put(ret_json_key__any__err_code(), code);
	if (msg)
	{
		root.put(ret_json_key__any__err_msg(), std::move(*msg));
	}
	if (createTx_errCode != boost::none)
	{
		root.put("createTx_errCode", createTx_errCode);
	}
	if (spendable_balance != boost::none)
	{
		root.put(ret_json_key__send__spendable_balance(), std::move(RetVals_Transforms::str_from(*spendable_balance)));
	}
	if (required_balance != boost::none)
	{
		root.put(ret_json_key__send__required_balance(), std::move(RetVals_Transforms::str_from(*required_balance)));
	}
	send_app_handler__error_json(ret_json_from_root(root));
}
//
void send_app_handler__success(const Success_RetVals &success_retVals)
{
	boost::property_tree::ptree root;
	root.put(ret_json_key__send__used_fee(), std::move(RetVals_Transforms::str_from(success_retVals.used_fee)));
	root.put(ret_json_key__send__total_sent(), std::move(RetVals_Transforms::str_from(success_retVals.total_sent)));
	root.put(ret_json_key__send__mixin(), success_retVals.mixin); // this is a uint32 so it can be sent in JSON
	if (success_retVals.final_payment_id)
	{
		root.put(ret_json_key__send__final_payment_id(), std::move(*(success_retVals.final_payment_id)));
	}
	root.put(ret_json_key__send__serialized_signed_tx(), std::move(success_retVals.signed_serialized_tx_string));
	root.put(ret_json_key__send__tx_hash(), std::move(success_retVals.tx_hash_string));
	root.put(ret_json_key__send__tx_key(), std::move(success_retVals.tx_key_string));
	root.put(ret_json_key__send__tx_pub_key(), std::move(success_retVals.tx_pub_key_string));

	string target_address_str;
	size_t nTargAddrs = success_retVals.target_addresses.size();
	for (size_t i = 0; i < nTargAddrs; ++i)
	{
		if (nTargAddrs == 1)
		{
			target_address_str += success_retVals.target_addresses[i];
		}
		else
		{
			if (i == 0)
			{
				target_address_str += "[";
			}

			target_address_str += success_retVals.target_addresses[i];

			if (i < nTargAddrs - 1)
			{
				target_address_str += ", ";
			}
			else
			{
				target_address_str += "]";
			}
		}
	}

	root.put("target_address", target_address_str);
	root.put("final_total_wo_fee", std::move(RetVals_Transforms::str_from(success_retVals.final_total_wo_fee)));
	root.put("isXMRAddressIntegrated", std::move(RetVals_Transforms::str_from(success_retVals.isXMRAddressIntegrated)));
	if (success_retVals.integratedAddressPIDForDisplay)
	{
		root.put("integratedAddressPIDForDisplay", std::move(*(success_retVals.integratedAddressPIDForDisplay)));
	}
	//
	EM_ASM_(
		{
			const JS__req_params_string = Module.UTF8ToString($0);
			const JS__req_params = JSON.parse(JS__req_params_string);
			Module.fromCpp__SendFundsFormSubmission__success(JS__req_params); // Module must implement this!
		},
		ret_json_from_root(root).c_str());
	THROW_WALLET_EXCEPTION_IF(controller_ptr == NULL, error::wallet_internal_error, "expected non-NULL controller_ptr");
	delete controller_ptr; // having finished
	controller_ptr = NULL;
}
//
// From-JS function decls
void emscr_SendFunds_bridge::send_funds(const string &args_string)
{
	std::cout << "Enter into send_funds" << std::endl;

	std::cout << "args_string in send_funds :" << args_string << std::endl;

	boost::property_tree::ptree json_root;
	if (!parsed_json_root(args_string, json_root))
	{
		// (it will already have thrown an exception)
		send_app_handler__error_msg(error_ret_json_from_message("Invalid JSON"));
		return;
	}

	std::cout << "args_string in send_funds after parsing :" << args_string << std::endl;

	const auto &destinations = json_root.get_child("destinations");
	vector<string> dest_addrs, dest_amounts;
	dest_addrs.reserve(destinations.size());
	dest_amounts.reserve(destinations.size());

	for (const auto &dest : destinations)
	{
		dest_addrs.emplace_back(dest.second.get<string>("to_address"));
		dest_amounts.emplace_back(dest.second.get<string>("send_amount"));
	}

	Parameters parameters{
		json_root.get<bool>("fromWallet_didFailToInitialize"),
		json_root.get<bool>("fromWallet_didFailToBoot"),
		json_root.get<bool>("fromWallet_needsImport"),
		//
		json_root.get<bool>("requireAuthentication"),
		//
		std::move(dest_amounts),
		json_root.get<bool>("is_sweeping"),
		(uint32_t)stoul(json_root.get<string>("priority")),
		//
		json_root.get<bool>("hasPickedAContact"),
		json_root.get_optional<string>("contact_payment_id"),
		json_root.get_optional<bool>("contact_hasOpenAliasAddress"),
		json_root.get_optional<string>("cached_OAResolved_address"),
		json_root.get_optional<string>("contact_address"),
		//
		nettype_from_string(json_root.get<string>("nettype_string")),
		json_root.get<string>("from_address_string"),
		json_root.get<string>("sec_viewKey_string"),
		json_root.get<string>("sec_spendKey_string"),
		json_root.get<string>("pub_spendKey_string"),
		//
		std::move(dest_addrs),
		//
		json_root.get_optional<string>("resolvedAddress"),
		json_root.get<bool>("resolvedAddress_fieldIsVisible"),
		//
		json_root.get_optional<string>("manuallyEnteredPaymentID"),
		json_root.get<bool>("manuallyEnteredPaymentID_fieldIsVisible"),
		//
		json_root.get_optional<string>("resolvedPaymentID"),
		json_root.get<bool>("resolvedPaymentID_fieldIsVisible"),
		//
		[]( // preSuccess_nonTerminal_validationMessageUpdate_fn
			ProcessStep step) -> void
		{
			send_app_handler__status_update(step);
		},
		[]( // failure_fn
			SendFunds::PreSuccessTerminalCode code,
			boost::optional<string> msg,
			boost::optional<CreateTransactionErrorCode> createTx_errCode,
			boost::optional<uint64_t> spendable_balance,
			boost::optional<uint64_t> required_balance) -> void
		{
			send_app_handler__error_code(code, msg, createTx_errCode, spendable_balance, required_balance);
		},
		[]() -> void { // preSuccess_passedValidation_willBeginSending
			EM_ASM_(
				{
					Module.fromCpp__SendFundsFormSubmission__willBeginSending({}); // Module must implement this!
				});
		},
		//
		[]() -> void { // canceled_fn
			EM_ASM_(
				{
					Module.fromCpp__SendFundsFormSubmission__canceled({}); // Module must implement this!
				});
			THROW_WALLET_EXCEPTION_IF(controller_ptr == NULL, error::wallet_internal_error, "expected non-NULL controller_ptr");
			delete controller_ptr; // having finished
			controller_ptr = NULL;
		},
		[](SendFunds::Success_RetVals retVals) -> void // success_fn
		{
			send_app_handler__success(retVals);
		}};

	controller_ptr = new SendFunds::FormSubmissionController{parameters}; // heap alloc
	if (!controller_ptr)
	{ // exception will be thrown if oom but JIC, since null ptrs are somehow legal in WASM
		send_app_handler__error_msg("Out of memory (heap vals container)");
		return;
	}
	(*controller_ptr).set__authenticate_fn([]() -> void { // authenticate_fn - this is not guaranteed to be called but it will be if requireAuthentication is true
		EM_ASM_(
			{
				Module.fromCpp__SendFundsFormSubmission__authenticate(); // Module must implement this!
			});
	});
	(*controller_ptr).set__get_unspent_outs_fn([](LightwalletAPI_Req_GetUnspentOuts req_params) -> void { // get_unspent_outs
		boost::property_tree::ptree req_params_root;
		req_params_root.put("address", req_params.address);
		req_params_root.put("view_key", req_params.view_key);
		req_params_root.put("amount", req_params.amount);
		req_params_root.put("dust_threshold", req_params.dust_threshold);
		req_params_root.put("use_dust", req_params.use_dust);
		req_params_root.put("mixin", req_params.mixin);
		stringstream req_params_ss;
		boost::property_tree::write_json(req_params_ss, req_params_root, false /*pretty*/);
		EM_ASM_(
			{
				const JS__req_params_string = Module.UTF8ToString($0);
				const JS__req_params = JSON.parse(JS__req_params_string);
				Module.fromCpp__SendFundsFormSubmission__get_unspent_outs(JS__req_params); // Module must implement this!
			},
			req_params_ss.str().c_str());
	});
	(*controller_ptr).set__get_random_outs_fn([](LightwalletAPI_Req_GetRandomOuts req_params) -> void { // get_random_outs
		boost::property_tree::ptree req_params_root;
		boost::property_tree::ptree amounts_ptree;
		BOOST_FOREACH (const string &amount_string, req_params.amounts)
		{
			property_tree::ptree amount_child;
			amount_child.put("", amount_string);
			amounts_ptree.push_back(std::make_pair("", amount_child));
		}
		req_params_root.add_child("amounts", amounts_ptree);
		req_params_root.put("count", req_params.count);
		stringstream req_params_ss;
		boost::property_tree::write_json(req_params_ss, req_params_root, false /*pretty*/);
		EM_ASM_(
			{
				const JS__req_params_string = Module.UTF8ToString($0);
				const JS__req_params = JSON.parse(JS__req_params_string);
				Module.fromCpp__SendFundsFormSubmission__get_random_outs(JS__req_params); // Module must implement this!
			},
			req_params_ss.str().c_str());
	});
	(*controller_ptr).set__submit_raw_tx_fn([](LightwalletAPI_Req_SubmitRawTx req_params) -> void { // submit_raw_tx
		boost::property_tree::ptree req_params_root;
		boost::property_tree::ptree amounts_ptree;
		req_params_root.put("address", std::move(req_params.address));
		req_params_root.put("view_key", std::move(req_params.view_key));
		req_params_root.put("tx", std::move(req_params.tx));
		req_params_root.put("fee", std::move(req_params.priority));
		stringstream req_params_ss;
		boost::property_tree::write_json(req_params_ss, req_params_root, false /*pretty*/);
		auto req_params_string = req_params_ss.str();
		EM_ASM_(
			{
				const JS__req_params_string = Module.UTF8ToString($0);
				const JS__req_params = JSON.parse(JS__req_params_string);
				Module.fromCpp__SendFundsFormSubmission__submit_raw_tx(JS__req_params); // Module must implement this!
			},
			req_params_ss.str().c_str());
	});
	(*controller_ptr).handle();
}
//
std::string emscr_SendFunds_bridge::register_funds(const string &args_string)
{
	std::cout << "Enter into emscr_register_funds :" << std::endl;

	boost::property_tree::ptree json_root;
	if (!parsed_json_root(args_string, json_root))
	{
		// (it will already have thrown an exception)
		send_app_handler__error_msg(error_ret_json_from_message("Invalid JSON"));
		// return;
	}

	// std::cout << "args_string in send_funds after parsing :" << args_string << std::endl;

	const auto &final_args = json_root.get_child("final_args");
	std::vector<std::string> local_args;
	local_args.reserve(final_args.size());

	for (const auto &arg : final_args)
	{
		local_args.emplace_back(arg.second.get_value<string>());
	}

	if (!local_args.empty())
	{
		local_args.erase(local_args.begin()); // Remove the first element
	}
	else
	{
		std::cout << "Vector is already empty." << std::endl;
	}

	std::cout << "Extracted final_args:" << std::endl;
	for (const auto &x : local_args)
	{
		std::cout << x << std::endl;
	}

	std::string nettype_string = json_root.get<string>("nettype_string");
	std::cout << "nettype_string: " << nettype_string << std::endl;

	// std::cout << "args_string in register_funds as original form :" << args_string << std::endl;

	// std::istringstream iss(args_string);

	// std::vector<std::string> local_args;

	// std::string token;

	// while (iss >> token)
	// {
	// 	local_args.push_back(token);
	// }
	// for (auto x : local_args)
	// {
	// 	std::cout << "args_string in register_funds before parsing :" << x << std::endl;
	// }

	// boost::property_tree::ptree json_root;
	// if (!parsed_json_root(args_string, json_root))
	// {
	// 	// (it will already have thrown an exception)
	// 	send_app_handler__error_msg(error_ret_json_from_message("Invalid JSON"));
	// 	// return;
	// }

	// std::cout << "args_string in register_funds after parsing :" << args_string << std::endl;
	// std::cout << "args_string in register_funds as original form :" << args_string << std::endl;

	// std::istringstream iss(args_string);

	// std::vector<std::string> local_args;

	// std::string token;

	// while (iss >> token)
	// {
	// 	local_args.push_back(token);
	// }
	// for (auto x : local_args)
	// {
	// 	std::cout << "args_string in register_funds before parsing :" << x << std::endl;
	// }

	// boost::property_tree::ptree json_root;
	// if (!parsed_json_root(args_string, json_root))
	// {
	// 	// (it will already have thrown an exception)
	// 	send_app_handler__error_msg(error_ret_json_from_message("Invalid JSON"));
	// 	// return;
	// }

	// std::cout << "args_string in register_funds after parsing :" << args_string << std::endl;

	// for (auto x : local_args)
	// {
	// 	std::cout << "args_string in register_funds After parsing :" << x << std::endl;
	// }
	// std::istringstream iss(args_string);

	// std::vector<std::string> local_args1;

	// std::string token;

	// while (iss >> token)
	// {
	// 	local_args1.push_back(token);
	// }

	// json j = json::parse(local_args1);

	// std::vector<std::string> local_args = j["final_args"];

	// std::cout << "before printing local args :" << std::endl;
	// for (auto x : local_args)
	// {
	// 	std::cout << "local_args value : " << x << std::endl;
	// }

	// Parameters parameters{
	// 	local_args,
	// };
	// for (auto x : local_args)
	// {
	// 	std::cout << "args_string in register_funds After parsing :" << x << std::endl;
	// }
	// std::istringstream iss(args_string);

	// std::vector<std::string> local_args1;

	// std::string token;

	// while (iss >> token)
	// {
	// 	local_args1.push_back(token);
	// }

	// json j = json::parse(local_args1);

	// std::vector<std::string> local_args = j["final_args"];

	// std::cout << "before printing local args :" << std::endl;
	// for (auto x : local_args)
	// {
	// 	std::cout << "local_args value : " << x << std::endl;
	// }

	// Parameters parameters{
	// 	local_args,
	// };

	register_master_node_result result = {};
	result.status = register_master_node_result_status::invalid;

	std::cout << "Before Priority " << std::endl;

	uint32_t priority = 0;
	{
		if (local_args.size() > 0 && parse_priority(local_args[0], priority))
			local_args.erase(local_args.begin());

		if (priority == tx_priority_flash)
		{
			result.status = register_master_node_result_status::no_flash;
			// result.msg += tr("Master node registrations cannot use flash priority");
			// send_app_handler__error_msg(error_ret_json_from_message("Master node registrations cannot use flash priority"));
			// return std::make_pair(result, args_string);
		}

		if (local_args.size() < 6)
		{
			result.status = register_master_node_result_status::insufficient_num_args;
			// result.msg += tr("\nPrepare this command in the daemon with the prepare_registration command");
			// result.msg += tr("\nThis command must be run from the daemon that will be acting as a master node");
			// return std::make_pair(result, args_string);
		}
	}

	std::cout << "After Priority " << std::endl;

	// Here think about the hf-version and related validation check to it.

	//
	// Parse Registration Contributor Args
	//
	std::optional<uint8_t> hf_version = 18;
	if (!hf_version)
	{
		result.status = register_master_node_result_status::network_version_query_failed;
		// result.msg    = ERR_MSG_NETWORK_VERSION_QUERY_FAILED;
		// return std::make_pair(result, args_string);
	}

	std::cout << "After hf_version " << std::endl;

	uint64_t staking_requirement = 0;
	master_nodes::contributor_args_t contributor_args = {};

	std::vector<std::string> args;
	{
		std::cout << "Enter into contributor_args_t " << std::endl;

		// {
		//   if (!is_synced(1))
		//   {
		//     result.status = register_master_node_result_status::wallet_not_synced;
		//     result.msg    = tr("Wallet is not synced. Please synchronise your wallet to the blockchain");
		//     return result;
		//   }
		// }

		staking_requirement = COIN * 10000;
		std::cout << "Staking Requirement: " << staking_requirement << std::endl;

		args = std::vector<std::string>(local_args.begin(), local_args.begin() + local_args.size() - 3); // Assigning values to args vector

		for (const auto &x : args)
		{
			std::cout << "args vector value : " << x << std::endl;
		}
		std::cout << "After declaring args vector " << std::endl;

		// std::cout << "Nettype: " << nettype() << std::endl;

		int networkTypeInt = getNetworkType(nettype_string);
		if (networkTypeInt != -1)
		{
			std::cout << "Integer value for " << nettype_string << " is: " << networkTypeInt << std::endl;
		}
		else
		{
			std::cerr << "Error: Unknown network type" << std::endl;
		}

		if (hf_version.has_value())
		{
			std::cout << "Hard fork version: " << static_cast<int>(*hf_version) << std::endl;
		}
		else
		{
			std::cout << "Hard fork version not available" << std::endl;
		}
		cryptonote::network_type networkType = static_cast<cryptonote::network_type>(networkTypeInt);

		contributor_args = master_nodes::convert_registration_args(networkType, args, staking_requirement, *hf_version);

		if (!contributor_args.success)
		{
			result.status = register_master_node_result_status::convert_registration_args_failed;
			// result.msg = tr("Could not convert registration args, reason: ") + contributor_args.err_msg;
			// return std::make_pair(result, args_string);
		}
		std::cout << "Exiting  from inside contributor_args_t " << std::endl;
	}

	bool successValue = contributor_args.success;
	std::vector<cryptonote::account_public_address> addressesValue = contributor_args.addresses;
	std::cout << "contributor_args.addresses size :" << contributor_args.addresses.size() << std::endl;
	std::vector<uint64_t> portionsValue = contributor_args.portions;
	std::cout << "contributor_args.addresses size :" << contributor_args.portions.size() << std::endl;

	uint64_t portionsForOperatorValue = contributor_args.portions_for_operator;
	std::string errMsgValue = contributor_args.err_msg;

	std::cout << "exiting from outside contributor_args_t " << std::endl;

	for (auto x : portionsValue)
	{
		std::cout << "portionValue =" << x << std::endl;
	}

	std::cout << "portions_for_operator=" << portionsForOperatorValue << std::endl;

	cryptonote::account_public_address address = contributor_args.addresses[0];

	std::cout << "Spend Public Key: " << address.m_spend_public_key << std::endl;
	std::cout << "View Public Key: " << address.m_view_public_key << std::endl;

	// THis is validation check ,we can implement this later
	//  if (!contains_address(address))
	//  {
	//  	result.status = register_master_node_result_status::first_address_must_be_primary_address;
	//  	// result.msg = tr(
	//  	// 	"The first reserved address for this registration does not belong to this wallet.\n"
	//  	// 	"Master node operator must specify an address owned by this wallet for master node registration.");
	//  	return result;
	//  }

	//
	// Parse Registration Metadata Args
	//
	size_t const timestamp_index = local_args.size() - 3;
	size_t const key_index = local_args.size() - 2;
	size_t const signature_index = local_args.size() - 1;
	const std::string &master_node_key_as_str = local_args[key_index];

	crypto::public_key master_node_key;
	crypto::signature signature;
	uint64_t expiration_timestamp = 0;
	{
		try
		{
			expiration_timestamp = boost::lexical_cast<uint64_t>(local_args[timestamp_index]);
			if (expiration_timestamp <= (uint64_t)time(nullptr) + 600 /* 10 minutes */)
			{
				result.status = register_master_node_result_status::registration_timestamp_expired;
				// result.msg = tr("The registration timestamp has expired.");
				// return std::make_pair(result, args_string);
			}
		}
		catch (const std::exception &e)
		{
			result.status = register_master_node_result_status::registration_timestamp_expired;
			// result.msg = tr("The registration timestamp failed to parse: ") + local_args[timestamp_index];
			// return std::make_pair(result, args_string);
		}

		if (!tools::hex_to_type(local_args[key_index], master_node_key))
		{
			result.status = register_master_node_result_status::master_node_key_parse_fail;
			// result.msg = tr("Failed to parse master node pubkey");
			// return std::make_pair(result, args_string);
		}

		if (!tools::hex_to_type(local_args[signature_index], signature))
		{
			result.status = register_master_node_result_status::master_node_signature_parse_fail;
			// result.msg = tr("Failed to parse master node signature");
			// return std::make_pair(result, args_string);
		}
	}

	std::map<std::string, cryptonote::account_public_address> addressNames;

	std::ostringstream oss;
	oss << "timestamp=\"" << local_args[timestamp_index] << "\", "
		<< "master_node_pubkey=\"" << local_args[key_index] << "\", "
		<< "signature=\"" << local_args[signature_index] << "\"";

	// Concatenating addressesValue
	oss << "addresses=[";
	for (size_t i = 0; i < addressesValue.size(); ++i)
	{
		std::string name = "address" + std::to_string(i + 1);
		addressNames[name] = addressesValue[i];
		oss << "(" << name << ": " << addressesValue[i].m_spend_public_key << "," << addressesValue[i].m_view_public_key << ")";
		if (i != addressesValue.size() - 1)
			oss << ",";
	}
	oss << "], ";

	// Concatenating portionsValue
	oss << "portions=[";
	for (size_t i = 0; i < portionsValue.size(); ++i)
	{
		oss << portionsValue[i];
		if (i != portionsValue.size() - 1)
			oss << ",";
	}
	oss << "], ";

	// Concatenating portionsForOperatorValue
	oss << "portions_for_operator=" << portionsForOperatorValue;

	try
	{
		master_nodes::validate_contributor_args(*hf_version, contributor_args);
		master_nodes::validate_contributor_args_signature(contributor_args, expiration_timestamp, master_node_key, signature);
	}
	catch (const master_nodes::invalid_contributions &e)
	{
		result.status = register_master_node_result_status::validate_contributor_args_fail;
		// result.msg = e.what();
		// return result;
	}

	std::vector<uint8_t> extra;
	add_master_node_contributor_to_tx_extra(extra, address);
	add_master_node_pubkey_to_tx_extra(extra, master_node_key);
	if (!add_master_node_register_to_tx_extra(extra, contributor_args.addresses, contributor_args.portions_for_operator, contributor_args.portions, expiration_timestamp, signature))
	{
		result.status = register_master_node_result_status::master_node_register_serialize_to_tx_extra_fail;
		// result.msg = tr("Failed to serialize master node registration tx extra");
		// return result;
	}

	// Check master is able to be registered
	//
	// refresh(false);
	// {
	// 	const auto [success, response] = get_master_nodes({master_node_key_as_str});
	// 	if (!success)
	// 	{
	// 		result.status = register_master_node_result_status::master_node_list_query_failed;
	// 		// result.msg = ERR_MSG_NETWORK_VERSION_QUERY_FAILED;
	// 		// return result;
	// 	}

	// 	if (response.size() >= 1)
	// 	{
	// 		result.status = register_master_node_result_status::master_node_cannot_reregister;
	// 		// result.msg = tr("This master node is already registered");
	// 		// return result;
	// 	}
	// }

	//
	// Create Register Transaction

	{
		uint64_t amount_payable_by_operator = 0;
		{
			const uint64_t DUST = MAX_NUMBER_OF_CONTRIBUTORS;
			uint64_t amount_left = staking_requirement;
			for (size_t i = 0; i < contributor_args.portions.size(); i++)
			{
				uint64_t amount = master_nodes::portions_to_amount(staking_requirement, contributor_args.portions[i]);
				if (i == 0)
					amount_payable_by_operator += amount;
				amount_left -= amount;
			}

			if (amount_left <= DUST)
				amount_payable_by_operator += amount_left;
		}

		std::cout << "Amount payable by operator: " << amount_payable_by_operator << std::endl;

		std::vector<cryptonote::tx_destination_entry> dsts;
		cryptonote::tx_destination_entry de;
		de.addr = address;
		de.is_subaddress = false;
		de.amount = amount_payable_by_operator;
		dsts.push_back(de);

		// for (const auto &entry : dsts)
		// {
		// 	std::cout << "Address: " << entry.addr.to_string() << ", Amount: " << entry.amount << ", Is subaddress: " << entry.is_subaddress << std::endl;
		// }

		oss << "Amount payable by operator:=" << amount_payable_by_operator << std::endl;

		oss << "dsts=[";
		for (size_t i = 0; i < dsts.size(); ++i)
		{
			oss << "("
				<< "addr: " << dsts[i].addr.m_spend_public_key << "," << dsts[i].addr.m_view_public_key << "), "
				<< "is_subaddress: " << (dsts[i].is_subaddress ? "true" : "false") << ", "
				<< "amount: " << dsts[i].amount;
			if (i != dsts.size() - 1)
				oss << ",";
		}
		oss << "]";

		// Convert the ostringstream object to a string
		std::string concatenatedString = oss.str();

		return concatenatedString;

		// try
		// {
		// 	// NOTE(beldex): We know the address should always be a primary address and has no payment id, so we can ignore the subaddress/payment id field here
		// 	cryptonote::address_parse_info dest = {};
		// 	dest.address = address;

		// 	beldex_construct_tx_params tx_params = tools::wallet2::construct_params(*hf_version, txtype::stake, priority);
		// 	std::cout << "Before create_transactions_2 " << std::endl;

		// 	auto ptx_vector = create_transactions_2(dsts, CRYPTONOTE_DEFAULT_TX_MIXIN, 0 /* unlock_time */, priority, extra, subaddr_account, subaddr_indices, tx_params);
		// 	std::cout << "After create_transactions_2 " << std::endl;
		// 	if (ptx_vector.size() == 1)
		// 	{
		// 		result.status = register_master_node_result_status::success;
		// 		result.ptx = ptx_vector[0];
		// 	}
		// 	else
		// 	{
		// 		result.status = register_master_node_result_status::too_many_transactions_constructed;
		// 		// result.msg = ERR_MSG_TOO_MANY_TXS_CONSTRUCTED;
		// 	}
		// }
		// catch (const std::exception &e)
		// {
		// 	result.status = register_master_node_result_status::exception_thrown;
		// 	// result.msg = ERR_MSG_EXCEPTION_THROWN;
		// 	// result.msg += e.what();
		// 	return result;
		// }
	}

	// assert(result.status != register_master_node_result_status::invalid);
	// result.args_string = args_string;
	// return std::make_pair(result, args_string);

	// const operator_fee_k = 'value2'; // Replace with the desired key
	// const operator_fee = myDynamicObject[operator_fee_k];

	// const operator_address_k = 'value3';
	// const operator_address = myDynamicObject[operator_address_k];

	// const operator_amount_k = 'value4';
	// const operator_amount = myDynamicObject[operator_amount_k];

	// const contributor1_address_k = 'value5';
	// const contributor1_address = myDynamicObject[contributor1_address_k];

	// const contributor1_amount_k = 'value6';
	// const contributor1_amount = myDynamicObject[contributor1_amount_k];

	// const contributor2_address_k = 'value7';
	// const contributor2_address = myDynamicObject[contributor2_address_k];

	// const contributor2_amount_k = 'value8';
	// const contributor2_amount = myDynamicObject[contributor2_amount_k];

	// const contributor3_address_k = 'value9';
	// const contributor3_address = myDynamicObject[contributor3_address_k];

	// const contributor3_amount_k = 'value10';
	// const contributor3_amount = myDynamicObject[contributor3_amount_k];

	// const timestamp_k = 'value11';
	// const timestamp = myDynamicObject[timestamp_k];

	// const master_node_pubkey_k = 'value12';
	// const master_node_pubkey_k = myDynamicObject[master_node_pubkey_k];

	// const signature_k = 'value13';
	// const signature = myDynamicObject[signature_k];

	// boost::property_tree::ptree json_root;
	// if (!parsed_json_root(args_string, json_root))
	// {
	// 	// (it will already have thrown an exception)
	// 	send_app_handler__error_msg(error_ret_json_from_message("Invalid JSON"));
	// 	// return  std::make_pair(result, args_string);

	// }

	const auto &destinations = json_root.get_child("destinations");
	vector<string> dest_addrs, dest_amounts;
	dest_addrs.reserve(destinations.size());
	dest_amounts.reserve(destinations.size());

	for (const auto &dest : destinations)
	{
		dest_addrs.emplace_back(dest.second.get<string>("to_address"));
		dest_amounts.emplace_back(dest.second.get<string>("send_amount"));
	}

	Parameters parameters{
		json_root.get<bool>("fromWallet_didFailToInitialize"),
		json_root.get<bool>("fromWallet_didFailToBoot"),
		json_root.get<bool>("fromWallet_needsImport"),
		//
		json_root.get<bool>("requireAuthentication"),
		//
		std::move(dest_amounts),
		json_root.get<bool>("is_sweeping"),
		(uint32_t)stoul(json_root.get<string>("priority")),
		//
		json_root.get<bool>("hasPickedAContact"),
		json_root.get_optional<string>("contact_payment_id"),
		json_root.get_optional<bool>("contact_hasOpenAliasAddress"),
		json_root.get_optional<string>("cached_OAResolved_address"),
		json_root.get_optional<string>("contact_address"),
		//
		nettype_from_string(json_root.get<string>("nettype_string")),
		json_root.get<string>("from_address_string"),
		json_root.get<string>("sec_viewKey_string"),
		json_root.get<string>("sec_spendKey_string"),
		json_root.get<string>("pub_spendKey_string"),
		//
		std::move(dest_addrs),
		//
		json_root.get_optional<string>("resolvedAddress"),
		json_root.get<bool>("resolvedAddress_fieldIsVisible"),
		//
		json_root.get_optional<string>("manuallyEnteredPaymentID"),
		json_root.get<bool>("manuallyEnteredPaymentID_fieldIsVisible"),
		//
		json_root.get_optional<string>("resolvedPaymentID"),
		json_root.get<bool>("resolvedPaymentID_fieldIsVisible"),
		//
		[]( // preSuccess_nonTerminal_validationMessageUpdate_fn
			ProcessStep step) -> void
		{
			send_app_handler__status_update(step);
		},
		[]( // failure_fn
			SendFunds::PreSuccessTerminalCode code,
			boost::optional<string> msg,
			boost::optional<CreateTransactionErrorCode> createTx_errCode,
			boost::optional<uint64_t> spendable_balance,
			boost::optional<uint64_t> required_balance) -> void
		{
			send_app_handler__error_code(code, msg, createTx_errCode, spendable_balance, required_balance);
		},
		[]() -> void { // preSuccess_passedValidation_willBeginSending
			EM_ASM_(
				{
					Module.fromCpp__SendFundsFormSubmission__willBeginSending({}); // Module must implement this!
				});
		},
		//
		[]() -> void { // canceled_fn
			EM_ASM_(
				{
					Module.fromCpp__SendFundsFormSubmission__canceled({}); // Module must implement this!
				});
			THROW_WALLET_EXCEPTION_IF(controller_ptr == NULL, error::wallet_internal_error, "expected non-NULL controller_ptr");
			delete controller_ptr; // having finished
			controller_ptr = NULL;
		},
		[](SendFunds::Success_RetVals retVals) -> void // success_fn
		{
			send_app_handler__success(retVals);
		}};
	controller_ptr = new FormSubmissionController{parameters}; // heap alloc
	if (!controller_ptr)
	{ // exception will be thrown if oom but JIC, since null ptrs are somehow legal in WASM
		send_app_handler__error_msg("Out of memory (heap vals container)");
		// return std::make_pair(result, args_string);
	}
	(*controller_ptr).set__authenticate_fn([]() -> void { // authenticate_fn - this is not guaranteed to be called but it will be if requireAuthentication is true
		EM_ASM_(
			{
				Module.fromCpp__SendFundsFormSubmission__authenticate(); // Module must implement this!
			});
	});
	(*controller_ptr).set__get_unspent_outs_fn([](LightwalletAPI_Req_GetUnspentOuts req_params) -> void { // get_unspent_outs
		boost::property_tree::ptree req_params_root;
		req_params_root.put("address", req_params.address);
		req_params_root.put("view_key", req_params.view_key);
		req_params_root.put("amount", req_params.amount);
		req_params_root.put("dust_threshold", req_params.dust_threshold);
		req_params_root.put("use_dust", req_params.use_dust);
		req_params_root.put("mixin", req_params.mixin);
		stringstream req_params_ss;
		boost::property_tree::write_json(req_params_ss, req_params_root, false /*pretty*/);
		EM_ASM_(
			{
				const JS__req_params_string = Module.UTF8ToString($0);
				const JS__req_params = JSON.parse(JS__req_params_string);
				Module.fromCpp__SendFundsFormSubmission__get_unspent_outs(JS__req_params); // Module must implement this!
			},
			req_params_ss.str().c_str());
	});
	(*controller_ptr).set__get_random_outs_fn([](LightwalletAPI_Req_GetRandomOuts req_params) -> void { // get_random_outs
		boost::property_tree::ptree req_params_root;
		boost::property_tree::ptree amounts_ptree;
		BOOST_FOREACH (const string &amount_string, req_params.amounts)
		{
			property_tree::ptree amount_child;
			amount_child.put("", amount_string);
			amounts_ptree.push_back(std::make_pair("", amount_child));
		}
		req_params_root.add_child("amounts", amounts_ptree);
		req_params_root.put("count", req_params.count);
		stringstream req_params_ss;
		boost::property_tree::write_json(req_params_ss, req_params_root, false /*pretty*/);
		EM_ASM_(
			{
				const JS__req_params_string = Module.UTF8ToString($0);
				const JS__req_params = JSON.parse(JS__req_params_string);
				Module.fromCpp__SendFundsFormSubmission__get_random_outs(JS__req_params); // Module must implement this!
			},
			req_params_ss.str().c_str());
	});
	(*controller_ptr).set__submit_raw_tx_fn([](LightwalletAPI_Req_SubmitRawTx req_params) -> void { // submit_raw_tx
		boost::property_tree::ptree req_params_root;
		boost::property_tree::ptree amounts_ptree;
		req_params_root.put("address", std::move(req_params.address));
		req_params_root.put("view_key", std::move(req_params.view_key));
		req_params_root.put("tx", std::move(req_params.tx));
		req_params_root.put("fee", std::move(req_params.priority));
		stringstream req_params_ss;
		boost::property_tree::write_json(req_params_ss, req_params_root, false /*pretty*/);
		auto req_params_string = req_params_ss.str();
		EM_ASM_(
			{
				const JS__req_params_string = Module.UTF8ToString($0);
				const JS__req_params = JSON.parse(JS__req_params_string);
				Module.fromCpp__SendFundsFormSubmission__submit_raw_tx(JS__req_params); // Module must implement this!
			},
			req_params_ss.str().c_str());
	});
	(*controller_ptr).handle();

	std::cout << "exiting from register_funds" << std::endl;
}
//
void emscr_SendFunds_bridge::send_cb__authentication(const string &args_string)
{
	boost::property_tree::ptree json_root;
	if (!parsed_json_root(args_string, json_root))
	{
		// (it will already have thrown an exception)
		send_app_handler__error_msg(error_ret_json_from_message("Invalid JSON"));
		return;
	}
	auto did_pass = json_root.get<bool>("did_pass");
	if (!controller_ptr)
	{ // an error will have been returned already - just bail.
		return;
	}
	(*controller_ptr).cb__authentication(did_pass);
}
void emscr_SendFunds_bridge::send_cb_I__got_unspent_outs(const string &args_string)
{
	boost::property_tree::ptree json_root;
	if (!parsed_json_root(args_string, json_root))
	{
		// (it will already have thrown an exception)
		send_app_handler__error_msg(error_ret_json_from_message("Invalid JSON"));
		return;
	}
	auto optl__err_msg = json_root.get_optional<string>("err_msg");
	if (optl__err_msg != boost::none && (*optl__err_msg).size() > 0)
	{ // if args_string actually contains a server error, call error fn with it - this must be done so that the heap alloc'd vals container can be freed
		stringstream err_msg_ss;
		err_msg_ss << "An error occurred while getting your latest balance: " << *(optl__err_msg);
		send_app_handler__error_msg(err_msg_ss.str());
		return;
	}
	if (!controller_ptr)
	{ // an error will have been returned already - just bail.
		return;
	}
	(*controller_ptr).cb_I__got_unspent_outs(optl__err_msg, json_root.get_child("res"));
}
void emscr_SendFunds_bridge::send_cb_II__got_random_outs(const string &args_string)
{
	boost::property_tree::ptree json_root;
	if (!parsed_json_root(args_string, json_root))
	{
		// (it will already have thrown an exception)
		send_app_handler__error_msg(error_ret_json_from_message("Invalid JSON"));
		return;
	}
	auto optl__err_msg = json_root.get_optional<string>("err_msg");
	if (optl__err_msg != boost::none && (*optl__err_msg).size() > 0)
	{ // if args_string actually contains a server error, call error fn with it - this must be done so that the heap alloc'd vals container can be freed
		stringstream err_msg_ss;
		err_msg_ss << "An error occurred while getting decoy outputs: " << *(optl__err_msg);
		send_app_handler__error_msg(err_msg_ss.str());
		return;
	}
	if (!controller_ptr)
	{ // an error will have been returned already - just bail.
		return;
	}
	(*controller_ptr).cb_II__got_random_outs(optl__err_msg, json_root.get_child("res"));
}
void emscr_SendFunds_bridge::send_cb_III__submitted_tx(const string &args_string)
{
	boost::property_tree::ptree json_root;
	if (!parsed_json_root(args_string, json_root))
	{
		// (it will already have thrown an exception)
		send_app_handler__error_msg(error_ret_json_from_message("Invalid JSON"));
		return;
	}
	auto optl__err_msg = json_root.get_optional<string>("err_msg");
	if (optl__err_msg != boost::none && (*optl__err_msg).size() > 0)
	{ // if args_string actually contains a server error, call error fn with it - this must be done so that the heap alloc'd vals container can be freed
		stringstream err_msg_ss;
		err_msg_ss << "An error occurred while submitting transaction: " << *(optl__err_msg);
		send_app_handler__error_msg(err_msg_ss.str());
		return;
	}
	if (!controller_ptr)
	{ // an error will have been returned already - just bail.
		return;
	}
	(*controller_ptr).cb_III__submitted_tx(optl__err_msg);
}
