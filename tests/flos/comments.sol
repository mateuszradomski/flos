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
}
