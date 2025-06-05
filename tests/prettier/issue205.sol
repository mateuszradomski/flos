contract Example {
    using SafeMath for uint256;

    struct BalanceState {
        uint256 balance;
    }

    mapping(address => mapping(address => BalanceState)) private balanceStatesWithLongerName;
    function example(address token, uint amount) public {
        // TODO(radomski): This should be different. The expression after the equals should be on it's own line
        balanceStatesWithLongerName[msg.sender][token].balance = balanceStatesWithLongerName[msg.sender][token].balance.sub(amount);
    }
}
// Above input, below output
contract Example {
    using SafeMath for uint256;

    struct BalanceState {
        uint256 balance;
    }

    mapping(address => mapping(address => BalanceState)) private balanceStatesWithLongerName;
    function example(address token, uint amount) public {
        // TODO(radomski): This should be different. The expression after the equals should be on it's own line
        balanceStatesWithLongerName[msg.sender][token].balance = balanceStatesWithLongerName[msg.sender][
            token
        ].balance.sub(amount);
    }
}
