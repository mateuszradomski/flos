contract Example {
    using SafeMath for uint256;

    struct BalanceState {
        uint256 balance;
    }

    mapping(address => mapping(address => BalanceState)) private balanceStatesWithLongerName;
    function example(address token, uint amount) public {
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
        balanceStatesWithLongerName[msg.sender][token].balance = balanceStatesWithLongerName[msg.sender][token].balance
            .sub(amount);
    }
}
