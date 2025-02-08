pragma solidity ^0.4.0;

contract SimpleStorage {
  string public name = "SimpleStorage";
  uint storedData;

  function set(uint x) public {
    storedData = x;
  }

  function get() public view returns (uint) {
    return storedData;
  }
}
// Above input, below output
pragma solidity ^0.4.0;

contract SimpleStorage {
    string public name = "SimpleStorage";
    uint storedData;

    function set(uint x) public {
        storedData = x;
    }

    function get() public view returns (uint) {
        return storedData;
    }
}
