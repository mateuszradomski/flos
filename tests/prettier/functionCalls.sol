contract FunctionCalls {
    function foo() {
        address veryLongValidatorAddress = veryVeryVeryLongSignature.popLast20Bytes();
    }

    function foo() {
      Voter you = Voter(1, true);

      Voter me = Voter({
          weight: 2,
          voted: abstain()
      });

      Voter airbnb = Voter({
        weight: 2,
        voted: true
      });
    }
    function verify(ForwardRequest calldata req, bytes calldata signature) public view returns (bool) {
    address signer =
        _hashTypedDataV1(
            keccak256(abi.encode(TYPEHASH, req.from, req.to, req.value, req.gas, req.nonce, keccak256(req.data)))
        )
            ._hashTypedDataV2(
            keccak256(abi.encode(TYPEHASH, req.from, req.to, req.value, req.gas, req.nonce, keccak256(req.data)))
        )._hashTypedDataV3(
            keccak256(abi.encode(TYPEHASH, req.from, req.to, req.value, req.gas, req.nonce, keccak256(req.data)))
        )                .recover(signature);
    signer =
        _hashTypedDataV1(
            keccak256(abi.encode(TYPEHASH, req.from, req.to, req.value, req.gas, req.nonce, keccak256(req.data)))
        )
            ._hashTypedDataV2(
            keccak256(abi.encode(TYPEHASH, req.from, req.to, req.value, req.gas, req.nonce, keccak256(req.data)))
        )._hashTypedDataV3(
            keccak256(abi.encode(TYPEHASH, req.from, req.to, req.value, req.gas, req.nonce, keccak256(req.data)))
        )                .recover(signature);
    return _nonces[req.from] == req.nonce && signer == req.from;
}
}
// Above input, below output
contract FunctionCalls {
    function foo() {
        address veryLongValidatorAddress = veryVeryVeryLongSignature.popLast20Bytes();
    }

    function foo() {
        Voter you = Voter(1, true);

        Voter me = Voter({ weight: 2, voted: abstain() });

        Voter airbnb = Voter({ weight: 2, voted: true });
    }
    function verify(ForwardRequest calldata req, bytes calldata signature) public view returns (bool) {
        address signer = _hashTypedDataV1(
            keccak256(abi.encode(TYPEHASH, req.from, req.to, req.value, req.gas, req.nonce, keccak256(req.data)))
        )
            ._hashTypedDataV2(
                keccak256(abi.encode(TYPEHASH, req.from, req.to, req.value, req.gas, req.nonce, keccak256(req.data)))
            )
            ._hashTypedDataV3(
                keccak256(abi.encode(TYPEHASH, req.from, req.to, req.value, req.gas, req.nonce, keccak256(req.data)))
            )
            .recover(signature);
        signer = _hashTypedDataV1(
            keccak256(abi.encode(TYPEHASH, req.from, req.to, req.value, req.gas, req.nonce, keccak256(req.data)))
        )
            ._hashTypedDataV2(
                keccak256(abi.encode(TYPEHASH, req.from, req.to, req.value, req.gas, req.nonce, keccak256(req.data)))
            )
            ._hashTypedDataV3(
                keccak256(abi.encode(TYPEHASH, req.from, req.to, req.value, req.gas, req.nonce, keccak256(req.data)))
            )
            .recover(signature);
        return _nonces[req.from] == req.nonce && signer == req.from;
    }
}
