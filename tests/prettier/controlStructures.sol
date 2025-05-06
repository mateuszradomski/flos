pragma solidity >=0.4.0 <0.7.0;


contract ControlStructures
{
    struct Bank
    {
        address owner;
        uint balance;
    }

    function () {
        if (true)
        {
            doSomething();
        }

        while(true){
            break;
        }

        for (uint a; a < 10; a++) {
            doSomethingElse();}

        if (x < 10)
            x += 1;

        if (x < 10)
            someArray.push(Coin({
                name: 'spam',
                value: 42
            }));

        if (x < 3) {
            x += 1;
        } else
        if (x > 7) {
            x -= 1;
        }
        else {
            x = 5;
        }

        if (x < 3)
            x += 1;
        else
            x -= 1;
    }
}
// Above input, below output
pragma solidity >=0.4.0 <0.7.0;

contract ControlStructures {
    struct Bank {
        address owner;
        uint balance;
    }

    function() {
        if(true) {
            doSomething();
        }

        while(true) {
            break;
        }

        for(uint a; a < 10; a++) {
            doSomethingElse();
        }

        if(x < 10) x += 1;

        if(x < 10) someArray.push(Coin({ name: "spam", value: 42 }));

        if(x < 3) {
            x += 1;
        } else if(x > 7) {
            x -= 1;
        } else {
            x = 5;
        }

        if(x < 3) x += 1;
        else x -= 1;
    }
}
