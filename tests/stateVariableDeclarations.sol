contract Contract {
  bytes32 private constant DOMAIN_SEPARATOR_TYPEHASH = 0x035aff83d86937d35b32e04f0ddc6ff469290eef2f1b692d8a815c89404d4749;

    uint256 public number1;
    uint256 public number2;
    uint256 constant public BROADCAST_INDEX = type(uint256).max;
    bytes32 public constant BROADCAST_INDEX1 = bytes32(type(uint256).max);
}
// Above input, below output
contract Contract {
    bytes32 private constant DOMAIN_SEPARATOR_TYPEHASH =
        0x035aff83d86937d35b32e04f0ddc6ff469290eef2f1b692d8a815c89404d4749;

    uint256 public number1;
    uint256 public number2;
    uint256 public constant BROADCAST_INDEX = type(uint256).max;
    bytes32 public constant BROADCAST_INDEX1 = bytes32(type(uint256).max);
}
