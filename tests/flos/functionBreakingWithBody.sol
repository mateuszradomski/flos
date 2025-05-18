contract FunctionBreakingWithBody {
    function getCachedAddress(uint64 _chainId, bytes32 _name) internal pure override returns (bool found, address addr) { }
    function getCachedAddress(uint64 chainId, bytes32 name) internal pure override returns (bool found, address addr) { }
}
// Above input, below output
contract FunctionBreakingWithBody {
    function getCachedAddress(uint64 _chainId, bytes32 _name)
        internal
        pure
        override
        returns (bool found, address addr)
    { }
    function getCachedAddress(uint64 chainId, bytes32 name)
        internal
        pure
        override
        returns (bool found, address addr)
    { }
}
