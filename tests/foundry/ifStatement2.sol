contract IfStatement {

    function test() external {
        bool anotherLongCondition;

        if (condition && ((condition || anotherLongCondition)
        )
        ) execute();
    }
}
// Above input, below output
contract IfStatement {
    function test() external {
        bool anotherLongCondition;

        if(condition && ((condition || anotherLongCondition))) execute();
    }
}
