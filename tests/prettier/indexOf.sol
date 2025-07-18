contract IndexOf {

    address creator;

    function IndexOf() 
    {
        creator = msg.sender;
    }
    
    int whatwastheval = -10; // -2 = not yet tested, as separate from -1, tested but error
    
    function indexOf(string _a, string _b) returns (int) // _a = string to search, _b = string we want to find
    {
    	bytes memory a = bytes(_a);  
        bytes memory b = bytes(_b);
       
    	if(a.length < 1 || b.length < 1 || (b.length > a.length)) 
    	{
    		whatwastheval = -1;
    		return -1;
    	}
    	else if(a.length > (2**128 -1)) // since we have to be able to return -1 (if the char isn't found or input error), this function must return an "int" type with a max length of (2^128 - 1)
    	{
    		whatwastheval = -1;
    		return -1;									
    	}
    	else
    	{
    		uint subindex = 0;
    		for (uint i = 0; i < a.length; i ++)
    		{
    			if (a[i] == b[0]) // found the first char of b
    			{
    				subindex = 1;
    				while(subindex < b.length && (i + subindex) < a.length && a[i + subindex] == b[subindex]) // search until the chars don't match or until we reach the end of a or b
    				{
    					subindex++;
    				}	
    				if(subindex == b.length)
    				{	
    					whatwastheval = int(i);
    					return int(i);
    				}
    			}
    		}
    		whatwastheval = -1;
    		return -1;
    	}	
    }
    
    function whatWasTheVal() constant returns (int)
    {
    	return whatwastheval;
    }
    
    /**********
     Standard kill() function to recover funds 
     **********/
    
    function kill()
    { 
        if (msg.sender == creator)
        {
            suicide(creator);  // kills this contract and sends remaining funds back to creator
        }
    }
}
// Above input, below output
contract IndexOf {
    address creator;

    function IndexOf() {
        creator = msg.sender;
    }

    int whatwastheval = -10; // -2 = not yet tested, as separate from -1, tested but error

    function indexOf(string _a, string _b)
        returns (int) // _a = string to search, _b = string we want to find
    {
        bytes memory a = bytes(_a);
        bytes memory b = bytes(_b);

        if(a.length < 1 || b.length < 1 || (b.length > a.length)) {
            whatwastheval = -1;
            return -1;
        } else if(a.length > (2 ** 128 - 1)) // since we have to be able to return -1 (if the char isn't found or input error), this function must return an "int" type with a max length of (2^128 - 1)
        {
            whatwastheval = -1;
            return -1;
        } else {
            uint subindex = 0;
            for(uint i = 0; i < a.length; i++) {
                if(a[i] == b[0]) // found the first char of b
                {
                    subindex = 1;
                    while(subindex < b.length && (i + subindex) < a.length && a[i + subindex] == b[subindex]) // search until the chars don't match or until we reach the end of a or b
                    {
                        subindex++;
                    }
                    if(subindex == b.length) {
                        whatwastheval = int(i);
                        return int(i);
                    }
                }
            }
            whatwastheval = -1;
            return -1;
        }
    }

    function whatWasTheVal() constant returns (int) {
        return whatwastheval;
    }

    /**********
     Standard kill() function to recover funds 
     **********/

    function kill() {
        if(msg.sender == creator) {
            suicide(creator); // kills this contract and sends remaining funds back to creator
        }
    }
}
