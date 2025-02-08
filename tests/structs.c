UTEST(solfmt, structs) {
	Arena arena = arenaCreate(1024*1024, 4096, 32);
	TestData data = readTestInput(&arena, "tests/structs.sol");

	String result = stringTrim(format(&arena, data.input));

	showDifferences(result, data.output);
	ASSERT_TRUE(stringMatch(result, data.output));
    arenaDestroy(&arena);
}
