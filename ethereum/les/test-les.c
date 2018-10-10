//
//  test-les.c
//  breadwallet-core Ethereum
//
//  Created by Lamont Samuels on 4/16/18.
//  Copyright (c) 2018 breadwallet LLC
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <regex.h>
#include "BRInt.h"
#include "BRCrypto.h"
#include "../base/BREthereumHash.h"
#include "../blockchain/BREthereumNetwork.h"
#include "BREthereumLES.h"
#include "BREthereumNode.h"

#include "BREthereum.h"
#include "../ewm/BREthereumEWM.h"
#include "BREthereumLESRandom.h"
#include "test-les.h"

#define TST_LOG_TOPIC    "TST"

#define DEFAULT_UDPPORT     (30303)
#define DEFAULT_TCPPORT     (30303)

#define LES_LOCAL_ENDPOINT_ADDRESS    "1.1.1.1"
#define LES_LOCAL_ENDPOINT_TCP_PORT   DEFAULT_TCPPORT
#define LES_LOCAL_ENDPOINT_UDP_PORT   DEFAULT_UDPPORT
#define LES_LOCAL_ENDPOINT_NAME       "BRD Light Client"

/// MARK: Node Test

static void
_nodeCallbackProvide (BREthereumNodeContext context,
                      BREthereumNode node,
                      BREthereumProvisionResult result) {
}

static void
_nodeCallbackMessage (BREthereumNodeContext context,
                      BREthereumNode node,
                      BREthereumLESMessage message) {
}

static void
_nodeCallbackStatus (BREthereumNodeContext context,
                     BREthereumNode node,
                     BREthereumNodeEndpointRoute route,
                     BREthereumNodeState state) {
}

// pthread locks/conditions and wait and signal functions
static pthread_mutex_t _testLock;
static pthread_cond_t _testCond;
static int _testComplete;
static int _numOfTests;

static void _initTestState() {
    
    // Initializes the pthread variables
    pthread_mutex_init(&_testLock, NULL);
    pthread_cond_init(&_testCond, NULL);
    
    pthread_mutex_lock(&_testLock);
    _testComplete = 0;
    pthread_mutex_unlock(&_testLock);
}

static void _initTest(int numOfTests) {
    pthread_mutex_lock(&_testLock);
    _testComplete = 0;
    _numOfTests = numOfTests;
    pthread_mutex_unlock(&_testLock);
}

static void _waitForTests() {
    struct timespec wait = { 30, 0 };
    //Wait for a little bit to get a reply back from the server.
    pthread_mutex_lock(&_testLock);
    //Wait until all the tests are complete
    while(_testComplete < _numOfTests){
        pthread_cond_timedwait_relative_np (&_testCond, &_testLock, &wait);
    }
    pthread_mutex_unlock(&_testLock);
}

static void _signalTestComplete() {
    
    //Signal to the testing thread that this test is complete
    pthread_mutex_lock(&_testLock);
    _testComplete++;
    pthread_mutex_unlock(&_testLock);
    
    pthread_cond_signal(&_testCond);
}

// LES Tests

static void
_announceCallback (BREthereumLESCallbackContext context,
                   BREthereumHash headHash,
                   uint64_t headNumber,
                   UInt256 headTotalDifficulty,
                   uint64_t reorgDepth) {
    eth_log(TST_LOG_TOPIC, "Block: %llu", headNumber);
}

static void
_statusCallback (BREthereumLESCallbackContext context,
                 BREthereumHash headHash,
                 uint64_t headNumber) {
    eth_log(TST_LOG_TOPIC, "Status: %llu", headNumber);
}

static void
_saveNodesCallback (BREthereumLESCallbackContext context,
                    BRArrayOf(BREthereumNodeConfig) nodes) {
    eth_log (TST_LOG_TOPIC, "SaveNode: %zu", array_count(nodes));
    array_free(nodes);
}

//
//  Testing SendTx and SendTxV2 message
//
#if 0
#define GAS_PRICE_20_GWEI       2000000000
#define GAS_PRICE_10_GWEI       1000000000
#define GAS_PRICE_5_GWEI         500000000
#define GAS_LIMIT_DEFAULT 21000

static void _transactionStatus(BREthereumLESTransactionStatusContext context,
                               BREthereumHash transaction,
                               BREthereumTransactionStatus status){
    
    printf("RECEIVED AN TRANSACTION STATUS");
}
void prepareLESTransaction (BREthereumLES les, const char *paperKey, const char *recvAddr, const uint64_t gasPrice, const uint64_t gasLimit, const uint64_t amount) {
    printf ("     Prepare Transaction\n");
    
    BREthereumClient client = {};
    BREthereumEWM ewm = ethereumCreate(ethereumMainnet, paperKey, EWM_USE_LES, SYNC_MODE_FULL_BLOCKCHAIN, client, NULL, NULL, NULL, NULL);
    // A wallet amount Ether
    BREthereumWalletId wallet = ethereumGetWallet(ewm);
    // END - One Time Code Block
    
    // Optional - will provide listNodeWalletCreateTransactionDetailed.
    ethereumWalletSetDefaultGasPrice(ewm, wallet, WEI, gasPrice);
    ethereumWalletSetDefaultGasLimit(ewm, wallet, gasLimit);
    
    BREthereumAmount amountAmountInEther =
    ethereumCreateEtherAmountUnit(ewm, amount, WEI);
    
    BREthereumTransferId tx1 =
    ethereumWalletCreateTransfer
    (ewm,
     wallet,
     recvAddr,
     amountAmountInEther);
    
    ethereumWalletSignTransfer (ewm, wallet, tx1, paperKey);
    
    const char *rawTransactionHexEncoded =
    ethereumTransferGetRawDataHexEncoded(ewm, wallet, tx1, "0x");
    
    printf ("        Raw Transaction: %s\n", rawTransactionHexEncoded);
    
    char *fromAddr = ethereumGetAccountPrimaryAddress(ewm);
    BREthereumTransferId *tids = ethereumWalletGetTransfers(ewm, wallet);
    
    assert (NULL != tids && -1 != tids[0]);
    
    BREthereumTransferId tid = tids[0];
    assert (0 == strcmp (fromAddr, ethereumTransferGetSendAddress(ewm, tid)) &&
            0 == strcmp (recvAddr, ethereumTransferGetRecvAddress(ewm, tid)));
    
    BREthereumTransfer actualTransfer = ewmLookupTransfer(ewm, tid);
    BREthereumTransaction actualTransaction = transferGetOriginatingTransaction(actualTransfer);
    
    //Initilize testing state
    _initTest(1);
    
    lesSubmitTransaction(les, NULL, _transactionStatus, actualTransaction);
    
    sleep(600);
    
    free (fromAddr);
    ethereumDestroy(ewm);
}

extern void
reallySendLESTransaction(BREthereumLES les) {
    
    char paperKey[] = "biology just ridge kidney random donkey master employ poverty remind panel twenty";
    char recvAddress[] = "0x49f4C50d9BcC7AfdbCF77e0d6e364C29D5a660DF";
    /*
     fputs("PaperKey: ", stdout);
     fgets (paperKey, 1024, stdin);
     paperKey[strlen(paperKey) - 1] = '\0';
     
     fputs("Address: ", stdout);
     fgets (recvAddress, 1024, stdin);
     recvAddress[strlen(recvAddress) - 1] = '\0';
     */
    printf ("PaperKey: '%s'\nAddress: '%s'\n", paperKey, recvAddress);
    
    // 0.001/2 ETH
    prepareLESTransaction(les, paperKey, recvAddress, GAS_PRICE_5_GWEI, GAS_LIMIT_DEFAULT, 1000000000000000000 / 1000 / 2);
}
#endif

//
//  Testing BlockHeaders message
//
static int _GetBlockHeaders_Context1 = 0;
static int _GetBlockHeaders_Context2 = 0;
static int _GetBlockHeaders_Context3 = 0;
static int _GetBlockHeaders_Context4 = 0;
static int _GetBlockHeaders_Context5 = 0;

typedef struct {
    BREthereumHash hash;
    uint64_t blockNum;
    UInt256 difficulty;
    uint64_t gasUsed;
    BREthereumHash parent;
    size_t transactionCount;
    size_t ommersCount;
}BlockHeaderTestData;
static BlockHeaderTestData _blockHeaderTestData[5];
#define BLOCK_4732522_IDX 2

static void _initBlockHeaderTestData(void){
    
    //Block Number: 4732524
    _blockHeaderTestData[0].hash = hashCreate("0x3a510c07862ebce419a14bfcd95620f924d188a935654c5ad0f4d5d7ee429193");
    _blockHeaderTestData[0].blockNum = 4732524;
    _blockHeaderTestData[0].difficulty = createUInt256(1645417372907632);
    _blockHeaderTestData[0].gasUsed = 7996865;
    _blockHeaderTestData[0].parent = hashCreate("0x5463afdad9eb343096a6a6561d4fed4b478380d02721cdd8fab97fda058f9fa2");
    _blockHeaderTestData[0].transactionCount = 331;
    _blockHeaderTestData[0].ommersCount = 0;
    
    //Block Number: 4732523
    _blockHeaderTestData[1].hash = hashCreate("0x5463afdad9eb343096a6a6561d4fed4b478380d02721cdd8fab97fda058f9fa2");
    _blockHeaderTestData[1].blockNum = 4732523;
    _blockHeaderTestData[1].difficulty = createUInt256(1645417372874864);
    _blockHeaderTestData[1].gasUsed = 7998505;
    _blockHeaderTestData[1].parent = hashCreate("0xb812a7b4a96c87a3d7d572847b3dee352b395cc9cfe3b6f0d163bc54e7d8a78e");
    _blockHeaderTestData[1].transactionCount = 193;
    _blockHeaderTestData[1].ommersCount = 0;
    
    //Block Number: 4732522
    _blockHeaderTestData[2].hash = hashCreate("0xb812a7b4a96c87a3d7d572847b3dee352b395cc9cfe3b6f0d163bc54e7d8a78e");
    _blockHeaderTestData[2].blockNum = 4732522;
    _blockHeaderTestData[2].difficulty = createUInt256(1646221191783396);
    _blockHeaderTestData[2].gasUsed = 8003540;
    _blockHeaderTestData[2].parent = hashCreate("0x4b29fb30276713be22786a9bdd548d787e9a2ea10248669f189b3f57f86ebaf8");
    _blockHeaderTestData[2].transactionCount = 186;
    _blockHeaderTestData[2].ommersCount = 0;
    
    
    //Block Number: 4732521
    _blockHeaderTestData[3].hash = hashCreate("0x4b29fb30276713be22786a9bdd548d787e9a2ea10248669f189b3f57f86ebaf8");
    _blockHeaderTestData[3].blockNum = 4732521;
    _blockHeaderTestData[3].difficulty = createUInt256(1647025403373368);
    _blockHeaderTestData[3].gasUsed = 7996801;
    _blockHeaderTestData[3].parent = hashCreate("0x4abb508954ec5f827184fb0d8bc74b104094d4060a06cc2dd743e4bfeaf1d8af");
    _blockHeaderTestData[3].transactionCount = 316;
    _blockHeaderTestData[3].ommersCount = 0;
    
    
    //Block Number: 4732520
    _blockHeaderTestData[4].hash = hashCreate("0x4abb508954ec5f827184fb0d8bc74b104094d4060a06cc2dd743e4bfeaf1d8af");
    _blockHeaderTestData[4].blockNum = 4732520;
    _blockHeaderTestData[4].difficulty = createUInt256(1647830007836613);
    _blockHeaderTestData[4].gasUsed = 7986707;
    _blockHeaderTestData[4].parent = hashCreate("0xe8f5d7cd81ad8ae3a677f6df6d87438ee5c98ead11f8df1b90b788f059a7deab");
    _blockHeaderTestData[4].transactionCount = 169;
    _blockHeaderTestData[4].ommersCount = 0;
}

static BREthereumBoolean _checkBlockHeader(BREthereumBlockHeader header,
                                           BREthereumHash expectedHash,
                                           uint64_t expectedBlockNumber,
                                           UInt256 expectedDifficulty,
                                           uint64_t expectedGasUsed,
                                           BREthereumHash expectedParenthash) {
    
    BREthereumHash gotHash = blockHeaderGetHash(header);
    uint64_t gotBlockNumber = blockHeaderGetNumber(header);
    uint64_t gotGasUsed = blockHeaderGetGasUsed(header);
    BREthereumHash gotParentHash = blockHeaderGetParentHash(header);
    
    
    return (gotBlockNumber == expectedBlockNumber &&
            hashSetEqual(&gotHash, &expectedHash)  &&
            gotGasUsed == expectedGasUsed &&
            (hashSetEqual(&gotParentHash, &expectedParenthash)
             ? ETHEREUM_BOOLEAN_TRUE
             : ETHEREUM_BOOLEAN_FALSE));
    
}

static BREthereumBoolean
_checkBlockHeaderWithTest (BREthereumLESProvisionContext context,
                           BREthereumProvisionResult result,
                           BlockHeaderTestData *data,
                           unsigned int count) {
    BREthereumHash expectedHash = data->hash;
    uint64_t expectedBlockNumber =  data->blockNum;
    UInt256 expectedDifficulty = data->difficulty;
    uint64_t expectedGasUsed = data->gasUsed;
    BREthereumHash expectedParenthash = data->parent;

    assert (PROVISION_BLOCK_HEADERS == result.type);
    assert (PROVISION_SUCCESS == result.status);

    BRArrayOf(BREthereumBlockHeader) headers = result.u.success.provision.u.headers.headers;
    assert (count == array_count (headers));

    assert(ETHEREUM_BOOLEAN_IS_TRUE(_checkBlockHeader (headers[0], expectedHash, expectedBlockNumber, expectedDifficulty, expectedGasUsed, expectedParenthash)));

    return ETHEREUM_BOOLEAN_TRUE;
}

void _GetBlockHeaders_Calllback_Test5 (BREthereumLESProvisionContext context,
                                       BREthereumLES les,
                                       BREthereumNodeReference node,
                                       BREthereumProvisionResult result) {

    assert(context != NULL);
    int* context1 = (int *)context;

    assert(context1 == &_GetBlockHeaders_Context5); //Check to make sure the context is correct

    _checkBlockHeaderWithTest (context, result, &_blockHeaderTestData[_GetBlockHeaders_Context4], 200);

    // Monotonically increasing header block number.
    BRArrayOf(BREthereumBlockHeader) headers = result.u.success.provision.u.headers.headers;
    for (size_t index = 0; index < 200; index++)
        assert (index == (blockHeaderGetNumber(headers[index]) - blockHeaderGetNumber(headers[0])));

    _signalTestComplete();
}


void _GetBlockHeaders_Calllback_Test4 (BREthereumLESProvisionContext context,
                                       BREthereumLES les,
                                       BREthereumNodeReference node,
                                       BREthereumProvisionResult result) {
    
    assert(context != NULL);
    int* context1 = (int *)context;
    
    assert(context1 == &_GetBlockHeaders_Context4); //Check to make sure the context is correct

    _checkBlockHeaderWithTest (context, result, &_blockHeaderTestData[_GetBlockHeaders_Context4], 2);
    _signalTestComplete();
}

void _GetBlockHeaders_Calllback_Test3 (BREthereumLESProvisionContext context,
                                       BREthereumLES les,
                                       BREthereumNodeReference node,
                                       BREthereumProvisionResult result) {
    
    assert(context != NULL);
    int* context1 = (int *)context;
    
    assert(context1 == &_GetBlockHeaders_Context3); //Check to make sure the context is correct

    _checkBlockHeaderWithTest (context, result, &_blockHeaderTestData[_GetBlockHeaders_Context3], 2);
    _signalTestComplete();
}
void _GetBlockHeaders_Calllback_Test2 (BREthereumLESProvisionContext context,
                                       BREthereumLES les,
                                       BREthereumNodeReference node,
                                       BREthereumProvisionResult result) {
    
    assert(context != NULL);
    int* context1 = (int *)context;
    
    assert(context1 == &_GetBlockHeaders_Context2); //Check to make sure the context is correct
    
    _checkBlockHeaderWithTest (context, result, &_blockHeaderTestData[_GetBlockHeaders_Context2], 3);
    _signalTestComplete();
}

void _GetBlockHeaders_Calllback_Test1 (BREthereumLESProvisionContext context,
                                       BREthereumLES les,
                                       BREthereumNodeReference node,
                                       BREthereumProvisionResult result) {
    
    assert(context != NULL);
    int* context1 = (int *)context;
    
    assert(context1 == &_GetBlockHeaders_Context1); //Check to make sure the context is correct

    _checkBlockHeaderWithTest (context, result, &_blockHeaderTestData[_GetBlockHeaders_Context1], 3);
    _signalTestComplete();
}

static void
run_GetBlockHeaders_Tests (BREthereumLES les){
    //Initialze test
    _initTest(5);

    //Request block headers 4732522, 4732523, 4732524
    _GetBlockHeaders_Context1 = BLOCK_4732522_IDX;
    lesProvideBlockHeaders (les,
                            (void*)&_GetBlockHeaders_Context1,
                            _GetBlockHeaders_Calllback_Test1,
                            _blockHeaderTestData[BLOCK_4732522_IDX].blockNum,
                            3, 0, ETHEREUM_BOOLEAN_FALSE);

    //Request block headers 4732522, 4732521, 4732520
    _GetBlockHeaders_Context2 = BLOCK_4732522_IDX;
    lesProvideBlockHeaders (les,
                            (void*)&_GetBlockHeaders_Context2,
                            _GetBlockHeaders_Calllback_Test2,
                            _blockHeaderTestData[BLOCK_4732522_IDX].blockNum,
                            3, 0, ETHEREUM_BOOLEAN_TRUE);

    //Request block headers 4732522, 4732524
    _GetBlockHeaders_Context3 = BLOCK_4732522_IDX;
    lesProvideBlockHeaders (les,
                            (void*)&_GetBlockHeaders_Context3,
                            _GetBlockHeaders_Calllback_Test3,
                            _blockHeaderTestData[BLOCK_4732522_IDX].blockNum,
                            2, 1, ETHEREUM_BOOLEAN_FALSE);

    //Request block headers 4732522, 4732520
    _GetBlockHeaders_Context4 = BLOCK_4732522_IDX;
    lesProvideBlockHeaders (les,
                            (void*)&_GetBlockHeaders_Context4,
                            _GetBlockHeaders_Calllback_Test4,
                            _blockHeaderTestData[BLOCK_4732522_IDX].blockNum,
                            2, 1, ETHEREUM_BOOLEAN_TRUE) ;

    _GetBlockHeaders_Context5 = BLOCK_4732522_IDX;
    lesProvideBlockHeaders (les,
                            (void*)&_GetBlockHeaders_Context5,
                            _GetBlockHeaders_Calllback_Test5,
                            _blockHeaderTestData[BLOCK_4732522_IDX].blockNum,
                            200, 0, ETHEREUM_BOOLEAN_FALSE) ;

    //Wait for tests to complete
    _waitForTests();
    
    eth_log(TST_LOG_TOPIC, "GetBlockHeaders: %s", "Tests Successful");
}

//
// Testing GetTxtStatus message
//
static const int _GetTxStatus_Context1 = 1;
static const int _GetTxStatus_Context2 = 2;

static void
_GetTxStatus_Test2_Callback (BREthereumLESProvisionContext context,
                                        BREthereumLES les,
                                        BREthereumNodeReference node,
                                        BREthereumProvisionResult result) {
    assert (PROVISION_SUCCESS == result.status);
    assert (PROVISION_TRANSACTION_STATUSES == result.u.success.provision.type);

    BRArrayOf(BREthereumHash) hashes = result.u.success.provision.u.statuses.hashes;
    BRArrayOf(BREthereumTransactionStatus) statusesByHash = result.u.success.provision.u.statuses.statuses;

    assert (2 == array_count(hashes));
    assert (2 == array_count(statusesByHash));

    assert(context != NULL);
    int* context1 = (int *)context;
    
    assert(*context1 == _GetTxStatus_Context2); //Check to make sure the context is correct
    
    //Check to make sure we get back the right transaction
    BREthereumHash expectedTransactionHashes[] = {
        hashCreate("0xc070b1e539e9a329b14c95ec960779359a65be193137779bf2860dc239248d7c"),
        hashCreate("0x78453edd2955e6ef6b200f5f9b98b3940d0d3f1528f902e7e855df56bf934cc5")
    };

    BREthereumHash expectedBlockHashes[] = {
        hashCreate("0xf16becb908162df51c3789fab0e6ba52568fa7ee7d0127eb51bfaa0bcd40fb1b"),
        hashCreate("0x0a4b16bac21b6dfeb51ccb522d8c34840844ae78ed0bc177670c501c18d35ff2")
    };

    uint64_t expectedBlockNumbers[] = {
        5202375,
        5766700
    };

    uint64_t expectedTransactionIndexes[] = {
        39,
        36
    };

    for (size_t index = 0; index < 2; index++) {
        assert(hashSetEqual(&hashes[index], &expectedTransactionHashes[index]));

        assert (TRANSACTION_STATUS_INCLUDED == statusesByHash[index].type);


        assert(hashSetEqual(&statusesByHash[index].u.included.blockHash, &expectedBlockHashes[index]));

        assert(statusesByHash[index].u.included.blockNumber == expectedBlockNumbers[index]);
        assert(statusesByHash[index].u.included.transactionIndex == expectedTransactionIndexes[index]);
    }

    //Signal to the testing thread that this test completed successfully
    _signalTestComplete();
}

static void
_GetTxStatus_Test1_Callback (BREthereumLESProvisionContext context,
                             BREthereumLES les,
                             BREthereumNodeReference node,
                             BREthereumProvisionResult result) {
    // BREthereumHash transaction,
    // BREthereumTransactionStatus status){

    assert (PROVISION_SUCCESS == result.status);
    assert (PROVISION_TRANSACTION_STATUSES == result.u.success.provision.type);

    BRArrayOf(BREthereumHash) hashes = result.u.success.provision.u.statuses.hashes;
    BRArrayOf(BREthereumTransactionStatus) statusesByHash = result.u.success.provision.u.statuses.statuses;

    assert (1 == array_count(hashes));
    assert (1 == array_count(statusesByHash));


    assert(context != NULL);
    int* context1 = (int *)context;
    
    assert(*context1 == _GetTxStatus_Context1); //Check to make sure the context is correct

    BREthereumHash expectedTransactionHashes[] = {
        hashCreate("0xc070b1e539e9a329b14c95ec960779359a65be193137779bf2860dc239248d7c"),
    };

    BREthereumHash expectedBlockHashes[] = {
        hashCreate("0xf16becb908162df51c3789fab0e6ba52568fa7ee7d0127eb51bfaa0bcd40fb1b"),
    };

    uint64_t expectedBlockNumbers[] = {
        5202375,
    };

    uint64_t expectedTransactionIndexes[] = {
        39,
    };

    for (size_t index = 0; index < 1; index++) {
        assert(hashSetEqual(&hashes[index], &expectedTransactionHashes[index]));

        assert (TRANSACTION_STATUS_INCLUDED == statusesByHash[index].type);


        assert(hashSetEqual(&statusesByHash[index].u.included.blockHash, &expectedBlockHashes[index]));

        assert(statusesByHash[index].u.included.blockNumber == expectedBlockNumbers[index]);
        assert(statusesByHash[index].u.included.transactionIndex == expectedTransactionIndexes[index]);
    }

    //Signal to the testing thread that this test completed successfully
    _signalTestComplete();
}

static void
run_GetTxStatus_Tests (BREthereumLES les) {
    
    // Prepare values to be given to a send tranactions status message
    BREthereumHash transaction1Hash = hashCreate("0xc070b1e539e9a329b14c95ec960779359a65be193137779bf2860dc239248d7c");
    BREthereumHash transaction2Hash = hashCreate("0x78453edd2955e6ef6b200f5f9b98b3940d0d3f1528f902e7e855df56bf934cc5");
    
    //Initilize testing state
    _initTest(2);
    
    lesProvideTransactionStatusOne (les,
                                    (void *)&_GetTxStatus_Context1,
                                    _GetTxStatus_Test1_Callback,
                                    transaction1Hash);
    
    BREthereumHash* transactions;
    array_new(transactions, 2);
    array_add(transactions, transaction1Hash);
    array_add(transactions, transaction2Hash);
    
    lesProvideTransactionStatus (les,
                                 (void *)&_GetTxStatus_Context2,
                                 _GetTxStatus_Test2_Callback,
                                 transactions);
    
    //Wait for tests to complete
    _waitForTests();
    
    eth_log(TST_LOG_TOPIC, "GetTxStatus: %s", "Tests Successful");
}

//
//  Testing BlockBodies message
//
static int _GetBlockBodies_Context1 = 0;
static int _GetBlockBodies_Context2 = 0;

static void
_GetBlockBodies_Callback_Test1 (BREthereumLESProvisionContext context,
                                           BREthereumLES les,
                                           BREthereumNodeReference node,
                                           BREthereumProvisionResult result) {
    assert (PROVISION_SUCCESS == result.status);
    assert (PROVISION_BLOCK_BODIES == result.u.success.provision.type);

    BRArrayOf(BREthereumHash) hashes = result.u.success.provision.u.bodies.hashes;
    BRArrayOf(BREthereumBlockBodyPair) pairs = result.u.success.provision.u.bodies.pairs;

    assert (1 == array_count(hashes));
    assert (1 == array_count(pairs));

    assert(context != NULL);
    int* context1 = (int *)context;
    
    assert(*context1 == _GetBlockBodies_Context1); //Check to make sure the context is correct
    
    //Check Block Hash
    assert(hashSetEqual(&hashes[0], &_blockHeaderTestData[_GetBlockBodies_Context1].hash));
    
    //Check to make sure we got back the right number of transactions and ommers
    assert(array_count(pairs[0].transactions) == _blockHeaderTestData[_GetBlockBodies_Context1].transactionCount);
    assert(array_count(pairs[0].uncles)       == _blockHeaderTestData[_GetBlockBodies_Context1].ommersCount);
    
    //Signal to the testing thread that this test completed successfully
    _signalTestComplete();
}

static void
_GetBlockBodies_Callback_Test2 (BREthereumLESProvisionContext context,
                                BREthereumLES les,
                                BREthereumNodeReference node,
                                BREthereumProvisionResult result) {
    assert (PROVISION_SUCCESS == result.status);
    assert (PROVISION_BLOCK_BODIES == result.u.success.provision.type);

    BRArrayOf(BREthereumHash) hashes = result.u.success.provision.u.bodies.hashes;
    BRArrayOf(BREthereumBlockBodyPair) pairs = result.u.success.provision.u.bodies.pairs;

    assert (2 == array_count(hashes));
    assert (2 == array_count(pairs));

    assert(context != NULL);
    int* context1 = (int *)context;
    
    assert(*context1 == _GetBlockBodies_Context2); //Check to make sure the context is correct

    for (size_t index = 0; index < 2; index++) {
        //Check Block Hash
        assert(hashSetEqual(&hashes[index], &_blockHeaderTestData[_GetBlockBodies_Context2 + index].hash));
    
        //Check to make sure we got back the right number of transactions and ommers
        assert(array_count(pairs[index].transactions) == _blockHeaderTestData[_GetBlockBodies_Context2 + index].transactionCount);
        assert(array_count(pairs[index].uncles)       == _blockHeaderTestData[_GetBlockBodies_Context2 + index].ommersCount);
    }

    //Signal to the testing thread that this test completed successfully
    _signalTestComplete();
}

static void
run_GetBlockBodies_Tests (BREthereumLES les) {
    
    //Initilize testing state
    _initTest(2);
    
    //Request block bodies 4732522
    _GetBlockBodies_Context1 = BLOCK_4732522_IDX;
    lesProvideBlockBodiesOne (les,
                              (void *)&_GetBlockBodies_Context1,
                              _GetBlockBodies_Callback_Test1,
                              _blockHeaderTestData[BLOCK_4732522_IDX].hash);
    
    //Request block bodies 4732522, 4732521
    _GetBlockBodies_Context2 = BLOCK_4732522_IDX;
    BREthereumHash* blockHeaders;
    array_new(blockHeaders, 2);
    array_add(blockHeaders, _blockHeaderTestData[BLOCK_4732522_IDX].hash);
    array_add(blockHeaders, _blockHeaderTestData[BLOCK_4732522_IDX + 1].hash);
    
    lesProvideBlockBodies (les,
                           (void *)&_GetBlockBodies_Context2,
                           _GetBlockBodies_Callback_Test2,
                           blockHeaders);
    
    //Wait for tests to complete
    _waitForTests();
    
    eth_log(TST_LOG_TOPIC, "GetBlockBlodies: %s", "Tests Successful");
}

//
// Testing GetReceipts
//
static int _GetReceipts_Context1 = 0;
static int _GetReceipts_Context2 = 0;

static void
_GetReceipts_Callback_Test1 (BREthereumLESProvisionContext context,
                             BREthereumLES les,
                             BREthereumNodeReference node,
                             BREthereumProvisionResult result) {
    assert (PROVISION_SUCCESS == result.status);
    assert (PROVISION_TRANSACTION_RECEIPTS == result.u.success.provision.type);

    BRArrayOf(BREthereumHash) hashes = result.u.success.provision.u.receipts.hashes;
    BRArrayOf(BRArrayOf(BREthereumTransactionReceipt)) receiptsByHash = result.u.success.provision.u.receipts.receipts;

    assert (1 == array_count(hashes));
    assert (1 == array_count(receiptsByHash));

    assert(context != NULL);
    int* context1 = (int *)context;
    
    assert(*context1 == _GetReceipts_Context1); //Check to make sure the context is correct
    
    //Check Block Hash
    assert(hashSetEqual(&hashes[0], &_blockHeaderTestData[_GetReceipts_Context1].hash));
    
    //Check to make sure we got back the right number of transactions and ommers
    assert(array_count(receiptsByHash[0]) == _blockHeaderTestData[_GetReceipts_Context1].transactionCount);
    
    //Signal to the testing thread that this test completed successfully
    _signalTestComplete();
}

static void
_GetReceipts_Callback_Test2 (BREthereumLESProvisionContext context,
                             BREthereumLES les,
                             BREthereumNodeReference node,
                             BREthereumProvisionResult result) {

    assert (PROVISION_SUCCESS == result.status);
    assert (PROVISION_TRANSACTION_RECEIPTS == result.u.success.provision.type);

    BRArrayOf(BREthereumHash) hashes = result.u.success.provision.u.receipts.hashes;
    BRArrayOf(BRArrayOf(BREthereumTransactionReceipt)) receiptsByHash = result.u.success.provision.u.receipts.receipts;

    assert (2 == array_count(hashes));
    assert (2 == array_count(receiptsByHash));

    assert(context != NULL);
    int* context1 = (int *)context;
    
    assert(*context1 == _GetReceipts_Context2); //Check to make sure the context is correct

    for (size_t index = 0; index < 2; index++) {
        //Check Block Hash
        assert(hashSetEqual(&hashes[index], &_blockHeaderTestData[_GetReceipts_Context2 + index].hash));
    
        //Check to make sure we got back the right number of receipts
        assert(array_count(receiptsByHash[index]) == _blockHeaderTestData[_GetReceipts_Context2 + index].transactionCount);
    }

    _signalTestComplete();
}

static void
run_GetReceipts_Tests (BREthereumLES les) {
    
    //Initilize testing state
    _initTest(2);
    
    //Request receipts for block 4732522
    _GetReceipts_Context1 = BLOCK_4732522_IDX;
    lesProvideReceiptsOne (les,
                           (void *) &_GetReceipts_Context1,
                           _GetReceipts_Callback_Test1,
                           _blockHeaderTestData[BLOCK_4732522_IDX].hash);
    
    //Request receipts for block 4732522, 4732521
    _GetReceipts_Context2 = BLOCK_4732522_IDX;
    BREthereumHash* blockHeaders;
    array_new(blockHeaders, 2);
    array_add(blockHeaders, _blockHeaderTestData[BLOCK_4732522_IDX].hash);
    array_add(blockHeaders, _blockHeaderTestData[BLOCK_4732522_IDX + 1].hash);
    
    lesProvideReceipts (les,
                        (void *)&_GetReceipts_Context2,
                        _GetReceipts_Callback_Test2,
                        blockHeaders);
    
    //Wait for tests to complete
    _waitForTests();
    
    eth_log(TST_LOG_TOPIC, "GetReceipts: %s", "Tests Successful");
    
}

//
// Test GetProofsV2
//
#if 0
static int _GetProofsV2_Context1 = 0;

static void _GetProofs_Callback_Test1(BREthereumLESProvisionContext context,
                                      BREthereumHash blockHash,
                                      BRRlpData key1,
                                      BRRlpData key2) {
    
    assert(context != NULL);
    int* context1 = (int *)context;
    
    assert(*context1 == _GetProofsV2_Context1); //Check to make sure the context is correct
    
    //Signal to the testing thread that this test completed successfully
    _signalTestComplete(); 
}

static BREthereumHash blockHashes[] = {
    HASH_INIT ("d4e56740f876aef8c010b86a40d5f56745a118d0906a34e69aec8c0db1cb8fa3"), // 0
    HASH_INIT ("88e96d4537bea4d9c05d12549907b32561d3bf31f45aae734cdc119f13406cb6"), // 1
    HASH_INIT ("b495a1d7e6663152ae92708da4843337b958146015a2802f4193a410044698c9"), // 2
    HASH_INIT ("373df5c980b96580d00715fac46d667b217f00b731f990c636df24a592c75015"), // 4095
    HASH_INIT ("037a9074fc7802a0d1095e035f37bbe6ed18a340b0a2009643c6fc693eae4383"), // 4096
    HASH_INIT ("b6700dd17c4b4881566f6ef50f60a000bba1c553553ffa910fbbf898c46d0b87"), // 4097
    HASH_INIT ("089a6c0b4b960261287d30ee40b1eea2da2972e7189bd381137f55540d492b2c"), // 5,503,921
    HASH_INIT ("204167e38efa1a4d75c996491637027bb1c8b1fe0d29e8d233160b5256cb415a"), // 6,100,000
};
static size_t blockCount = sizeof (blockHashes) / sizeof(BREthereumHash);

static void run_GetProofsV2_Tests(BREthereumLES les){
    
    //Initilize testing state
    _initTest((int) blockCount);
    
    BREthereumAddress address = addressCreate("0x49f4C50d9BcC7AfdbCF77e0d6e364C29D5a660DF");
    
    BREthereumHash addr  = addressGetHash(address);
    
    // Have mucked with combinations of key1, key2 being: key1 empty, key2 empty, both key1 adn
    // key2 provided.  With key1, key2 being: the addres, the hash of address.
    //
    // Have mucked with the Go Ethereum test case - to explore the meanings of key1, key2.
    //
    
    BRRlpData key1 = (BRRlpData) { 0, NULL };
    BRRlpData key2 = (BRRlpData) { sizeof (addr), addr.bytes };
    
    for (size_t blockIndex = 0; blockIndex < blockCount; blockIndex++) {
        BREthereumHash block = blockHashes[blockIndex];
        lesGetProofsV2One(les, (void *)&_GetProofsV2_Context1, _GetProofs_Callback_Test1, block, key1, key2, 0);
    }
    
    //Wait for tests to complete
    _waitForTests();// sleep (5);
    
    eth_log(TST_LOG_TOPIC, "GetProofsV2: %s", "Tests Successful");
}
#endif

//
// Test GetAccountState
//
static int _GetAccount_Context1 = 0;
static void _GetAccountState_Callback_Test1 (BREthereumLESProvisionContext context,
                                             BREthereumLES les,
                                             BREthereumNodeReference node,
                                             BREthereumProvisionResult result) {

    assert (PROVISION_SUCCESS == result.status);
    assert (PROVISION_ACCOUNTS == result.u.success.provision.type);

    BRArrayOf(BREthereumHash) hashes = result.u.success.provision.u.accounts.hashes;
    BRArrayOf(BREthereumAccountState) accountsByHash = result.u.success.provision.u.accounts.accounts;

    //    BREthereumAddress address = result.u.success.provision.u.accounts.address;
    //    BRArrayOf(uint64_t) numbers = result.u.success.provision.u.accounts.numbers;

    assert (1 == array_count(hashes));
    assert (1 == array_count(accountsByHash));

    assert(context != NULL);
    int* context1 = (int *)context;

    assert(*context1 == _GetAccount_Context1); //Check to make sure the context is correct

    assert (0xcbee == accountsByHash[0].nonce);

    _signalTestComplete();
}

static void _GetAccountState_Callback_Test2 (BREthereumLESProvisionContext context,
                                             BREthereumLES les,
                                             BREthereumNodeReference node,
                                             BREthereumProvisionResult result) {


    assert (PROVISION_SUCCESS == result.status);
    assert (PROVISION_ACCOUNTS == result.u.success.provision.type);

    BRArrayOf(BREthereumHash) hashes = result.u.success.provision.u.accounts.hashes;
    BRArrayOf(BREthereumAccountState) accountsByHash = result.u.success.provision.u.accounts.accounts;

    //    BREthereumAddress address = result.u.success.provision.u.accounts.address;
    //    BRArrayOf(uint64_t) numbers = result.u.success.provision.u.accounts.numbers;

    assert (2 == array_count(hashes));
    assert (2 == array_count(accountsByHash));

    assert(context != NULL);
    int* context1 = (int *)context;

    assert(*context1 == _GetAccount_Context1); //Check to make sure the context is correct

    assert (0xcbee == accountsByHash[0].nonce);
    assert (0xcbee == accountsByHash[1].nonce);

    _signalTestComplete();
}

static void run_GetAccountState_Tests (BREthereumLES les){
    //Initilize testing state
    _initTest(3);

    BREthereumAddress address = addressCreate("0x52bc44d5378309ee2abf1539bf71de1b7d7be3b5");

    BREthereumHash block_350000 = hashCreate("0x8cd1f73a98ab1cdd65f829530a46559d3ea345f330bc04924f30fe00bcbad6f1");
    lesProvideAccountStatesOne (les,
                                (void*) &_GetAccount_Context1,
                                _GetAccountState_Callback_Test1,
                                address,
                                block_350000);

    BREthereumHash block_349999 = hashCreate("0xb86a49b1589b3f8474171d35c796e27f5c20ba1db16a2d93cf67d7358122b361");
    lesProvideAccountStatesOne (les,
                                (void*) &_GetAccount_Context1,
                                _GetAccountState_Callback_Test1,
                                address,
                                block_349999);

    BRArrayOf(BREthereumHash) hashes;
    array_new (hashes, 2);

    array_add (hashes, block_350000); array_add (hashes, block_349999);
    lesProvideAccountStates (les,
                             (void*) &_GetAccount_Context1,
                             _GetAccountState_Callback_Test2,
                             address,
                             hashes);

    _waitForTests();
    eth_log(TST_LOG_TOPIC, "GetAccopuntState: %s", "Tests Successful");
}

extern void
runLEStests(void) {
    
    //Prepare values to be given to a LES context
    char headHashStr[] = "0xd4e56740f876aef8c010b86a40d5f56745a118d0906a34e69aec8c0db1cb8fa3";
    BREthereumHash headHash = hashCreate(headHashStr);
    
    uint64_t headNumber = 0;
    UInt256 headTD = createUInt256 (0x400000000);
    
    BREthereumHash genesisHash = hashCreate(headHashStr);
    
    // Create an LES context
    BREthereumLES les = lesCreate (ethereumMainnet,
                                   NULL, _announceCallback, _statusCallback, _saveNodesCallback,
                                   headHash, headNumber, headTD, genesisHash,
                                   NULL);
    lesStart(les);
    // Sleep for a little bit to allow the context to connect to the network
    sleep(2);
    
    //Initialize testing state
    _initTestState();
    
    //Initialize data needed for tests
    _initBlockHeaderTestData();
    
    // Run Tests on the LES messages
    run_GetBlockHeaders_Tests(les);
    run_GetBlockBodies_Tests(les);
    run_GetReceipts_Tests(les);
    run_GetAccountState_Tests(les);
    run_GetTxStatus_Tests(les);
    //    run_GetProofsV2_Tests(les); //NOTE: The callback function won't be called.
    //    reallySendLESTransaction(les);
    
    //    sleep (60);
    lesStop(les);
    lesRelease(les);
    printf ("Done\n");
}
