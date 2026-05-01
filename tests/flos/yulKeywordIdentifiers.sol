contract YulKeywordIdentifiers {
    function test() external {
        assembly {
            let layout := 1
            let at := layout
            let transient := at
        }
    }
}
// Above input, below output
contract YulKeywordIdentifiers {
    function test() external {
        assembly {
            let layout := 1
            let at := layout
            let transient := at
        }
    }
}
