contract Contract {
    function test() { unchecked { a += 1; }

        unchecked {
        a += 1;
        }
    2 + 2;

unchecked { a += 1;
        }
    unchecked {}

    1 + 1;


    }
}
// Above input, below output
contract Contract {
    function test() {
        unchecked {
            a += 1;
        }

        unchecked {
            a += 1;
        }
        2 + 2;

        unchecked {
            a += 1;
        }
        unchecked { }

        1 + 1;
    }
}
