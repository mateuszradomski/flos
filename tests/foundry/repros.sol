// Repros of fmt issues

// https://github.com/foundry-rs/foundry/issues/4403
function errorIdentifier() {
    bytes memory error = bytes("");
    if (error.length > 0) {}
}

// https://github.com/foundry-rs/foundry/issues/7549
function one() external {
    this.other({
        data: abi.encodeCall(
            this.other,
            (
                "bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla"
            )
            )
    });
}

// https://github.com/foundry-rs/foundry/issues/3979
contract Format {
    bool public test;

    function testing(uint256 amount) public payable {
        if (
            // This is a comment
            msg.value == amount
        ) {
            test = true;
        } else {
            test = false;
        }

        if (
            // Another one
            block.timestamp >= amount
        ) {}
    }
}

contract IfElseTest {
     function setNumber(uint256 newNumber) public {
         number = newNumber;
         if (newNumber = 1) {
             number = 1;
         } else if (newNumber = 2) {
         //            number = 2;
         }
         else {
             newNumber = 3;
         }
     }
}

contract DbgFmtTest is Test {
    function test_argsList() public {
        // uint256 result1 = internalNoArgs({}); TODO(radomski): Crashes the parser
        result2 = add({a: 1, b: 2});
    }

    function add(uint256 a, uint256 b) internal pure returns (uint256) {
        return a + b;
    }

    function internalNoArgs() internal pure returns (uint256) {
        return 0;
    }
}
// Above input, below output
// Repros of fmt issues

// https://github.com/foundry-rs/foundry/issues/4403
function errorIdentifier() {
    bytes memory error = bytes("");
    if(error.length > 0) { }
}

// https://github.com/foundry-rs/foundry/issues/7549
function one() external {
    this.other({
        data: abi.encodeCall(
            this.other,
            ("bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla")
        )
    });
}

// https://github.com/foundry-rs/foundry/issues/3979
contract Format {
    bool public test;

    function testing(uint256 amount) public payable {
        if(
            // This is a comment
            msg.value == amount
        ) {
            test = true;
        } else {
            test = false;
        }

        if(
            // Another one
            block.timestamp >= amount
        ) { }
    }
}

contract IfElseTest {
    function setNumber(uint256 newNumber) public {
        number = newNumber;
        if(newNumber = 1) {
            number = 1;
        } else if(newNumber = 2) {
            //            number = 2;
        } else {
            newNumber = 3;
        }
    }
}

contract DbgFmtTest is Test {
    function test_argsList() public {
        // uint256 result1 = internalNoArgs({}); TODO(radomski): Crashes the parser
        result2 = add({ a: 1, b: 2 });
    }

    function add(uint256 a, uint256 b) internal pure returns (uint256) {
        return a + b;
    }

    function internalNoArgs() internal pure returns (uint256) {
        return 0;
    }
}
