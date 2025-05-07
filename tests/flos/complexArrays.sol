// SPDX-License-Identifier: MIT
pragma solidity >=0.8.0;

struct Point { uint x; uint y; }

contract ComplexArrays {
    // Array of structs
    Point[] public points;

    // Array of mappings (Invalid in Solidity, test parser/formatter tolerance)
    // mapping(address => uint)[] mapArray;

    // Multi-dimensional array of simple types
    uint256[][] public matrix;

    // Multi-dimensional array of structs
    Point[][] public pointMatrix;

    // Array literal requiring breaks
    function setupPoints() public {
        points = [
            Point({x: 1, y: 2}), Point({x: 3, y: 4}), Point({x: 5, y: 6}),
            Point({x: 7, y: 8}), Point({x: 9, y: 10}), Point({x: 11, y: 12}),
            Point({x: 13, y: 14}), Point({x: 15, y: 16}), Point({x: 17, y: 18})
        ];

        matrix = [
            [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19],
            [20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36]
        ];
    }

    // Accessing complex arrays
    function accessArrays(uint i, uint j) public view returns (uint, uint) {
        uint x = points[i].x;
        uint val = matrix[i][j];
        // uint mapVal = mapArray[i][msg.sender]; // Invalid
        uint pointX = pointMatrix[i][j].x;
        return (x, val);
    }
}
// Above input, below output
// SPDX-License-Identifier: MIT
pragma solidity >=0.8.0;

struct Point {
    uint x;
    uint y;
}

contract ComplexArrays {
    // Array of structs
    Point[] public points;

    // Array of mappings (Invalid in Solidity, test parser/formatter tolerance)
    // mapping(address => uint)[] mapArray;

    // Multi-dimensional array of simple types
    uint256[][] public matrix;

    // Multi-dimensional array of structs
    Point[][] public pointMatrix;

    // Array literal requiring breaks
    function setupPoints() public {
        points = [
            Point({ x: 1, y: 2 }),
            Point({ x: 3, y: 4 }),
            Point({ x: 5, y: 6 }),
            Point({ x: 7, y: 8 }),
            Point({ x: 9, y: 10 }),
            Point({ x: 11, y: 12 }),
            Point({ x: 13, y: 14 }),
            Point({ x: 15, y: 16 }),
            Point({ x: 17, y: 18 })
        ];

        matrix = [
            [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19],
            [20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36]
        ];
    }

    // Accessing complex arrays
    function accessArrays(uint i, uint j) public view returns (uint, uint) {
        uint x = points[i].x;
        uint val = matrix[i][j];
        // uint mapVal = mapArray[i][msg.sender]; // Invalid
        uint pointX = pointMatrix[i][j].x;
        return (x, val);
    }
}
