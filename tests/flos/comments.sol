contract Comments {
    /*´:°•.°+.*•´.*:˚.°*.˚•´.°:°•.°•.*•´.*:˚.°*.˚•´.°:°•.°+.*•´.*:*/
    /*              SIMPLIFIED FIXED POINT OPERATIONS             */
    /*.•°:°.´+˚.*°.˚:*.´•*.+°.•°:´*.´•*.•°.•°:°.´:•˚°.*°.˚:*.´+°.•*/

    function fn() {
    revert (
            // comment3
        );

    revert (
        1
            // comment3
        );

    revert (
            // comment3
            1
        );
    }

    function startExitWithBurntTokens(bytes memory data) public returns (bytes memory) {
        require(
            msg.sender == address(topics.getField(2).toUint()), // from
            "Withdrawer and burn exit tx do not match"
        );
    }

    function forwardedBridgeRecoverFailedTransfer(
        uint256 /* _chainId */,
        bytes32 /* _assetInfo */,
        address /* _depositSender */,
        bytes calldata _chainData
    ) external payable override onlyBridgehub {}

    /** @dev Creates `amount` tokens and assigns them to `account`, increasing
     * the total supply.
     *
     * Emits a {Transfer} event with `from` set to the zero address.
     *
     * Requirements
     *
     * - `to` cannot be the zero address.
     */
}
// Above input, below output
contract Comments {
    /*´:°•.°+.*•´.*:˚.°*.˚•´.°:°•.°•.*•´.*:˚.°*.˚•´.°:°•.°+.*•´.*:*/
    /*              SIMPLIFIED FIXED POINT OPERATIONS             */
    /*.•°:°.´+˚.*°.˚:*.´•*.+°.•°:´*.´•*.•°.•°:°.´:•˚°.*°.˚:*.´+°.•*/

    function fn() {
        revert(
            // comment3
        );

        revert(
            1
            // comment3
        );

        revert(
            // comment3
            1
        );
    }

    function startExitWithBurntTokens(bytes memory data) public returns (bytes memory) {
        require(
            msg.sender == address(topics.getField(2).toUint()), // from
            "Withdrawer and burn exit tx do not match"
        );
    }

    function forwardedBridgeRecoverFailedTransfer(
        uint256 /* _chainId */,
        bytes32 /* _assetInfo */,
        address /* _depositSender */,
        bytes calldata _chainData
    ) external payable override onlyBridgehub { }

    /** @dev Creates `amount` tokens and assigns them to `account`, increasing
     * the total supply.
     *
     * Emits a {Transfer} event with `from` set to the zero address.
     *
     * Requirements
     *
     * - `to` cannot be the zero address.
     */
}
