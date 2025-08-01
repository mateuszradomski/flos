pragma solidity ^0.5.0;

contract demo {
    function hello() public view returns(bool,bool) {}
    function hello2() public view returns(bool) {}
    function hello3() public view returns(bool,bool,bool) {}

}

contract Tupples {
    function world(address payable _yo) public view {
        bool yo;
        yo = demo(_yo).hello2();
        (yo, ) = demo(_yo).hello();
        (, yo) = demo(_yo).hello();
        (yo, , yo) = demo(_yo).hello3();
        (yo, yo) = demo(_yo).hello();
        (bool success, bytes memory encodedDecimals) = address(asset_).staticcall(
            abi.encodeCall(IERC20Metadata.decimals, ())
        );
        (string memory holderId,
string memory containerId,
string memory parentContainerId,
bytes32 volumeId,
bytes32 subUnitId,
bytes32 requestId) = getParamsById(paramsId);
    }

    function emptyTuple() public {
        attacker.callSender(abi.encodeCall(this.callback, ()));
    }

}
// Above input, below output
pragma solidity ^0.5.0;

contract demo {
    function hello() public view returns (bool, bool) { }
    function hello2() public view returns (bool) { }
    function hello3() public view returns (bool, bool, bool) { }
}

contract Tupples {
    function world(address payable _yo) public view {
        bool yo;
        yo = demo(_yo).hello2();
        (yo,) = demo(_yo).hello();
        (, yo) = demo(_yo).hello();
        (yo,, yo) = demo(_yo).hello3();
        (yo, yo) = demo(_yo).hello();
        (bool success, bytes memory encodedDecimals) = address(asset_).staticcall(
            abi.encodeCall(IERC20Metadata.decimals, ())
        );
        (
            string memory holderId,
            string memory containerId,
            string memory parentContainerId,
            bytes32 volumeId,
            bytes32 subUnitId,
            bytes32 requestId
        ) = getParamsById(paramsId);
    }

    function emptyTuple() public {
        attacker.callSender(abi.encodeCall(this.callback, ()));
    }
}
