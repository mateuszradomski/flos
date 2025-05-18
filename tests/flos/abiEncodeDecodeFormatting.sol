// SPDX-License-Identifier: MIT
pragma solidity >=0.8.0;

contract AbiEncodeDecodeFormatting {

    struct MyData {
        uint256 id;
        string name;
        address owner;
    }

    event Encoded(bytes data);
    event Decoded(uint256 id, string name);

    function testEncodingDecoding(uint256 _id, string memory _name, address _owner, address target, bytes4 selector) external {
        // abi.encode
        bytes memory encodedData = abi.encode(
            _id,
            _name,
            _owner,
            block.timestamp,
            "a literal string",
            true,
            hex"deadbeef",
            MyData({ id: _id + 1, name: string.concat(_name, " suffix"), owner: target })
        );
        emit Encoded(encodedData);

        // abi.encodePacked (note: generally unsafe)
        bytes memory packedData = abi.encodePacked(
            _id, _name, _owner, block.timestamp, "literal", true, bytes4(0x12345678), // Many args requiring break
            "another literal string to force line break for sure"
        );
        emit Encoded(packedData);

        // abi.encodeWithSelector
        bytes memory callDataWithSelector = abi.encodeWithSelector(
            selector, // function selector
            _id,
            _name,
            _owner,
            // Add many more args to force break
            uint256(1), uint256(2), uint256(3), uint256(4), uint256(5), uint256(6), uint256(7), address(this)
        );
        emit Encoded(callDataWithSelector);

        // abi.encodeWithSignature
        bytes memory callDataWithSig = abi.encodeWithSignature(
            "myFunction(uint256,string,address,bool,bytes)", // function signature
            _id,
            _name,
            _owner,
            true,
            hex"cafebabe"
        );
        emit Encoded(callDataWithSig);

        // abi.decode (assuming encodedData structure)
        (uint256 decodedId, string memory decodedName, , , , , , ) = abi.decode(encodedData, (uint256, string, address, uint256, string, bool, bytes4, MyData));
        emit Decoded(decodedId, decodedName);

        // Decode with complex tuple type forcing breaks
        (
            uint256 a,
            MyData memory b,
            string memory c,
            bool d,
            address[] memory e,
            uint256[3] memory f,
            bytes memory g
        ) = abi.decode(encodedData, (uint256, MyData, string, bool, address[], uint256[3], bytes));
    }
}
// Above input, below output
// SPDX-License-Identifier: MIT
pragma solidity >=0.8.0;

contract AbiEncodeDecodeFormatting {
    struct MyData {
        uint256 id;
        string name;
        address owner;
    }

    event Encoded(bytes data);
    event Decoded(uint256 id, string name);

    function testEncodingDecoding(uint256 _id, string memory _name, address _owner, address target, bytes4 selector)
        external
    {
        // abi.encode
        bytes memory encodedData = abi.encode(
            _id,
            _name,
            _owner,
            block.timestamp,
            "a literal string",
            true,
            hex"deadbeef",
            MyData({ id: _id + 1, name: string.concat(_name, " suffix"), owner: target })
        );
        emit Encoded(encodedData);

        // abi.encodePacked (note: generally unsafe)
        bytes memory packedData = abi.encodePacked(
            _id,
            _name,
            _owner,
            block.timestamp,
            "literal",
            true,
            bytes4(0x12345678), // Many args requiring break
            "another literal string to force line break for sure"
        );
        emit Encoded(packedData);

        // abi.encodeWithSelector
        bytes memory callDataWithSelector = abi.encodeWithSelector(
            selector, // function selector
            _id,
            _name,
            _owner,
            // Add many more args to force break
            uint256(1),
            uint256(2),
            uint256(3),
            uint256(4),
            uint256(5),
            uint256(6),
            uint256(7),
            address(this)
        );
        emit Encoded(callDataWithSelector);

        // abi.encodeWithSignature
        bytes memory callDataWithSig = abi.encodeWithSignature(
            "myFunction(uint256,string,address,bool,bytes)", // function signature
            _id,
            _name,
            _owner,
            true,
            hex"cafebabe"
        );
        emit Encoded(callDataWithSig);

        // abi.decode (assuming encodedData structure)
        (uint256 decodedId, string memory decodedName, , , , , , ) = abi.decode(
            encodedData,
            (uint256, string, address, uint256, string, bool, bytes4, MyData)
        );
        emit Decoded(decodedId, decodedName);

        // Decode with complex tuple type forcing breaks
        (
            uint256 a,
            MyData memory b,
            string memory c,
            bool d,
            address[] memory e,
            uint256[3] memory f,
            bytes memory g
        ) = abi.decode(encodedData, (uint256, MyData, string, bool, address[], uint256[3], bytes));
    }
}
