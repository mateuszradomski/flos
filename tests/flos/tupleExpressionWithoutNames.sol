contract TupleExpressionWithoutNames {
    modifier onlyValidatorContract(uint256 validatorId) {
        address _contract;
        (, , , , , , _contract, ) = IStakeManagerLocal(
            registry.getStakeManagerAddress()
        )
           .validators(validatorId);
        require(_contract == msg.sender,
        "Invalid sender, not validator");
        _;
    }
}
// Above input, below output
contract TupleExpressionWithoutNames {
    modifier onlyValidatorContract(uint256 validatorId) {
        address _contract;
        (,,,,,, _contract,) = IStakeManagerLocal(registry.getStakeManagerAddress()).validators(validatorId);
        require(_contract == msg.sender, "Invalid sender, not validator");
        _;
    }
}
