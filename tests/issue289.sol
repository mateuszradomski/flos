contract Issue289 {
  function f() {
	address[] storage proposalValidators
 = ethProposals[_blockNumberNumberVariable][_proposalIdNumberVariable].proposalValidators;
	address[] storage proposalValidators
 = ethProposals[_blockNumberNumberVariable][_proposalIdNumberVariable][someotherImportantNumberVariable + myMagicNumber].proposalValidators;
  }
}
// Above input, below output
contract Issue289 {
    function f() {
        address[] storage proposalValidators =
            ethProposals[_blockNumberNumberVariable][_proposalIdNumberVariable].proposalValidators;
        address[] storage proposalValidators =
            ethProposals[_blockNumberNumberVariable][_proposalIdNumberVariable][
                someotherImportantNumberVariable + myMagicNumber
            ].proposalValidators;
    }
}
