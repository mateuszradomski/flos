contract Contract {
    struct State {
        mapping(uint64 blockId_mod_blockRingBufferSize => mapping(uint32 transitionId => TransitionState ts)) transitions;
    }
}
// Above input, below output
contract Contract {
    struct State {
        mapping(
            uint64 blockId_mod_blockRingBufferSize => mapping(uint32 transitionId => TransitionState ts)
        ) transitions;
    }
}
