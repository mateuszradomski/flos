pragma solidity ^0.4.24;

import "./MintableCommodity.sol";
import "../participant/ISupplier.sol";
import "../../node_modules/zeppelin-solidity/contracts//math/SafeMath.sol";

contract SplittableCommodity is MintableCommodity {
  using SafeMath for uint256;

  event Split(address indexed to, uint256 amount, uint64 parentId, address indexed operator, bytes operatorData);

  function split(uint _tokenId, address _to, uint256 _amount) public whenNotPaused {
    address supplierProxy = IContractRegistry(contractRegistry).getLatestProxyAddr("Supplier");
    require(
      msg.sender == IContractRegistry(contractRegistry).getLatestProxyAddr("FifoCrcMarket") ||
      ISupplier(supplierProxy).isAllowed(this, "IMintableCommodity"),
      "Splitting can only be done when both the ISplittableCommodity interface is enabled and is called by a supplier or the FifoCrcMarket is used."
    );

    commodities[_tokenId].value = commodities[_tokenId].value.sub(_amount);

    CommodityLib.Commodity memory _commodity = CommodityLib.Commodity({
        category: uint64(1),
        timeRegistered: uint64(now), // solium-disable-line
        parentId: _tokenId,
        value: _amount,
        locked: false,
        misc: commodities[_tokenId].misc
    });
    uint newCRCId = commodities.push(_commodity).sub(1);
    require(newCRCId <= 18446744073709551616, "You can only split a commodity if it is within a valid index range");

    if(msg.sender == IContractRegistry(contractRegistry).getLatestProxyAddr("FifoCrcMarket")) {
      _transfer(ownerOf(_tokenId), _to, newCRCId);
    } else {
      _transfer(msg.sender, _to, newCRCId);
    }

    callRecipient(
      msg.sender,
      0x0,
      _to,
      newCRCId,
      "",
      "",
      false
    );

    emit Split(
      _to,
      _amount,
      uint64(newCRCId),
      msg.sender,
      ""
    );
  }
}
// Above input, below output
pragma solidity ^0.4.24;

import "./MintableCommodity.sol";
import "../participant/ISupplier.sol";
import "../../node_modules/zeppelin-solidity/contracts//math/SafeMath.sol";

contract SplittableCommodity is MintableCommodity {
    using SafeMath for uint256;

    event Split(address indexed to, uint256 amount, uint64 parentId, address indexed operator, bytes operatorData);

    function split(uint _tokenId, address _to, uint256 _amount) public whenNotPaused {
        address supplierProxy = IContractRegistry(contractRegistry).getLatestProxyAddr("Supplier");
        require(
            msg.sender == IContractRegistry(contractRegistry).getLatestProxyAddr("FifoCrcMarket") ||
                ISupplier(supplierProxy).isAllowed(this, "IMintableCommodity"),
            "Splitting can only be done when both the ISplittableCommodity interface is enabled and is called by a supplier or the FifoCrcMarket is used."
        );

        commodities[_tokenId].value = commodities[_tokenId].value.sub(_amount);

        CommodityLib.Commodity memory _commodity = CommodityLib.Commodity({
            category: uint64(1),
            timeRegistered: uint64(now), // solium-disable-line
            parentId: _tokenId,
            value: _amount,
            locked: false,
            misc: commodities[_tokenId].misc
        });
        uint newCRCId = commodities.push(_commodity).sub(1);
        require(newCRCId <= 18446744073709551616, "You can only split a commodity if it is within a valid index range");

        if(msg.sender == IContractRegistry(contractRegistry).getLatestProxyAddr("FifoCrcMarket")) {
            _transfer(ownerOf(_tokenId), _to, newCRCId);
        } else {
            _transfer(msg.sender, _to, newCRCId);
        }

        callRecipient(msg.sender, 0x0, _to, newCRCId, "", "", false);

        emit Split(_to, _amount, uint64(newCRCId), msg.sender, "");
    }
}
