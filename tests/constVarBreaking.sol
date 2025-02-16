// Test case 1: Simple addition that exceeds 120 characters
uint256 constant test1 = 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + 11 + 12 + 13 + 14 + 15 + 16 + 17 + 18 + 19 + 20 + 21 + 22 + 23 + 24 + 25;

// Test case 2: Multiplication and addition with long variable names
uint256 constant veryLongVariableName1 = 100;
uint256 constant veryLongVariableName2 = 200;
uint256 constant test2 = veryLongVariableName1 * veryLongVariableName2 + 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + 11 + 12 + 13 + 14 + 15;

// Test case 3: Bitwise operations
uint256 constant test3 = (1 << 100) | (2 << 50) & (3 << 25) ^ (4 << 12) + (5 << 6) - (6 << 3) * (7 << 1) / (8 << 0);

// Test case 4: Complex arithmetic expression
uint256 constant test4 = (1000000000000000000 + 2000000000000000000) * (3000000000000000000 - 4000000000000000000) / (5000000000000000000 % 6000000000000000000);

// Test case 5:  Using constants defined outside the contract
uint256 constant EXTERNAL_CONSTANT = 1234567890123456789012345678901234567890123456789012345678901234;
uint256 constant test5 = EXTERNAL_CONSTANT + 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + 11 + 12 + 13 + 14 + 15 + 16 + 17 + 18 + 19 + 20;
uint256 constant test6 = helperFunction(helperFunction(helperFunction(helperFunction(helperFunction(1))))) + 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10;
// Above input, below output
// Test case 1: Simple addition that exceeds 120 characters
uint256 constant test1 =
    1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + 11 + 12 + 13 + 14 + 15 + 16 + 17 + 18 + 19 + 20 + 21 + 22 + 23 + 24 + 25;

// Test case 2: Multiplication and addition with long variable names
uint256 constant veryLongVariableName1 = 100;
uint256 constant veryLongVariableName2 = 200;
uint256 constant test2 =
    veryLongVariableName1 * veryLongVariableName2 + 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + 11 + 12 + 13 + 14 + 15;

// Test case 3: Bitwise operations
uint256 constant test3 =
    (1 << 100) | (2 << 50) & (3 << 25) ^ (4 << 12) + (5 << 6) - (6 << 3) * (7 << 1) / (8 << 0);

// Test case 4: Complex arithmetic expression
uint256 constant test4 =
    (1000000000000000000 + 2000000000000000000) *
    (3000000000000000000 - 4000000000000000000) /
    (5000000000000000000 % 6000000000000000000);

// Test case 5:  Using constants defined outside the contract
uint256 constant EXTERNAL_CONSTANT = 1234567890123456789012345678901234567890123456789012345678901234;
uint256 constant test5 =
    EXTERNAL_CONSTANT + 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + 11 + 12 + 13 + 14 + 15 + 16 + 17 + 18 + 19 + 20;
uint256 constant test6 =
    helperFunction(helperFunction(helperFunction(helperFunction(helperFunction(1))))) +
    1 +
    2 +
    3 +
    4 +
    5 +
    6 +
    7 +
    8 +
    9 +
    10;