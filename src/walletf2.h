#pragma once

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

// constexpr std::array<const char *const, 6> allowed_priority_strings = {{"default", "unimportant", "normal", "elevated", "priority", "flash"}};

// static const char *tr(const char *str) { return i18n_translate(str, "emscr_SendFunds_bridge::register_funds"); }
// static std::map<std::string, std::string> i18n_entries;
using pending_tx = wallet::pending_tx;
bool parse_priority(const std::string &arg, uint32_t &priority);
int getNetworkType(const std::string& netTypeStr);
inline std::map<std::string, std::string> i18n_entries;

enum struct register_master_node_result_status
{
  invalid,
  success,
  insufficient_num_args,
  subaddr_indices_parse_fail,
  network_height_query_failed,
  network_version_query_failed,
  convert_registration_args_failed,
  registration_timestamp_expired,
  registration_timestamp_parse_fail,
  validate_contributor_args_fail,
  master_node_key_parse_fail,
  master_node_signature_parse_fail,
  master_node_register_serialize_to_tx_extra_fail,
  first_address_must_be_primary_address,
  master_node_list_query_failed,
  master_node_cannot_reregister,
  insufficient_portions,
  wallet_not_synced,
  too_many_transactions_constructed,
  exception_thrown,
  no_flash,
};

struct register_master_node_result
{
  register_master_node_result_status status;
  std::string msg;
  pending_tx ptx;
  // std::string args_string;
};

// bool parse_priority(const std::string &arg, uint32_t &priority)
// {
//   auto priority_pos = std::find(
//       allowed_priority_strings.begin(),
//       allowed_priority_strings.end(),
//       arg);
//   if (priority_pos != allowed_priority_strings.end())
//   {
//     priority = std::distance(allowed_priority_strings.begin(), priority_pos);
//     return true;
//   }
//   return false;
// }

enum tx_priority
{
  tx_priority_default = 0,
  tx_priority_unimportant = 1,
  tx_priority_normal = 2,
  tx_priority_elevated = 3,
  tx_priority_priority = 4,
  tx_priority_flash = 5,
  tx_priority_last
};

inline const char *i18n_translate(const char *s, const std::string &context)
{
  const std::string key = context + "\0"s + s;
  std::map<std::string, std::string>::const_iterator i = i18n_entries.find(key);
  if (i == i18n_entries.end())
    return s;
  return (*i).second.c_str();
}

// class Cryptonote {
// private:
//     cryptonote::network_type m_nettype;
//     std::map<std::string, cryptonote::network_type> nettype_map = {
//         {"MAINNET", cryptonote::MAINNET},
//         {"TESTNET", cryptonote::TESTNET},
//         {"DEVNET", cryptonote::DEVNET},
//         {"FAKECHAIN", cryptonote::FAKECHAIN},
//         {"UNDEFINED", cryptonote::UNDEFINED}
//     };

// public:
//     Cryptonote() {
//         // Default constructor
//     }

//     void setNettype(const std::string& nettype_string) {
//         auto it = nettype_map.find(nettype_string);
//         if (it != nettype_map.end()) {
//             m_nettype = it->second;
//         } else {
//             m_nettype = cryptonote::UNDEFINED; // Default value for undefined strings
//         }
//     }

//     inline cryptonote::network_type nettype() { 
//         return m_nettype; 
//     }
// };

// Cryptonote cn;

// cn.setNettype(nettype_string);
// int net_value = static_cast<int>(cn.nettype());



// inline cryptonote::network_type m_nettype;
// inline cryptonote::network_type nettype() { return m_nettype; }


struct tx_construction_data
{
  std::vector<cryptonote::tx_source_entry> sources;
  cryptonote::tx_destination_entry change_dts;
  std::vector<cryptonote::tx_destination_entry> splitted_dsts; // split, includes change
  std::vector<size_t> selected_transfers;
  std::vector<uint8_t> extra;
  uint64_t unlock_time;
  rct::RCTConfig rct_config;
  std::vector<cryptonote::tx_destination_entry> dests; // original setup, does not include change
  uint32_t subaddr_account;                            // subaddress account of your wallet to be used in this transfer
  std::set<uint32_t> subaddr_indices;                  // set of address indices used as inputs in this transfer

  uint8_t hf_version;
  cryptonote::txtype tx_type;
};
