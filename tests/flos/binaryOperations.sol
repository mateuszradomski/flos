contract BinaryOperations {
    function testBinaryOperations() public {
        require(RollupLib.assertionHash(assertion.beforeStateData.prevPrevAssertionHash, assertion.beforeState, assertion.beforeStateData.sequencerBatchAcc) == prevAssertionHash, "INVALID_BEFORE_STATE");
    }
}
// Above input, below output
contract BinaryOperations {
    function testBinaryOperations() public {
        require(
            RollupLib.assertionHash(
                assertion.beforeStateData.prevPrevAssertionHash,
                assertion.beforeState,
                assertion.beforeStateData.sequencerBatchAcc
            ) == prevAssertionHash,
            "INVALID_BEFORE_STATE"
        );
    }
}
