contract IfStatements {
    function hi() public {
        if (simpleIf) return true;

        if (simpleIf) return (true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true);

        if (simpleIf) return true;
        else return false;

        if (simpleIf) return (true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true);
        else return false;

        if (simpleIf) return true;
        else return (false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false);

        if (simpleIf) return (true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true);
        else return (false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false);

        if (simpleIf) return true;
        else if (simpleElseIf) return (true,false);
        else return false;

        if (simpleIf) return (true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true);
        else if (simpleElseIf) return (true,false);
        else return false;

        if (simpleIf) return true;
        else if (simpleElseIf) return (true,false);
        else return (false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false);

        if (simpleIf) return (true,true,true,true,true,true,true,true,true,true,true,false,false,false,false,false,false);
        else if (simpleElseIf) return (true,false);
        else return (false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false);

        if (simpleIf) return true;
        else if (simpleElseIf) return (true,false,true,false,true,false,true,false,true,false,false,false,false,false,false,false);
        else return false;

        if (simpleIf) return (true,true,true,true,true,true,true,true,true,true,true,false,false,false,false,false,false);
        else if (simpleElseIf) return (true,false,true,false,true,false,true,false,true,false,false,false,false,false,false,false);
        else return false;

        if (simpleIf) return true;
        else if (simpleElseIf) return (true,false,true,false,true,false,true,false,true,false,false,false,false,false,false,false);
        else return (false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false);

        if (simpleIf) return (true,true,true,true,true,true,true,true,true,true,true,false,false,false,false,false,false);
        else if (simpleElseIf) return (true,false,true,false,true,false,true,false,true,false,false,false,false,false,false,false);
        else return (false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false);

        if (simpleIf) { return true; }

        if (simpleIf) { return true; }
        else return false;

        if (simpleIf) return true;
        else { return false; }

        if (simpleIf) { return true; }
        else { return false; }

        if (simpleIf) { return true; }
        else if (simpleElseIf) return (true,false);
        else return false;

        if (simpleIf) return true;
        else if (simpleElseIf) return (true,false);
        else { return false; }

        if (simpleIf) { return true; }
        else if (simpleElseIf) return (true,false);
        else { return false; }

        if (simpleIf) { return true; }
        else if (simpleElseIf) { return (true,false); }
        else return false;

        if (simpleIf) return true;
        else if (simpleElseIf) { return (true,false); }
        else { return false; }

        if (veryComplicatedComputation(variable1, variable2) == (400 + 500) * 1000) { return true; }

        if (simpleIf) { return true; }
        else if (veryComplicatedComputation(variable3, variable4) == (400 + 500) * 1000) { return (true,false); }
        else return false;

        if (veryComplicatedComputation(variable1, variable2) == (400 + 500) * 1000) { return true; }
        else if (simpleElseIf) { return (true,false); }
        else return false;

        if (veryComplicatedComputation(variable1, variable2) == (400 + 500) * 1000) { return true; }
        else if (veryComplicatedComputation(variable3, variable4) == (400 + 500) * 1000) { return (true,false); }
        else return false;

        if (simpleIf) if (simpleIfWithinIf) return true; else return false;

        if (simpleIf) if (simpleIfWithinIf) {return true;} else {return false;}

        if (simpleIf) {if (simpleIfWithinIf) {return true;} else {return false;}}

        if (simpleIf) {if (simpleIfWithinIf) return true;} else return false;

        if (simpleIf) {if (simpleIfWithinIf) {return true;}} else {return false;}

        if (simpleIf) if (simpleIfWithinIf) return true; else return false; else return false;

        if (simpleIf) if (simpleIfWithinIf) {return true;} else {return false;} else {return false;}

        if (simpleIf) {if (simpleIfWithinIf) {return true;} else {return false;}} else return false;
    }

         function _tryHexToUint(bytes1 char) private pure returns (bool, uint8) {
         uint8 c = uint8(char);
         unchecked {
             // Case 0-9
             if (47 < c && c < 58) {
                 return (true, c - 48);
            }
            // Case A-F
            else if (64 < c && c < 71) {
                 return (true, c - 55);
            }
            // Case a-f
            else if (96 < c && c < 103) {
                 return (true, c - 87);
            }
            // Else: not a hex char
            else {
                 return (false, 0);
             }
         }
     }

}
// Above input, below output
contract IfStatements {
    function hi() public {
        if(simpleIf) return true;

        if(simpleIf)
            return (
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true
            );

        if(simpleIf) return true;
        else return false;

        if(simpleIf)
            return (
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true
            );
        else return false;

        if(simpleIf) return true;
        else
            return (
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false
            );

        if(simpleIf)
            return (
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true
            );
        else
            return (
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false
            );

        if(simpleIf) return true;
        else if(simpleElseIf) return (true, false);
        else return false;

        if(simpleIf)
            return (
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true
            );
        else if(simpleElseIf) return (true, false);
        else return false;

        if(simpleIf) return true;
        else if(simpleElseIf) return (true, false);
        else
            return (
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false
            );

        if(simpleIf)
            return (
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                false,
                false,
                false,
                false,
                false,
                false
            );
        else if(simpleElseIf) return (true, false);
        else
            return (
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false
            );

        if(simpleIf) return true;
        else if(simpleElseIf)
            return (
                true,
                false,
                true,
                false,
                true,
                false,
                true,
                false,
                true,
                false,
                false,
                false,
                false,
                false,
                false,
                false
            );
        else return false;

        if(simpleIf)
            return (
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                false,
                false,
                false,
                false,
                false,
                false
            );
        else if(simpleElseIf)
            return (
                true,
                false,
                true,
                false,
                true,
                false,
                true,
                false,
                true,
                false,
                false,
                false,
                false,
                false,
                false,
                false
            );
        else return false;

        if(simpleIf) return true;
        else if(simpleElseIf)
            return (
                true,
                false,
                true,
                false,
                true,
                false,
                true,
                false,
                true,
                false,
                false,
                false,
                false,
                false,
                false,
                false
            );
        else
            return (
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false
            );

        if(simpleIf)
            return (
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                true,
                false,
                false,
                false,
                false,
                false,
                false
            );
        else if(simpleElseIf)
            return (
                true,
                false,
                true,
                false,
                true,
                false,
                true,
                false,
                true,
                false,
                false,
                false,
                false,
                false,
                false,
                false
            );
        else
            return (
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false
            );

        if(simpleIf) {
            return true;
        }

        if(simpleIf) {
            return true;
        } else return false;

        if(simpleIf) return true;
        else {
            return false;
        }

        if(simpleIf) {
            return true;
        } else {
            return false;
        }

        if(simpleIf) {
            return true;
        } else if(simpleElseIf) return (true, false);
        else return false;

        if(simpleIf) return true;
        else if(simpleElseIf) return (true, false);
        else {
            return false;
        }

        if(simpleIf) {
            return true;
        } else if(simpleElseIf) return (true, false);
        else {
            return false;
        }

        if(simpleIf) {
            return true;
        } else if(simpleElseIf) {
            return (true, false);
        } else return false;

        if(simpleIf) return true;
        else if(simpleElseIf) {
            return (true, false);
        } else {
            return false;
        }

        if(veryComplicatedComputation(variable1, variable2) == (400 + 500) * 1000) {
            return true;
        }

        if(simpleIf) {
            return true;
        } else if(veryComplicatedComputation(variable3, variable4) == (400 + 500) * 1000) {
            return (true, false);
        } else return false;

        if(veryComplicatedComputation(variable1, variable2) == (400 + 500) * 1000) {
            return true;
        } else if(simpleElseIf) {
            return (true, false);
        } else return false;

        if(veryComplicatedComputation(variable1, variable2) == (400 + 500) * 1000) {
            return true;
        } else if(veryComplicatedComputation(variable3, variable4) == (400 + 500) * 1000) {
            return (true, false);
        } else return false;

        if(simpleIf)
            if(simpleIfWithinIf) return true;
            else return false;

        if(simpleIf)
            if(simpleIfWithinIf) {
                return true;
            } else {
                return false;
            }

        if(simpleIf) {
            if(simpleIfWithinIf) {
                return true;
            } else {
                return false;
            }
        }

        if(simpleIf) {
            if(simpleIfWithinIf) return true;
        } else return false;

        if(simpleIf) {
            if(simpleIfWithinIf) {
                return true;
            }
        } else {
            return false;
        }

        if(simpleIf)
            if(simpleIfWithinIf) return true;
            else return false;
        else return false;

        if(simpleIf)
            if(simpleIfWithinIf) {
                return true;
            } else {
                return false;
            }
        else {
            return false;
        }

        if(simpleIf) {
            if(simpleIfWithinIf) {
                return true;
            } else {
                return false;
            }
        } else return false;
    }

    function _tryHexToUint(bytes1 char) private pure returns (bool, uint8) {
        uint8 c = uint8(char);
        unchecked {
            // Case 0-9
            if(47 < c && c < 58) {
                return (true, c - 48);
            }
            // Case A-F
            else if(64 < c && c < 71) {
                return (true, c - 55);
            }
            // Case a-f
            else if(96 < c && c < 103) {
                return (true, c - 87);
            }
            // Else: not a hex char
            else {
                return (false, 0);
            }
        }
    }
}
