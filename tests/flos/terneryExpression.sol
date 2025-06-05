contract TerneryExpression {
    function test() external {
        for(uint256 i; i < pathLength; i = i.uncheckedInc()) {
            currentHash = (_index % 2 == 0)
                ? efficientHash(currentHash, _path[i], _index)
                : efficientHash(_path[i], currentHash, _index);
            _index /= 2;
        }
    }

    function _findTcb(bytes memory der, uint256 oidPtr)
        private
        pure
        returns (bool success, uint256 pcesvn, uint256[] memory cpusvns)
    {
        for(uint256 i; i < SGX_TCB_CPUSVN_SIZE + 1; ++i) {
            uint16 svnValue = svnValueBytes.length < 2 ? uint16(bytes2(svnValueBytes)) / 256 : uint16(bytes2(svnValueBytes));
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

    function _findTcb(bytes memory der, uint256 oidPtr)
        private
        pure
        returns (bool success, uint256 pcesvn, uint256[] memory cpusvns)
    {
        for(uint256 i; i < SGX_TCB_CPUSVN_SIZE + 1; ++i) {
            uint16 svnValue =
                svnValueBytes.length < 2
                    ? uint16(bytes2(svnValueBytes)) / 256
                    : uint16(bytes2(svnValueBytes));
        }
    }
}
