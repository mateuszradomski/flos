contract ConstructorBreaking {
    // Constructor to initialize the Safe and baseModule instances
    constructor(
        Safe _safe,
        LivenessGuard _livenessGuard,
        uint256 _livenessInterval,
        uint256 _minOwners,
        uint256 _thresholdPercentage,
        address _fallbackOwner
    )
        FaultDisputeGame(
            _gameType,
            _absolutePrestate,
            _maxGameDepth,
            _splitDepth,
            _clockExtension,
            _maxClockDuration,
            _vm,
            _weth,
            _anchorStateRegistry,
            _l2ChainId
        )
    {
        SAFE = _safe;
        LIVENESS_GUARD = _livenessGuard;
        LIVENESS_INTERVAL = _livenessInterval;
        THRESHOLD_PERCENTAGE = _thresholdPercentage;
        FALLBACK_OWNER = _fallbackOwner;
        MIN_OWNERS = _minOwners;
        address[] memory owners = _safe.getOwners();
        require(_minOwners <= owners.length, "LivenessModule: minOwners must be less than the number of owners");
        require(_thresholdPercentage > 0, "LivenessModule: thresholdPercentage must be greater than 0");
        require(_thresholdPercentage <= 100, "LivenessModule: thresholdPercentage must be less than or equal to 100");
    }
}
// Above input, below output
contract ConstructorBreaking {
    // Constructor to initialize the Safe and baseModule instances
    constructor(
        Safe _safe,
        LivenessGuard _livenessGuard,
        uint256 _livenessInterval,
        uint256 _minOwners,
        uint256 _thresholdPercentage,
        address _fallbackOwner
    )
        FaultDisputeGame(
            _gameType,
            _absolutePrestate,
            _maxGameDepth,
            _splitDepth,
            _clockExtension,
            _maxClockDuration,
            _vm,
            _weth,
            _anchorStateRegistry,
            _l2ChainId
        )
    {
        SAFE = _safe;
        LIVENESS_GUARD = _livenessGuard;
        LIVENESS_INTERVAL = _livenessInterval;
        THRESHOLD_PERCENTAGE = _thresholdPercentage;
        FALLBACK_OWNER = _fallbackOwner;
        MIN_OWNERS = _minOwners;
        address[] memory owners = _safe.getOwners();
        require(_minOwners <= owners.length, "LivenessModule: minOwners must be less than the number of owners");
        require(_thresholdPercentage > 0, "LivenessModule: thresholdPercentage must be greater than 0");
        require(_thresholdPercentage <= 100, "LivenessModule: thresholdPercentage must be less than or equal to 100");
    }
}
