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
}
