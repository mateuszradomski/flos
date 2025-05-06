pragma solidity >=0.6.2 <0.7.0;

contract NameValueExpression {
    function callFeed() public { feed.info{value: 10, gas: 800, test: true, many: variables(), to: check[0], forSplits: true, wantsToBeSplit: "forsure"}(); }
    D newD = new D{value: amount}(arg);
}
// Above input, below output
pragma solidity >=0.6.2 <0.7.0;

contract NameValueExpression {
    function callFeed() public {
        feed.info{
            value: 10,
            gas: 800,
            test: true,
            many: variables(),
            to: check[0],
            forSplits: true,
            wantsToBeSplit: "forsure"
        }();
    }
    D newD = new D{ value: amount }(arg);
}
