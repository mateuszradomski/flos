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
