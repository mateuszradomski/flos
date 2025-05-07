// SPDX-License-Identifier: MIT
pragma solidity >=0.8.0;

struct ComplexValue {
    uint256 value;
    string name;
    uint256[] dataPoints;
}

contract ComplexMappings {
    // Mapping value is a struct containing an array
    mapping(address => ComplexValue) addressToComplexValue;

    // Mapping value is an array of structs
    mapping(uint256 => ComplexValue[]) uintToComplexValueArray;

    // Mapping value is another mapping
    mapping(bytes32 => mapping(address => uint256)) nestedMapping;

    // Mapping key is a struct (Invalid in Solidity, but testing parser/formatter tolerance if applicable)
    // mapping(ComplexValue => uint256) structKeyMapping; // This won't compile

    // Multi-dimensional mapping with complex types
    mapping(address => mapping(uint256 => ComplexValue[])) multiDimComplexMap;

    function testAccess(address addr, uint256 id, bytes32 key) public {
        // Access struct member within mapping value
        addressToComplexValue[addr].dataPoints.push(block.timestamp);

        // Access array element within mapping value
        uintToComplexValueArray[id].push(ComplexValue({
            value: 1, name: "test", dataPoints: new uint256[](0)
        }));
        uint256 firstVal = uintToComplexValueArray[id][0].value;

        // Access nested mapping
        nestedMapping[key][addr] = 123;

        // Access deeply nested mapping
        multiDimComplexMap[addr][id].push(ComplexValue({
            value: 2, name: "deep", dataPoints: new uint256[](1)
        }));
        string memory deepName = multiDimComplexMap[addr][id][0].name;
    }
}
// Above input, below output
// SPDX-License-Identifier: MIT
pragma solidity >=0.8.0;

struct ComplexValue {
    uint256 value;
    string name;
    uint256[] dataPoints;
}

contract ComplexMappings {
    // Mapping value is a struct containing an array
    mapping(address => ComplexValue) addressToComplexValue;

    // Mapping value is an array of structs
    mapping(uint256 => ComplexValue[]) uintToComplexValueArray;

    // Mapping value is another mapping
    mapping(bytes32 => mapping(address => uint256)) nestedMapping;

    // Mapping key is a struct (Invalid in Solidity, but testing parser/formatter tolerance if applicable)
    // mapping(ComplexValue => uint256) structKeyMapping; // This won't compile

    // Multi-dimensional mapping with complex types
    mapping(address => mapping(uint256 => ComplexValue[])) multiDimComplexMap;

    function testAccess(address addr, uint256 id, bytes32 key) public {
        // Access struct member within mapping value
        addressToComplexValue[addr].dataPoints.push(block.timestamp);

        // Access array element within mapping value
        uintToComplexValueArray[id].push(ComplexValue({ value: 1, name: "test", dataPoints: new uint256[](0) }));
        uint256 firstVal = uintToComplexValueArray[id][0].value;

        // Access nested mapping
        nestedMapping[key][addr] = 123;

        // Access deeply nested mapping
        multiDimComplexMap[addr][id].push(ComplexValue({ value: 2, name: "deep", dataPoints: new uint256[](1) }));
        string memory deepName = multiDimComplexMap[addr][id][0].name;
    }
}
