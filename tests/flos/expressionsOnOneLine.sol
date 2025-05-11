contract ExpressionsOnOneLine {
    function test() public {
        uint256[] memory arr = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29];
        uint256 memory arr = 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + 11 + 12 + 13 + 14 + 15 + 16 + 17 + 18 + 19 + 20 + 21 + 22 + 23;
        uint256 flags = flag1 | flag2 | flag3 | flag4 | flag5 | flag6 | flag7 | flag8 | flag9 | flag10 | flag11 | flag12 | flag13;
        string memory longString = string.concat(str1, str2, str3, str4, str5, str6, str7, str8, str9, str10, str11, str12, str13, str14);

    }
}
// Above input, below output
contract ExpressionsOnOneLine {
    function test() public {
        uint256[] memory arr = [
            1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29
        ];
        uint256 memory arr =
            1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + 11 + 12 + 13 + 14 + 15 + 16 + 17 + 18 + 19 + 20 + 21 + 22 + 23;
        uint256 flags =
            flag1 | flag2 | flag3 | flag4 | flag5 | flag6 | flag7 | flag8 | flag9 | flag10 | flag11 | flag12 | flag13;
        string memory longString = string.concat(
            str1,
            str2,
            str3,
            str4,
            str5,
            str6,
            str7,
            str8,
            str9,
            str10,
            str11,
            str12,
            str13,
            str14
        );
    }
}
