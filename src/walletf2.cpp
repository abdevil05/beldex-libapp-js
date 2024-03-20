// #pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <iostream>

// Include CryptoNote headers
#include "cryptonote_core/cryptonote_tx_utils.h"
#include "cryptonote_basic/cryptonote_basic_impl.h"

// Include Crypto libraries headers
#include "crypto/crypto.h"
#include "crypto/hash.h"
#include "pending_tx.h"
#include "tx_construction_data.h"
// #include "walletf2.h"


constexpr std::array<const char *const, 6> allowed_priority_strings = {{"default", "unimportant", "normal", "elevated", "priority", "flash"}};


bool parse_priority(const std::string &arg, uint32_t &priority)
{
  auto priority_pos = std::find(
      allowed_priority_strings.begin(),
      allowed_priority_strings.end(),
      arg);
  if (priority_pos != allowed_priority_strings.end())
  {
    priority = std::distance(allowed_priority_strings.begin(), priority_pos);
    return true;
  }
  return false;
}

// const char *i18n_translate(const char *s, const std::string &context)
// {
//   const std::string key = context + "\0"s + s;
//   std::map<std::string, std::string>::const_iterator i = i18n_entries.find(key);
//   if (i == i18n_entries.end())
//     return s;
//   return (*i).second.c_str();
// }

	int getNetworkType(const std::string &netTypeStr)
	{
		static const std::unordered_map<std::string, cryptonote::network_type> networkTypeMap = {
			{"MAINNET", cryptonote::MAINNET},
			{"TESTNET", cryptonote::TESTNET},
			{"DEVNET", cryptonote::DEVNET},
			{"FAKECHAIN", cryptonote::FAKECHAIN},
			{"UNDEFINED", cryptonote::UNDEFINED}};

		auto it = networkTypeMap.find(netTypeStr);
		if (it != networkTypeMap.end())
		{
			return static_cast<int>(it->second);
		}
		else
		{
			return -1; 
		}
	}