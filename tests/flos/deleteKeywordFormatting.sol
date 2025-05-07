// SPDX-License-Identifier: MIT
pragma solidity >=0.8.0;

contract DeleteKeywordFormatting {
    uint256 public myUint;
    address public myAddress;
    struct MyStruct { uint256 value; bool flag; }
    MyStruct public myStructInstance;
    mapping(uint256 => address) public myMap;
    uint256[] public myArray;

    function testDelete() public {
        // Delete state variable
        delete myUint;
        delete myAddress;

        // Delete struct instance
        delete myStructInstance;

        // Delete struct member (only for mappings/arrays within structs)
        // delete myStructInstance.value; // Invalid for non-mapping/array members

        // Delete mapping entry
        delete myMap[123];

        // Delete array element (sets to default value)
        if (myArray.length > 0) {
            delete myArray[0];
        }

        // Delete entire dynamic array
        delete myArray;

        // Delete with comments
        delete /* comment */ myUint /* comment */;
        delete myMap /* key */ [ /* another */ 123]; // comment // TODO(radomski): There should be a space between the comment and 123
        delete myArray [ 0 ] ; // end comment
    }
}
// Above input, below output
// SPDX-License-Identifier: MIT
pragma solidity >=0.8.0;

contract DeleteKeywordFormatting {
    uint256 public myUint;
    address public myAddress;
    struct MyStruct {
        uint256 value;
        bool flag;
    }
    MyStruct public myStructInstance;
    mapping(uint256 => address) public myMap;
    uint256[] public myArray;

    function testDelete() public {
        // Delete state variable
        delete myUint;
        delete myAddress;

        // Delete struct instance
        delete myStructInstance;

        // Delete struct member (only for mappings/arrays within structs)
        // delete myStructInstance.value; // Invalid for non-mapping/array members

        // Delete mapping entry
        delete myMap[123];

        // Delete array element (sets to default value)
        if(myArray.length > 0) {
            delete myArray[0];
        }

        // Delete entire dynamic array
        delete myArray;

        // Delete with comments
        delete /* comment */ myUint /* comment */;
        delete myMap /* key */[ /* another */123]; // comment // TODO(radomski): There should be a space between the comment and 123
        delete myArray[0]; // end comment
    }
}
