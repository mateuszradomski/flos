// SPDX-License-Identifier: MIT
pragma solidity >=0.8.0;

contract YulOpcodesFormatting {
    uint256 s1;
    mapping(uint256 => uint256) sMap;
    event Logged(uint256 indexed topic1, bytes data);

    function testOpcodes(uint256 key, uint256 value, uint256 index, bytes memory dataToLog) external payable {
        address addr;
        uint256 balance;
        uint256 gas;
        bytes32 codeHash;
        uint256 chainId;

        assembly {
            // Environment Opcodes
            addr := caller()
            balance := balance(caller())
            gas := gas()
            codeHash := codehash(address())
            chainId := chainid()
            addr := origin()
            addr := coinbase()
            value := callvalue()
            value := gasprice()
            value := difficulty()
            value := gaslimit()
            value := number()
            value := timestamp()

            // Memory Operations
            mstore8(0x80, 0xff)      // Store byte
            value := mload(0x40)     // Load word

            // Storage Operations
            sstore(s1.slot, 0x1234) // Store simple state variable
            sstore(key, value)      // Store mapping value directly (less common, tests key hashing)
            value := sload(sMap.slot) // Load mapping base slot
            value := sload(s1.slot)  // Load simple state variable

            // Logging
            log0(0x00, 0x20)
            log1(0x20, 0x20, chainId)
            log2(0x40, 0x20, chainId, number())
            log3(0x60, 0x20, chainId, number(), timestamp())
            log4(0x80, 0x20, chainId, number(), timestamp(), gaslimit())

            // System Operations
            addr := create(callvalue(), 0x00, 0x20) // Requires bytecode
            addr := create2(callvalue(), 0x20, 0x20, key) // Requires bytecode
            value := staticcall(gas(), addr, 0x00, 0x20, 0x00, 0x00) // Requires target
            selfdestruct(caller())

            // Test formatting with comments around opcodes
            value := /*comment*/ add(1, /*comment*/ 2) // comment
            sstore(/*slot*/ s1.slot, /* value */ 0x5678 /*end*/) // TODO(radomski): Space should be before s1
        }
    }
}
// Above input, below output
// SPDX-License-Identifier: MIT
pragma solidity >=0.8.0;

contract YulOpcodesFormatting {
    uint256 s1;
    mapping(uint256 => uint256) sMap;
    event Logged(uint256 indexed topic1, bytes data);

    function testOpcodes(uint256 key, uint256 value, uint256 index, bytes memory dataToLog) external payable {
        address addr;
        uint256 balance;
        uint256 gas;
        bytes32 codeHash;
        uint256 chainId;

        assembly {
            // Environment Opcodes
            addr := caller()
            balance := balance(caller())
            gas := gas()
            codeHash := codehash(address())
            chainId := chainid()
            addr := origin()
            addr := coinbase()
            value := callvalue()
            value := gasprice()
            value := difficulty()
            value := gaslimit()
            value := number()
            value := timestamp()

            // Memory Operations
            mstore8(0x80, 0xff) // Store byte
            value := mload(0x40) // Load word

            // Storage Operations
            sstore(s1.slot, 0x1234) // Store simple state variable
            sstore(key, value) // Store mapping value directly (less common, tests key hashing)
            value := sload(sMap.slot) // Load mapping base slot
            value := sload(s1.slot) // Load simple state variable

            // Logging
            log0(0x00, 0x20)
            log1(0x20, 0x20, chainId)
            log2(0x40, 0x20, chainId, number())
            log3(0x60, 0x20, chainId, number(), timestamp())
            log4(0x80, 0x20, chainId, number(), timestamp(), gaslimit())

            // System Operations
            addr := create(callvalue(), 0x00, 0x20) // Requires bytecode
            addr := create2(callvalue(), 0x20, 0x20, key) // Requires bytecode
            value := staticcall(gas(), addr, 0x00, 0x20, 0x00, 0x00) // Requires target
            selfdestruct(caller())

            // Test formatting with comments around opcodes
            value := /*comment*/ add(1, /*comment*/ 2) // comment
            sstore( /*slot*/s1.slot, /* value */ 0x5678 /*end*/) // TODO(radomski): Space should be before s1
        }
    }
}
