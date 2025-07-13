contract MemberAccess {
    function simpleMemberAccess() {
        obj.property.nestedProperty.deepProperty.veryDeepProperty.extremelyDeepProperty.finalProperty;

        obj.method().anotherMethod().thirdMethod().fourthMethod().fifthMethod().sixthMethod().finalMethod();

        obj.property.method().anotherProperty.anotherMethod().finalProperty.finalMethod();
    }

    function realWorldUseCases() {
        IERC20(tokenAddress).balanceOf(msg.sender).add(transferAmount).sub(feeAmount).mul(exchangeRate);

        contractInstance.methods.transfer(recipientAddress, amount).call().then(handleResponse);

        users[userId].profile.personalInfo.contactDetails.emailAddress.verified;

        SafeMath.add(firstValue, secondValue).mul(multiplier).div(divisor).sub(deduction);

        eventLogs[index].blockNumber.timestamp.transactionHash.gasUsed.cumulativeGasUsed;
    }

    function mixedComplexExpressions() {
        contracts[contractId].instances[instanceId].methods.getData().result.value.formatted;

        getContract(id).getInstances()[0].methods[methodName].call().result.data.parsed;

        getContract(id).getInstances()[0].methods[methodName].call().result.data.parsed();

        factory.createContract(params).initialize(settings).configure(options).deploy().address;

        (condition ? objectA : objectB).property.method().result.value.toString().length;
    }

    function assignmentsAndReturns() {
        finalResult = calculator.operations.addition.performCalculation(a, b).result.roundedValue;

        return tokenContract.balanceOf(userAddress).div(totalSupply).mul(100).toString();

        processData(dataSource.raw.filtered.processed.validated.final, settings.advanced.performance);

        if (user.permissions.admin.canModify.users.profiles.sensitiveData) {
            executeAdminFunction();
        }
    }

    function edgeCasesAndComplexScenarios() {
        veryLongObjectName.veryLongPropertyName.anotherVeryLongPropertyName.methodWithVeryLongName().resultProperty;

        contracts[getContractId(typeOf)].instances[0].methods.transfer(getRecipient(), getAmount()).call().result();

        contracts[getContractId(typeOf)].instances()[0].methods.transfer(getRecipient(), getAmount()).call().result();

        data[userId][sessionId].cache.temporary.values[index].processed.result.formatted.display.text;

        createComplexObject(param1, param2, param3).initialize().configure().validate().process().getResult();

        (dict[key] || defaultValue).property.method().transform().validate().serialize().compress();
    }

    function gasOptimizationPatterns() {
        UserProfile storage profile = users[msg.sender].profile.settings.preferences.notifications;

        bytes memory data = response.body.content.payload.data.encoded.compressed.final2;

        globalSettings.security.authentication.twoFactor.enabled.status.lastUpdated;
    }

    function memberAccess() {
        veryLongVariable.veryLongCall1(veryLongAttribute).veryLongCall2(veryLongAttribute).veryLongCall3(
            veryLongAttribute
        );

        veryLongVariable()().veryLongCall1(veryLongAttribute)()().veryLongCall2(veryLongAttribute)().veryLongCall3(
            veryLongAttribute
        );

        veryLongVariable[0][1].veryLongCall1[2](veryLongAttribute)[someIndex].veryLongCall2[3][4](veryLongAttribute)[5].veryLongCall3(veryLongAttribute);

        obj.property[2]()[2][3][3].a.a[4].method()[4].anotherProperty.anotherMethod().finalProperty.finalMethod();

        obj.property[2].a[4].method().finalMethod[2](1 + 2).a(1 + 2);

        obj.property().a[4].method().finalMethod(1 + 2);

        obj.property[2].a[4].method().finalMethod(1+2).a(1+2);
    }

    function _getBondTokenFinalFee() internal view returns (uint256) {
        (memoryFrame.sequencerBatchEnd, memoryFrame.sequencerBatchAcc) = inputDataFrame.sequencerInbox
            .proveInboxContainsMessage(sequencerBatchProof, assertion.afterState.inboxCount);

        return
            StoreInterface(finder.getImplementationAddress(OracleInterfaces.Store)).computeFinalFee(address(bondToken))
                .rawValue;
    }

    function correctNestingAtTheLastElement() {
        sharesMintedAsFees = _totalRewards
            .mul(rewardsDistribution.totalFee)
            .mul(_preTotalShares)
            .div(
                totalPooledEtherWithRewards
                    .mul(rewardsDistribution.precisionPoints)
                    .sub(_totalRewards.mul(rewardsDistribution.totalFee))
                );

        d = nA
            .mul(s)
            .div(AmplificationUtils.A_PRECISION)
            .add(dP.mul(numTokens))
            .mul(d)
            .div(
                nA
                    .sub(AmplificationUtils.A_PRECISION)
                    .mul(d)
                    .div(AmplificationUtils.A_PRECISION)
                    .add(numTokens.add(1).mul(dP))
            );

        return a0.add(a1.sub(a0).mul(block.timestamp.sub(t0)).div(t1.sub(t0)));

        require(signedStakePower >= currentTotalStake.mul(2).div(3).add(1), "2/3+1 non-majority!");

        if(approvals.add(disapprovals) >= LegacyToken(governanceToken).totalSupply().mul(quorumPercentage).div(100)) {}

        return
            (slots[slot].bidAmount == 0)
                ? _defaultSlotSetBid[slotSet].add(
                    _defaultSlotSetBid[slotSet]
                        .mul(_outbidding)
                        .div(
                        uint128(10000) // two decimal precision
                    )
                ) : 123;
    }
}
// Above input, below output
contract MemberAccess {
    function simpleMemberAccess() {
        obj.property.nestedProperty.deepProperty.veryDeepProperty.extremelyDeepProperty.finalProperty;

        obj
            .method()
            .anotherMethod()
            .thirdMethod()
            .fourthMethod()
            .fifthMethod()
            .sixthMethod()
            .finalMethod();

        obj.property
            .method()
            .anotherProperty.anotherMethod()
            .finalProperty.finalMethod();
    }

    function realWorldUseCases() {
        IERC20(tokenAddress)
            .balanceOf(msg.sender)
            .add(transferAmount)
            .sub(feeAmount)
            .mul(exchangeRate);

        contractInstance.methods
            .transfer(recipientAddress, amount)
            .call()
            .then(handleResponse);

        users[userId].profile.personalInfo.contactDetails.emailAddress.verified;

        SafeMath
            .add(firstValue, secondValue)
            .mul(multiplier)
            .div(divisor)
            .sub(deduction);

        eventLogs[index].blockNumber.timestamp.transactionHash.gasUsed.cumulativeGasUsed;
    }

    function mixedComplexExpressions() {
        contracts[contractId].instances[instanceId].methods.getData().result.value.formatted;

        getContract(id).getInstances()[0].methods[methodName].call().result.data.parsed;

        getContract(id)
            .getInstances()[0]
            .methods[methodName].call()
            .result.data.parsed();

        factory
            .createContract(params)
            .initialize(settings)
            .configure(options)
            .deploy().address;

        (condition ? objectA : objectB).property.method().result.value.toString().length;
    }

    function assignmentsAndReturns() {
        finalResult = calculator.operations.addition.performCalculation(a, b).result.roundedValue;

        return tokenContract
            .balanceOf(userAddress)
            .div(totalSupply)
            .mul(100)
            .toString();

        processData(dataSource.raw.filtered.processed.validated.final, settings.advanced.performance);

        if(user.permissions.admin.canModify.users.profiles.sensitiveData) {
            executeAdminFunction();
        }
    }

    function edgeCasesAndComplexScenarios() {
        veryLongObjectName.veryLongPropertyName.anotherVeryLongPropertyName.methodWithVeryLongName().resultProperty;

        contracts[getContractId(typeOf)].instances[0].methods
            .transfer(getRecipient(), getAmount())
            .call()
            .result();

        contracts[getContractId(typeOf)]
            .instances()[0]
            .methods.transfer(getRecipient(), getAmount())
            .call()
            .result();

        data[userId][sessionId].cache.temporary.values[index].processed.result.formatted.display.text;

        createComplexObject(param1, param2, param3)
            .initialize()
            .configure()
            .validate()
            .process()
            .getResult();

        (dict[key] || defaultValue).property
            .method()
            .transform()
            .validate()
            .serialize()
            .compress();
    }

    function gasOptimizationPatterns() {
        UserProfile storage profile = users[msg.sender].profile.settings.preferences.notifications;

        bytes memory data = response.body.content.payload.data.encoded.compressed.final2;

        globalSettings.security.authentication.twoFactor.enabled.status.lastUpdated;
    }

    function memberAccess() {
        veryLongVariable
            .veryLongCall1(veryLongAttribute)
            .veryLongCall2(veryLongAttribute)
            .veryLongCall3(veryLongAttribute);

        veryLongVariable()()
            .veryLongCall1(veryLongAttribute)()()
            .veryLongCall2(veryLongAttribute)()
            .veryLongCall3(veryLongAttribute);

        veryLongVariable[0][1].veryLongCall1[2](veryLongAttribute)[someIndex]
            .veryLongCall2[3][4](veryLongAttribute)[5]
            .veryLongCall3(veryLongAttribute);

        obj.property[2]()[2][3][3]
            .a.a[4].method()[4]
            .anotherProperty.anotherMethod()
            .finalProperty.finalMethod();

        obj.property[2].a[4]
            .method()
            .finalMethod[2](1 + 2)
            .a(1 + 2);

        obj
            .property()
            .a[4].method()
            .finalMethod(1 + 2);

        obj.property[2].a[4]
            .method()
            .finalMethod(1 + 2)
            .a(1 + 2);
    }

    function _getBondTokenFinalFee() internal view returns (uint256) {
        (memoryFrame.sequencerBatchEnd, memoryFrame.sequencerBatchAcc) =
            inputDataFrame.sequencerInbox.proveInboxContainsMessage(
                sequencerBatchProof,
                assertion.afterState.inboxCount
            );

        return
            StoreInterface(finder.getImplementationAddress(OracleInterfaces.Store)).computeFinalFee(
                address(bondToken)
            ).rawValue;
    }

    function correctNestingAtTheLastElement() {
        sharesMintedAsFees = _totalRewards
            .mul(rewardsDistribution.totalFee)
            .mul(_preTotalShares)
            .div(
                totalPooledEtherWithRewards
                    .mul(rewardsDistribution.precisionPoints)
                    .sub(_totalRewards.mul(rewardsDistribution.totalFee))
            );

        d = nA
            .mul(s)
            .div(AmplificationUtils.A_PRECISION)
            .add(dP.mul(numTokens))
            .mul(d)
            .div(
                nA
                    .sub(AmplificationUtils.A_PRECISION)
                    .mul(d)
                    .div(AmplificationUtils.A_PRECISION)
                    .add(numTokens.add(1).mul(dP))
            );

        return a0.add(
            a1
                .sub(a0)
                .mul(block.timestamp.sub(t0))
                .div(t1.sub(t0))
        );

        require(
            signedStakePower >=
                currentTotalStake
                    .mul(2)
                    .div(3)
                    .add(1),
            "2/3+1 non-majority!"
        );

        if(
            approvals.add(disapprovals) >=
            LegacyToken(governanceToken)
                .totalSupply()
                .mul(quorumPercentage)
                .div(100)
        ) { }

        return
            (slots[slot].bidAmount == 0)
                ? _defaultSlotSetBid[slotSet].add(
                    _defaultSlotSetBid[slotSet]
                        .mul(_outbidding)
                        .div(
                            uint128(10000) // two decimal precision
                        )
                )
                : 123;
    }
}
