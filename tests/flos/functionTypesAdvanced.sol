// SPDX-License-Identifier: MIT
pragma solidity >=0.8.0;

struct MyStruct { uint256 val; }

contract FunctionTypesAdvanced {
    // State variable function type
    function(uint256, address) external view returns (bool, bytes memory) public stateFunc;

    // Struct containing function type
    struct Callback {
        function(MyStruct calldata, uint256[] memory) internal pure returns (string memory) func;
        uint256 id;
    }
    Callback public myCallback;

    // Mapping value is function type
    mapping(uint256 => function(bytes calldata) external returns (address)) public idToFunc;

    // Complex function type signature
    function(mapping(address => uint256) storage, Callback memory) external returns (function() internal pure) public complexFuncType;

    event FuncCalled(uint256 id);

    function setCallback(function(MyStruct calldata, uint256[] memory) internal pure returns (string memory) _func) public {
       myCallback = Callback({
           func: _func,
           id: 1
       });
    }

    function executeCallback(MyStruct calldata data, uint256[] memory arr) public pure returns (string memory) {
        // Cannot call internal function type directly from external context usually,
        // This setup implies it would be called internally elsewhere.
        // return myCallback.func(data, arr); // Simulating internal call structure
        return "simulation";
    }

    function assignAndCall(uint256 id, address target) public returns (address) {
        idToFunc[id] = FunctionTypesAdvanced(target).externalFuncExample;
        return idToFunc[id](hex""); // Example call
    }

    function externalFuncExample(bytes calldata) external pure returns (address) {
        return address(this);
    }
}
// Above input, below output
// SPDX-License-Identifier: MIT
pragma solidity >=0.8.0;

struct MyStruct {
    uint256 val;
}

contract FunctionTypesAdvanced {
    // State variable function type
    function(uint256, address) external view returns (bool, bytes memory) public stateFunc;

    // Struct containing function type
    struct Callback {
        function(MyStruct calldata, uint256[] memory) internal pure returns (string memory) func;
        uint256 id;
    }
    Callback public myCallback;

    // Mapping value is function type
    mapping(uint256 => function(bytes calldata) external returns (address)) public idToFunc;

    // Complex function type signature
    function(mapping(address => uint256) storage, Callback memory) external returns (
        function() internal pure
    ) public complexFuncType;

    event FuncCalled(uint256 id);

    function setCallback(function(MyStruct calldata, uint256[] memory) internal pure returns (string memory) _func)
        public
    {
        myCallback = Callback({ func: _func, id: 1 });
    }

    function executeCallback(MyStruct calldata data, uint256[] memory arr) public pure returns (string memory) {
        // Cannot call internal function type directly from external context usually,
        // This setup implies it would be called internally elsewhere.
        // return myCallback.func(data, arr); // Simulating internal call structure
        return "simulation";
    }

    function assignAndCall(uint256 id, address target) public returns (address) {
        idToFunc[id] = FunctionTypesAdvanced(target).externalFuncExample;
        return idToFunc[id](hex""); // Example call
    }

    function externalFuncExample(bytes calldata) external pure returns (address) {
        return address(this);
    }
}
