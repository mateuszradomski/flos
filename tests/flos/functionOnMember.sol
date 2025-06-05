contract FunctionOnMember {
    function test() {
        (StoredBatchInfo[] memory batchesData, PriorityOpsBatchInfo[] memory priorityOpsData) = BatchDecoder.decodeAndCheckExecuteData(_executeData, _processFrom, _processTo);
    }
}
// Above input, below output
contract FunctionOnMember {
    function test() {
        (StoredBatchInfo[] memory batchesData, PriorityOpsBatchInfo[] memory priorityOpsData) =
            BatchDecoder.decodeAndCheckExecuteData(_executeData, _processFrom, _processTo);
    }
}
