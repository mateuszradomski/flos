contract ModifierDefinitions {
  modifier noParams() {}
  modifier oneParam(uint a) {}
  modifier twoParams(uint a,uint b) {}
  modifier threeParams(uint a,uint b   ,uint c) {}
  modifier test() virtual;
  modifier test() virtual {}
}
// Above input, below output
contract ModifierDefinitions {
    modifier noParams() { }
    modifier oneParam(uint a) { }
    modifier twoParams(uint a, uint b) { }
    modifier threeParams(uint a, uint b, uint c) { }
    modifier test() virtual;
    modifier test() virtual { }
}
