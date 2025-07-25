// SPDX-License-Identifier: MIT
pragma solidity 0.8.28;

contract HexLiteral {
  bytes8 hex1 = hex'DeadBeef';
  bytes8 hex2 = hex"DeadBeef";
}
// Above input, below output
// SPDX-License-Identifier: MIT
pragma solidity 0.8.28;

contract HexLiteral {
    bytes8 hex1 = hex"DeadBeef";
    bytes8 hex2 = hex"DeadBeef";
}
