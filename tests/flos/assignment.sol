contract Assignment {
    function getTimeBounds() internal view virtual returns (IBridge.TimeBounds memory) {
        (uint64 delayBlocks_, uint64 futureBlocks_, uint64 delaySeconds_, uint64 futureSeconds_) = maxTimeVariationInternal();
    }
}
// Above input, below output
contract Assignment {
    function getTimeBounds() internal view virtual returns (IBridge.TimeBounds memory) {
        (uint64 delayBlocks_, uint64 futureBlocks_, uint64 delaySeconds_, uint64 futureSeconds_) =
            maxTimeVariationInternal();
    }
}
