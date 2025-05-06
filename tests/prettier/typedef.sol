type MyType is uint256;

contract MyContract {
    // Simple type alias within a contract
    type MyContractType is uint256;

    // Type alias with a comment
    // This is a comment for MyContractTypeWithComment
    type MyContractTypeWithComment is uint256;
}

library MyLibrary {
    // Type alias within a library
    type MyLibraryType is address;
}

interface MyInterface {
    // Type alias within an interface
    type MyInterfaceType is bytes32;
}

contract MultipleTypes {
    type Type1 is uint256;
    type Type2 is address;
    type Type3 is bytes32;
}

contract BaseContract {
    type BaseType is uint256;
}

contract VisibilityTest {
    // Solidity does not support private types, but testing formatter handling
    type PrivateType is uint256;
}
// Above input, below output
type MyType is uint256;

contract MyContract {
    // Simple type alias within a contract
    type MyContractType is uint256;

    // Type alias with a comment
    // This is a comment for MyContractTypeWithComment
    type MyContractTypeWithComment is uint256;
}

library MyLibrary {
    // Type alias within a library
    type MyLibraryType is address;
}

interface MyInterface {
    // Type alias within an interface
    type MyInterfaceType is bytes32;
}

contract MultipleTypes {
    type Type1 is uint256;
    type Type2 is address;
    type Type3 is bytes32;
}

contract BaseContract {
    type BaseType is uint256;
}

contract VisibilityTest {
    // Solidity does not support private types, but testing formatter handling
    type PrivateType is uint256;
}
