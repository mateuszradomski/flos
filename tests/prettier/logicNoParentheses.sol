// SPDX-License-Identifier: MIT
pragma solidity 0.8.28;

contract LogicNoParentheses {
    function orOr(bool a, bool b, bool c) public pure returns (bool) {
        return a || b || c;
    }

    function orAnd(bool a, bool b, bool c) public pure returns (bool) {
        return a || b && c;
    }

    function andOr(bool a, bool b, bool c) public pure returns (bool) {
        return a && b || c;
    }

    function andAnd(bool a, bool b, bool c) public pure returns (bool) {
        return a && b && c;
    }
}
// Above input, below output
// SPDX-License-Identifier: MIT
pragma solidity 0.8.28;

contract LogicNoParentheses {
    function orOr(bool a, bool b, bool c) public pure returns (bool) {
        return a || b || c;
    }

    function orAnd(bool a, bool b, bool c) public pure returns (bool) {
        return a || (b && c);
    }

    function andOr(bool a, bool b, bool c) public pure returns (bool) {
        return (a && b) || c;
    }

    function andAnd(bool a, bool b, bool c) public pure returns (bool) {
        return a && b && c;
    }
}
