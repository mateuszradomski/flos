// SPDX-License-Identifier: MIT
pragma solidity >=0.8.0;

contract TypeCastingFormatting {
    address payable public recipient;
    bytes public myBytes;
    uint8 public u8;
    int256 public i256;

    function testCasting(uint256 value, address addr, bytes32 b32, int i) external {
        // Simple casts
        uint256 castedValue = uint256(u8);
        address castedAddr = address(uint160(value));
        address payable castedPayable = payable(addr);
        bytes32 castedBytes32 = bytes32(value);
        bytes memory castedBytes = bytes(b32);
        string memory castedString = string(myBytes);
        int256 castedInt = int256(value);
        uint256 castedUint = uint256(i);

        // Casts within long expressions
        uint256 result1 = uint256(u8) + 10000000000000000000000000000000000000000000000 * 2;
        bool check1 = address(uint160(value)) == addr || address(uint160(value + 1)) != recipient;

        // Payable cast requiring break
        address payable longPayableCastVariable = payable(
            address(uint160(uint256(keccak256(abi.encodePacked(value, addr, block.timestamp)))))
        );

        // Cast as function argument in long call
        anotherLongFunctionCall(
            uint256(u8),
            bytes(b32),
            int256(value),
            payable(addr), // requires break
            address(uint160(value + 100000000000000000000000000000000000000000000000))
        );

        // Cast with comments
        uint256 commentedCast = uint256 /* to uint */ (u8) /* from u8 */;
        address payable commentedPayable = payable /* make payable */ ( /* the */ addr /* address */ );
    }

    function anotherLongFunctionCall(uint256, bytes memory, int256, address payable, address) public pure {}
}
// Above input, below output
// SPDX-License-Identifier: MIT
pragma solidity >=0.8.0;

contract TypeCastingFormatting {
    address payable public recipient;
    bytes public myBytes;
    uint8 public u8;
    int256 public i256;

    function testCasting(uint256 value, address addr, bytes32 b32, int i) external {
        // Simple casts
        uint256 castedValue = uint256(u8);
        address castedAddr = address(uint160(value));
        address payable castedPayable = payable(addr);
        bytes32 castedBytes32 = bytes32(value);
        bytes memory castedBytes = bytes(b32);
        string memory castedString = string(myBytes);
        int256 castedInt = int256(value);
        uint256 castedUint = uint256(i);

        // Casts within long expressions
        uint256 result1 = uint256(u8) + 10000000000000000000000000000000000000000000000 * 2;
        bool check1 = address(uint160(value)) == addr || address(uint160(value + 1)) != recipient;

        // Payable cast requiring break
        address payable longPayableCastVariable = payable(
            address(uint160(uint256(keccak256(abi.encodePacked(value, addr, block.timestamp)))))
        );

        // Cast as function argument in long call
        anotherLongFunctionCall(
            uint256(u8),
            bytes(b32),
            int256(value),
            payable(addr), // requires break
            address(uint160(value + 100000000000000000000000000000000000000000000000))
        );

        // Cast with comments
        uint256 commentedCast = uint256 /* to uint */(u8) /* from u8 */;
        address payable commentedPayable = payable /* make payable */( /* the */addr /* address */);
    }

    function anotherLongFunctionCall(uint256, bytes memory, int256, address payable, address) public pure { }
}
