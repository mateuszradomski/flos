// SPDX-License-Identifier: MIT
pragma solidity 0.8.28;

contract NumberLiteral {
  function numbers()public {
  2 ether;
  5 days;
  2.3e5;
  1000000e-2;
    .1;
  1_000_000;
  }
}
// Above input, below output
// SPDX-License-Identifier: MIT
pragma solidity 0.8.28;

contract NumberLiteral {
    function numbers() public {
        2 ether;
        5 days;
        2.3e5;
        1000000e-2;
        .1;
        1_000_000;
    }
}
