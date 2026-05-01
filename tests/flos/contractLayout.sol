contract Layout is Base layout at 123 {}

contract LayoutBreaking is SomeLongerBaseContract layout at 0x3b86514c5c56b21f08d8e56ab090292e07c2483b3e667a2a45849dcb71368600 {}

contract LayoutBreaking is SomeLongerBaseContract layout at 0x3b86514c5c56b21f08d8e56ab090292e07c2483b3e667a2a45849dcb71368600 + 0x3b86514c5c56b21f08d8e56ab090292e07c2483b3e667a2a45849dcb71368600 {}

contract DifferentOrder layout at 151 is Base1, Base2 { }
// Above input, below output
contract Layout is Base layout at 123 { }

contract LayoutBreaking is SomeLongerBaseContract
  layout at 0x3b86514c5c56b21f08d8e56ab090292e07c2483b3e667a2a45849dcb71368600
{ }

contract LayoutBreaking is SomeLongerBaseContract
  layout at 0x3b86514c5c56b21f08d8e56ab090292e07c2483b3e667a2a45849dcb71368600 +
      0x3b86514c5c56b21f08d8e56ab090292e07c2483b3e667a2a45849dcb71368600
{ }

contract DifferentOrder is Base1, Base2 layout at 151 { }
