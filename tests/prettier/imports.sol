import "SomeFile.sol";
import "SomeFile.sol" as SomeOtherFile;
import * as SomeSymbol from "AnotherFile.sol";
import {symbol1 as alias1, symbol2} from "File.sol";
import {symbol1 as alias1, symbol2 as alias2, symbol3 as alias3, symbol4} from "File2.sol";
import {symbol1 as alias1, symbol2 as alias2, symbol3 as alias3, symbol4, symbol5, symbol6, symbol7, symbol8} from "File2.sol";
import/*two*/{/*three*/symbol1/*four*/}/*five*/ from /*six*/"File2.sol"/*seven*/;
// Above input, below output
import "SomeFile.sol";
import "SomeFile.sol" as SomeOtherFile;
import "AnotherFile.sol" as SomeSymbol;
import { symbol1 as alias1, symbol2 } from "File.sol";
import { symbol1 as alias1, symbol2 as alias2, symbol3 as alias3, symbol4 } from "File2.sol";
import {
    symbol1 as alias1,
    symbol2 as alias2,
    symbol3 as alias3,
    symbol4,
    symbol5,
    symbol6,
    symbol7,
    symbol8
} from "File2.sol";
import /*two*/ { /*three*/ symbol1 /*four*/ } /*five*/ from /*six*/ "File2.sol" /*seven*/;
