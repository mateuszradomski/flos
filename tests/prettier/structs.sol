struct KeyValuePair {
    address a0;
    bool a1;
    string a2;
    bytes a3;
    int a4;
    int256 a36;
    uint a37;
    uint8 a38;
    uint256 a69;
    bytes32 a101;

    abc.a a102;
    abc.b a103;
    MyContract.MyStruct a104;
    MyContract.MyStruct1.MyStruct2 a105;
      
    mapping(address => uint) a106;
    mapping(address => mapping(address => uint)) a107;
    mapping(uint=>x) a108;
    mapping(address => bytes32) a109;
    mapping(bytes32 => test_struct) a110;
    mapping (uint64 => mapping (bytes32 => uint)) a111;
    mapping (address a => bool b) a112;
    mapping(address=>bytes32) a113;
    mapping(address=>bytes32) a114;
    mapping (Foo => uint) a115;
    mapping (address =>  bool) a116;
    mapping (address a => bool) a117;
    mapping (address => bool b) a118;

    uint[] a119;
    uint[][] a120;
    uint[][][] a121;
    KeyValuePair[] a122;
    Locations a123;
    mapping(uint=>x)[] a124;
}
// Above input, below output
struct KeyValuePair {
    address a0;
    bool a1;
    string a2;
    bytes a3;
    int a4;
    int256 a36;
    uint a37;
    uint8 a38;
    uint256 a69;
    bytes32 a101;

    abc.a a102;
    abc.b a103;
    MyContract.MyStruct a104;
    MyContract.MyStruct1.MyStruct2 a105;

    mapping(address => uint) a106;
    mapping(address => mapping(address => uint)) a107;
    mapping(uint => x) a108;
    mapping(address => bytes32) a109;
    mapping(bytes32 => test_struct) a110;
    mapping(uint64 => mapping(bytes32 => uint)) a111;
    mapping(address a => bool b) a112;
    mapping(address => bytes32) a113;
    mapping(address => bytes32) a114;
    mapping(Foo => uint) a115;
    mapping(address => bool) a116;
    mapping(address a => bool) a117;
    mapping(address => bool b) a118;

    uint[] a119;
    uint[][] a120;
    uint[][][] a121;
    KeyValuePair[] a122;
    Locations a123;
    mapping(uint => x)[] a124;
}