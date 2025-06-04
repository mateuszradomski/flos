contract FunctionPointerBreaking {
    mapping(uint256 l2BatchNumber => mapping(uint256 l2ToL1MessageNumber => bool isFinalized)) public isWithdrawalFinalized;

    /// @dev A mapping account => L1 token address => L2 deposit transaction hash => amount.
    /// @dev Used for saving the number of deposited funds, to claim them in case the deposit transaction will fail in ZKsync Era.
    mapping(address account => mapping(address l1Token => mapping(bytes32 depositL2TxHash => uint256 amount))) public depositAmount;

    function executeOneStep(
        ExecutionContext calldata execCtx,
        Machine calldata startMach,
        Module calldata startMod,
        Instruction calldata inst,
        bytes calldata proof
    ) external view override returns (Machine memory mach, Module memory mod) {
        mach = startMach;
        mod = startMod;

        uint16 opcode = inst.opcode;

        function(ExecutionContext calldata, Machine memory, Module memory, Instruction calldata, bytes calldata) internal view impl;

    }
}
// Above input, below output
contract FunctionPointerBreaking {
    mapping(
        uint256 l2BatchNumber => mapping(uint256 l2ToL1MessageNumber => bool isFinalized)
    ) public isWithdrawalFinalized;

    /// @dev A mapping account => L1 token address => L2 deposit transaction hash => amount.
    /// @dev Used for saving the number of deposited funds, to claim them in case the deposit transaction will fail in ZKsync Era.
    mapping(
        address account => mapping(address l1Token => mapping(bytes32 depositL2TxHash => uint256 amount))
    ) public depositAmount;

    function executeOneStep(
        ExecutionContext calldata execCtx,
        Machine calldata startMach,
        Module calldata startMod,
        Instruction calldata inst,
        bytes calldata proof
    ) external view override returns (Machine memory mach, Module memory mod) {
        mach = startMach;
        mod = startMod;

        uint16 opcode = inst.opcode;

        function(
            ExecutionContext calldata,
            Machine memory,
            Module memory,
            Instruction calldata,
            bytes calldata
        ) internal view impl;
    }
}
