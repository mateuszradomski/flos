contract TerneryExpression {
    function test() external {
        for(uint256 i; i < pathLength; i = i.uncheckedInc()) {
            currentHash = (_index % 2 == 0)
                ? efficientHash(currentHash, _path[i], _index)
                : efficientHash(_path[i], currentHash, _index);
            _index /= 2;
        }
    }
}
// Above input, below output
contract TerneryExpression {
    function test() external {
        for(uint256 i; i < pathLength; i = i.uncheckedInc()) {
            currentHash =
                (_index % 2 == 0)
                    ? efficientHash(currentHash, _path[i], _index)
                    : efficientHash(_path[i], currentHash, _index);
            _index /= 2;
        }
    }
}
