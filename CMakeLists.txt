cmake_minimum_required(VERSION 3.4.1)

add_compile_definitions(BELDEX_CORE_CUSTOM)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

project(BeldexLibAppCpp)


#



include_directories("src")
#
set(BELDEX_LIBAPP_CPP "src/submodules/beldex-libapp-cpp")
set(BELDEX_LIBAPP_CPP_SRC "${BELDEX_LIBAPP_CPP}/src")
include_directories("${BELDEX_LIBAPP_CPP_SRC}")
include_directories("${BELDEX_LIBAPP_CPP_SRC}/SendFunds/Controllers")
#
set(BELDEX_CORE_CPP "${BELDEX_LIBAPP_CPP_SRC}/beldex-core-cpp")
set(BELDEX_CORE_CPP_SRC "${BELDEX_CORE_CPP}/src")
include_directories("${BELDEX_CORE_CPP_SRC}")
#
set(BELDEX_SRC "${BELDEX_CORE_CPP}/contrib/beldex-core-custom")
include_directories(${BELDEX_SRC})
include_directories("${BELDEX_SRC}/external")
include_directories("${BELDEX_SRC}/external/loki-mq")
include_directories("${BELDEX_SRC}/epee/include")
include_directories("${BELDEX_SRC}/common")
include_directories("${BELDEX_SRC}/vtlogger")
include_directories("${BELDEX_SRC}/crypto")
include_directories("${BELDEX_SRC}/cryptonote_basic")
include_directories("${BELDEX_SRC}/multisig")
include_directories("${BELDEX_SRC}/cryptonote_core")
include_directories("${BELDEX_SRC}/cryptonote_protocol")
include_directories("${BELDEX_SRC}/wallet")
include_directories("${BELDEX_SRC}/rpc")
include_directories("${BELDEX_SRC}/mnemonics")
include_directories("${BELDEX_SRC}/contrib/libsodium/include") # support sodium/… paths
include_directories("${BELDEX_SRC}/contrib/libsodium/include/sodium")

set(
    SRC_FILES
    #
    src/index.cpp
    #
    src/emscr_SendFunds_bridge.hpp
    src/emscr_SendFunds_bridge.cpp
    src/walletf2.h
    src/walletf2.cpp
    

    #
    ${BELDEX_CORE_CPP_SRC}/beldex_address_utils.hpp
    ${BELDEX_CORE_CPP_SRC}/beldex_address_utils.cpp
    ${BELDEX_CORE_CPP_SRC}/beldex_paymentID_utils.hpp
    ${BELDEX_CORE_CPP_SRC}/beldex_paymentID_utils.cpp
    ${BELDEX_CORE_CPP_SRC}/beldex_key_image_utils.hpp
    ${BELDEX_CORE_CPP_SRC}/beldex_key_image_utils.cpp
    ${BELDEX_CORE_CPP_SRC}/beldex_fee_utils.hpp
    ${BELDEX_CORE_CPP_SRC}/beldex_fee_utils.cpp
    ${BELDEX_CORE_CPP_SRC}/beldex_transfer_utils.hpp
    ${BELDEX_CORE_CPP_SRC}/beldex_transfer_utils.cpp
    ${BELDEX_CORE_CPP_SRC}/beldex_fork_rules.hpp
    ${BELDEX_CORE_CPP_SRC}/beldex_fork_rules.cpp
    ${BELDEX_CORE_CPP_SRC}/beldex_wallet_utils.hpp
    ${BELDEX_CORE_CPP_SRC}/beldex_wallet_utils.cpp
    ${BELDEX_CORE_CPP_SRC}/serial_bridge_index.hpp
    ${BELDEX_CORE_CPP_SRC}/serial_bridge_index.cpp
    ${BELDEX_CORE_CPP_SRC}/beldex_send_routine.hpp
    ${BELDEX_CORE_CPP_SRC}/beldex_send_routine.cpp
    ${BELDEX_CORE_CPP_SRC}/serial_bridge_utils.hpp
    ${BELDEX_CORE_CPP_SRC}/serial_bridge_utils.cpp
    ${BELDEX_CORE_CPP_SRC}/tools__ret_vals.hpp
    ${BELDEX_CORE_CPP_SRC}/tools__ret_vals.cpp
    #
    ${BELDEX_LIBAPP_CPP_SRC}/SendFunds/Controllers/SendFundsFormSubmissionController.hpp
    ${BELDEX_LIBAPP_CPP_SRC}/SendFunds/Controllers/SendFundsFormSubmissionController.cpp
    #
    ${BELDEX_SRC}/cryptonote_basic/account.cpp
    ${BELDEX_SRC}/cryptonote_basic/cryptonote_basic.h
    ${BELDEX_SRC}/cryptonote_basic/cryptonote_basic_impl.h
    ${BELDEX_SRC}/cryptonote_basic/cryptonote_basic_impl.cpp
    ${BELDEX_SRC}/cryptonote_basic/cryptonote_format_utils.h
    ${BELDEX_SRC}/cryptonote_basic/cryptonote_format_utils.cpp
    ${BELDEX_SRC}/cryptonote_basic/cryptonote_basic.cpp
    ${BELDEX_SRC}/crypto/crypto.cpp
    ${BELDEX_SRC}/crypto/hash.c
    ${BELDEX_SRC}/crypto/cn_heavy_hash_hard_arm.cpp
    ${BELDEX_SRC}/crypto/cn_heavy_hash_hard_intel.cpp
    ${BELDEX_SRC}/crypto/cn_heavy_hash_soft.cpp
    ${BELDEX_SRC}/crypto/cn_turtle_hash.c
    ${BELDEX_SRC}/crypto/slow-hash-dummied.cpp
    ${BELDEX_SRC}/crypto/oaes_lib.c
    ${BELDEX_SRC}/crypto/crypto-ops.c
    ${BELDEX_SRC}/crypto/crypto-ops-data.c
    ${BELDEX_SRC}/crypto/keccak.c
    ${BELDEX_SRC}/crypto/chacha.c
    ${BELDEX_SRC}/crypto/random.c
    ${BELDEX_SRC}/crypto/aesb.c
    ${BELDEX_SRC}/crypto/tree-hash.c
    ${BELDEX_SRC}/crypto/hash-extra-blake.c
    ${BELDEX_SRC}/crypto/blake256.c
    ${BELDEX_SRC}/crypto/hash-extra-groestl.c
    ${BELDEX_SRC}/crypto/hash-extra-jh.c
    ${BELDEX_SRC}/crypto/hash-extra-skein.c
    ${BELDEX_SRC}/crypto/groestl.c
    ${BELDEX_SRC}/crypto/jh.c
    ${BELDEX_SRC}/crypto/skein.c
    ${BELDEX_SRC}/cryptonote_core/cryptonote_tx_utils.cpp
    ${BELDEX_SRC}/cryptonote_core/master_node_list.cpp
    ${BELDEX_SRC}/cryptonote_core/master_node_rules.cpp
    ${BELDEX_SRC}/common/base58.cpp
    ${BELDEX_SRC}/common/threadpool.cpp
    ${BELDEX_SRC}/common/aligned.c
    ${BELDEX_SRC}/common/util.cpp
    ${BELDEX_SRC}/common/i18n.cpp
    ${BELDEX_SRC}/common/string_util.cpp
    ${BELDEX_SRC}/epee/src/hex.cpp
    ${BELDEX_SRC}/epee/src/string_tools.cpp
    ${BELDEX_SRC}/epee/src/portable_storage.cpp
    ${BELDEX_SRC}/epee/src/memwipe.c
    ${BELDEX_SRC}/epee/src/mlocker.cpp
    ${BELDEX_SRC}/epee/src/wipeable_string.cpp
    ${BELDEX_SRC}/device/device.cpp
    ${BELDEX_SRC}/device/device_default.cpp
    ${BELDEX_SRC}/ringct/rctOps.cpp
    ${BELDEX_SRC}/ringct/rctTypes.cpp
    ${BELDEX_SRC}/ringct/rctCryptoOps.c
    ${BELDEX_SRC}/ringct/rctSigs.cpp
    ${BELDEX_SRC}/ringct/bulletproofs.cc
    ${BELDEX_SRC}/ringct/multiexp.cc
    ${BELDEX_SRC}/mnemonics/electrum-words.cpp
    ${BELDEX_SRC}/vtlogger/logger.cpp
    ${BELDEX_SRC}/contrib/libsodium/src/crypto_verify/verify.c
)
#
#


set (
  EMCC_LINKER_FLAGS__WASM
"-Wall \
-gsource-map \
-std=c++11 \
-flto \
--bind \
-s STRICT=0 \
-s MODULARIZE=1 \
-s 'EXPORT_NAME=\"BeldexLibAppCpp\"' \
-s WASM=1 \
-s ASSERTIONS=2 \
-s DEMANGLE_SUPPORT=1 \
-s ALLOW_MEMORY_GROWTH=1 \
-s NO_DISABLE_EXCEPTION_CATCHING \
-s NODEJS_CATCH_EXIT=1 \
-s NODEJS_CATCH_REJECTION=0 \
-s ERROR_ON_UNDEFINED_SYMBOLS=1 \
-s LLD_REPORT_UNDEFINED \
-s RESERVED_FUNCTION_POINTERS=5 \
-O3 \
-s EXPORTED_RUNTIME_METHODS='[\"UTF8ToString\",\"stringToUTF8\",\"lengthBytesUTF8\",\"intArrayToString\",\"getTempRet0\",\"addFunction\"]' \
--source-map-base ${CMAKE_CURRENT_LIST_DIR}/sourcemap \
--memory-init-file 1 \
--post-js ${CMAKE_CURRENT_LIST_DIR}/src/module-post.js \
")




# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")


message(STATUS "EMCC_LINKER_FLAGS__WASM ${EMCC_LINKER_FLAGS__WASM}")
#
add_executable(BeldexLibAppCpp_WASM ${SRC_FILES})



#
set_target_properties(BeldexLibAppCpp_WASM PROPERTIES COMPILE_FLAGS "-s USE_BOOST_HEADERS=1" LINK_FLAGS "${EMCC_LINKER_FLAGS__WASM}")