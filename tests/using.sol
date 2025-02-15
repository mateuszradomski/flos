contract UsingDirectiveTest {
    using MyLibrary for address;
    using MyLibrary for uint256;
    using Library1 for uint256;
    using Library2 for uint256;
    using MyLibrary for mapping(address => uint256);
    using MyLibrary for uint256[];
    using MyLibrary for *;

    struct MyStruct {
        uint256 value;
    }

    using MyLibrary for MyStruct;
}

type Fixed18 is int256;
using Fixed18Lib for Fixed18 global;
using {plusOne, minusOne} for RestrictedNumber global;

using { add as + } for Fixed18 global;
using { add as +, sub as - } for Fixed18 global;
// Above input, below output
contract UsingDirectiveTest {
    using MyLibrary for address;
    using MyLibrary for uint256;
    using Library1 for uint256;
    using Library2 for uint256;
    using MyLibrary for mapping(address => uint256);
    using MyLibrary for uint256[];
    using MyLibrary for *;

    struct MyStruct {
        uint256 value;
    }

    using MyLibrary for MyStruct;
}

type Fixed18 is int256;
using Fixed18Lib for Fixed18 global;
using { plusOne, minusOne } for RestrictedNumber global;

using { add as + } for Fixed18 global;
using { add as +, sub as - } for Fixed18 global;
