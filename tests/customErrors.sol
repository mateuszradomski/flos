pragma solidity ^0.8.4;

error
  TopLevelCustomError();
  error TopLevelCustomErrorWithArg(uint    x)  ;
error TopLevelCustomErrorArgWithoutName  (string);

contract CustomErrors {
  error
    ContractCustomError();
    error ContractCustomErrorWithArg(uint    x)  ;
  error ContractCustomErrorArgWithoutName  (string);
  error ERC1155TransferFailed(address sender, address recipient,
   uint256 balance, uint256 amount, uint256 tokenId,


    string reason);

  function throwCustomError() {
    revert
      FunctionCustomError();
    revert FunctionCustomErrorWithArg(  1  )  ;
    revert    FunctionCustomErrorArgWithoutName  (  "a reason");
  }
}
error ERC1155InsufficientAllowance(address sender, address spender, uint256 availableAllowance, uint256 requiredAllowance, uint256 tokenId);
// Above input, below output
pragma solidity ^0.8.4;

error TopLevelCustomError();
error TopLevelCustomErrorWithArg(uint x);
error TopLevelCustomErrorArgWithoutName(string);

contract CustomErrors {
    error ContractCustomError();
    error ContractCustomErrorWithArg(uint x);
    error ContractCustomErrorArgWithoutName(string);
    error ERC1155TransferFailed(
        address sender,
        address recipient,
        uint256 balance,
        uint256 amount,
        uint256 tokenId,
        string reason
    );

    function throwCustomError() {
        revert FunctionCustomError();
        revert FunctionCustomErrorWithArg(1);
        revert FunctionCustomErrorArgWithoutName("a reason");
    }
}
error ERC1155InsufficientAllowance(
    address sender,
    address spender,
    uint256 availableAllowance,
    uint256 requiredAllowance,
    uint256 tokenId
);