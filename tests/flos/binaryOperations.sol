contract BinaryOperations {
    function testBinaryOperations() public {
        require(RollupLib.assertionHash(assertion.beforeStateData.prevPrevAssertionHash, assertion.beforeState, assertion.beforeStateData.sequencerBatchAcc) == prevAssertionHash, "INVALID_BEFORE_STATE");

        require(_configHash == configHash(configData.wasmModuleRoot, configData.requiredStake, configData.challengeManager, configData.confirmPeriodBlocks, configData.nextInboxPosition), 'CONFIG_HASH_MISMATCH');

        require(
            (isTopLevelCall && _initialized < 1) ||
            (!AddressUpgradeable.isContract(address(this)) && _initialized == 1),
            "Initializable: contract is already initialized"
        );
    }

    function _setResourceConfig(IResourceMetering.ResourceConfig memory _config) internal {
        // Min base fee must be less than or equal to max base fee.
        require(
            _config.minimumBaseFee <= _config.maximumBaseFee,
            "SystemConfig: min base fee must be less than max base"
        );

        // No precision loss when computing target resource limit.
        require(
            ((_config.maxResourceLimit / _config.elasticityMultiplier) * _config.elasticityMultiplier) ==
            _config.maxResourceLimit,
            "SystemConfig: precision loss with target resource limit"
        );
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

        require(
            _configHash ==
                configHash(
                    configData.wasmModuleRoot,
                    configData.requiredStake,
                    configData.challengeManager,
                    configData.confirmPeriodBlocks,
                    configData.nextInboxPosition
                ),
            "CONFIG_HASH_MISMATCH"
        );

        require(
            (isTopLevelCall && _initialized < 1) ||
                (!AddressUpgradeable.isContract(address(this)) && _initialized == 1),
            "Initializable: contract is already initialized"
        );
    }

    function _setResourceConfig(IResourceMetering.ResourceConfig memory _config) internal {
        // Min base fee must be less than or equal to max base fee.
        require(
            _config.minimumBaseFee <= _config.maximumBaseFee,
            "SystemConfig: min base fee must be less than max base"
        );

        // No precision loss when computing target resource limit.
        require(
            ((_config.maxResourceLimit / _config.elasticityMultiplier) * _config.elasticityMultiplier) ==
                _config.maxResourceLimit,
            "SystemConfig: precision loss with target resource limit"
        );
    }
}
