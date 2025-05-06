pragma solidity >=0.4.0 <0.7.0;


contract A {
}
contract B {
}

contract C {
}

contract A {
    function spam() public pure {
        // ...
    }
    function ham() public pure {
        // ...
    }
}
// Above input, below output
pragma solidity >=0.4.0 <0.7.0;

contract A { }
contract B { }

contract C { }

contract A {
    function spam() public pure {
        // ...
    }
    function ham() public pure {
        // ...
    }
}
