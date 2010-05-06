#include "cactusGlobalsPrivate.h"

static NetDisk *netDisk;
static Net *net;
static Net *nestedNet;
static End *end1;
static End *end2;
static End *end3;
static End *end4;
static End *nestedEnd1;
static End *nestedEnd2;
static Group *group, *group2;

static void cactusGroupTestTeardown() {
	if(netDisk != NULL) {
		netDisk_destruct(netDisk);
		testCommon_deleteTemporaryNetDisk();
		netDisk = NULL;
	}
}

static void cactusGroupTestSetup() {
	cactusGroupTestTeardown();
	netDisk = netDisk_construct(testCommon_getTemporaryNetDisk());
	net = net_construct(netDisk);
	nestedNet = net_construct(netDisk);
	end1 = end_construct(0, net);
	end2 = end_construct(0, net);
	end3 = end_construct(0, net);
	nestedEnd1 = end_copyConstruct(end1, nestedNet);
	nestedEnd2 = end_copyConstruct(end2, nestedNet);
	group = group_construct(net, nestedNet);
	group2 = group_construct2(net);
	end4 = end_construct(0, net);
}

void testGroup_construct(CuTest* testCase) {
	cactusGroupTestSetup();
	CuAssertTrue(testCase, group != NULL);
	cactusGroupTestTeardown();
}

void testGroup_updateContainedEnds(CuTest* testCase) {
	cactusGroupTestSetup();
	end_copyConstruct(end3, nestedNet);
	CuAssertTrue(testCase, group_getEndNumber(group) == 2);
	group_updateContainedEnds(group);
	CuAssertTrue(testCase, group_getEndNumber(group) == 3);
	CuAssertTrue(testCase, group_getEnd(group, end_getName(end1)) == end1);
	CuAssertTrue(testCase, group_getEnd(group, end_getName(end2)) == end2);
	CuAssertTrue(testCase, group_getEnd(group, end_getName(end3)) == end3);
	cactusGroupTestTeardown();
}

void testGroup_makeNonLeaf(CuTest *testCase) {
	cactusGroupTestSetup();
	CuAssertTrue(testCase, group_isLeaf(group2));
	end_setGroup(end4, group2);
	group_makeNestedNet(group2);
	CuAssertTrue(testCase, !group_isLeaf(group2));
	Net *nestedNet = group_getNestedNet(group2);
	CuAssertTrue(testCase, nestedNet != NULL);
	CuAssertTrue(testCase, !net_builtBlocks(net));
	CuAssertTrue(testCase, !net_builtTrees(net));
	CuAssertTrue(testCase, !net_builtFaces(net));
	CuAssertTrue(testCase, net_getName(nestedNet) == group_getName(group2));
	CuAssertTrue(testCase, net_getParentGroup(nestedNet) == group2);
	CuAssertTrue(testCase, net_getEndNumber(nestedNet) == 1);
	End *nestedEnd = net_getFirstEnd(nestedNet);
	CuAssertTrue(testCase, end_getName(end4) == end_getName(nestedEnd));
	CuAssertTrue(testCase, end_getGroup(nestedEnd) != NULL);
	CuAssertTrue(testCase, net_getGroupNumber(nestedNet) == 1);
	CuAssertTrue(testCase, net_isTerminal(nestedNet));
	cactusGroupTestTeardown();
}

void testGroup_addEnd(CuTest *testCase) {
	cactusGroupTestSetup();
	CuAssertTrue(testCase, group_getEndNumber(group2) == 0);
	end_setGroup(end4, group2);
	CuAssertTrue(testCase, group_getEndNumber(group2) == 1);
	CuAssertTrue(testCase, end_getGroup(end4) == group2);
	CuAssertTrue(testCase, group_getEnd(group2, end_getName(end4)) == end4);
	cactusGroupTestTeardown();
}

void testGroup_isLeaf(CuTest *testCase) {
	cactusGroupTestSetup();
	CuAssertTrue(testCase, !group_isLeaf(group));
	CuAssertTrue(testCase, group_isLeaf(group2));
	cactusGroupTestTeardown();
}

static Chain *setupChain() {
	Chain *chain = chain_construct(net);
	end_setGroup(end1, group2);
	end_setGroup(end2, group2);
	link_construct(end1, end2, group2, chain);
	return chain;
}

void testGroup_getLink(CuTest *testCase) {
	cactusGroupTestSetup();
	CuAssertTrue(testCase, group_getLink(group2) == NULL);
	Chain *chain = setupChain();
	CuAssertTrue(testCase, group_getLink(group2) == chain_getLink(chain, 0));
	chain_destruct(chain);
	CuAssertTrue(testCase, group_getLink(group2) == NULL);
	cactusGroupTestTeardown();
}

void testGroup_isTangle(CuTest *testCase) {
	cactusGroupTestSetup();
	CuAssertTrue(testCase, group_isTangle(group2));
	Chain *chain = setupChain();
	CuAssertTrue(testCase, !group_isTangle(group2));
	chain_destruct(chain);
	CuAssertTrue(testCase, group_isTangle(group2));
	cactusGroupTestTeardown();
}

void testGroup_isLink(CuTest *testCase) {
	cactusGroupTestSetup();
	CuAssertTrue(testCase, !group_isLink(group2));
	Chain *chain = setupChain();
	CuAssertTrue(testCase, group_isLink(group2));
	chain_destruct(chain);
	CuAssertTrue(testCase, !group_isLink(group2));
	cactusGroupTestTeardown();
}

void testGroup_getNet(CuTest* testCase) {
	cactusGroupTestSetup();
	CuAssertTrue(testCase, group_getNet(group) == net);
	cactusGroupTestTeardown();
}

void testGroup_getNestedNetName(CuTest* testCase) {
	cactusGroupTestSetup();
	CuAssertTrue(testCase, group_getName(group) == net_getName(nestedNet));
	cactusGroupTestTeardown();
}

void testGroup_getNestedNet(CuTest* testCase) {
	cactusGroupTestSetup();
	CuAssertTrue(testCase, group_getNestedNet(group) == nestedNet);
	cactusGroupTestTeardown();
}

void testGroup_getChain(CuTest* testCase) {
	cactusGroupTestSetup();
	CuAssertTrue(testCase, group_getLink(group) == NULL);
	Chain *chain = chain_construct(net);
	link_construct(end1, end2, group, chain);
	CuAssertTrue(testCase, group_getLink(group) == chain_getLink(chain, 0));
	cactusGroupTestTeardown();
}

void testGroup_getEnd(CuTest* testCase) {
	cactusGroupTestSetup();
	CuAssertTrue(testCase, group_getEnd(group, end_getName(end1)) == end1);
	CuAssertTrue(testCase, group_getEnd(group, end_getName(end2)) == end2);
	cactusGroupTestTeardown();
}

void testGroup_getEndNumber(CuTest* testCase) {
	cactusGroupTestSetup();
	CuAssertTrue(testCase, group_getEndNumber(group) == 2);
	cactusGroupTestTeardown();
}

void testGroup_endIterator(CuTest* testCase) {
	cactusGroupTestSetup();
	Group_EndIterator *iterator = group_getEndIterator(group);

	CuAssertTrue(testCase, group_getNextEnd(iterator) == end1);
	CuAssertTrue(testCase, group_getNextEnd(iterator) == end2);
	CuAssertTrue(testCase, group_getNextEnd(iterator) == NULL);

	Group_EndIterator *iterator2 = group_copyEndIterator(iterator);

	CuAssertTrue(testCase, group_getPreviousEnd(iterator) == end2);
	CuAssertTrue(testCase, group_getPreviousEnd(iterator) == end1);
	CuAssertTrue(testCase, group_getPreviousEnd(iterator) == NULL);

	group_destructEndIterator(iterator);

	CuAssertTrue(testCase, group_getPreviousEnd(iterator2) == end2);
	CuAssertTrue(testCase, group_getPreviousEnd(iterator2) == end1);
	CuAssertTrue(testCase, group_getPreviousEnd(iterator2) == NULL);

	group_destructEndIterator(iterator2);

	cactusGroupTestTeardown();
}

void testGroup_getTotalBaseLength(CuTest *testCase) {
	cactusGroupTestSetup();

	CuAssertTrue(testCase, group_getTotalBaseLength(group) == 0);
	//cap_construct

	cactusGroupTestTeardown();
}

/*void testGroup_mergeGroups(CuTest *testCase) {
	cactusGroupTestSetup();

	CuAssertTrue(testCase, group_getTotalBaseLength(group) == 0);
	//cap_construct
	Net *parentNet = net_construct(netDisk);
	End *end1 = end_construct(0, parentNet);
	End *end2 = end_construct(0, parentNet);
	End *end3 = end_construct(0, parentNet);
	End *end4 = end_construct(0, parentNet);

	Group *parentGroup1 = group_construct2(parentNet);
	Group *parentGroup2 = group_construct2(parentNet);
	Group *parentGroup3 = group_construct2(parentNet);
	Group *parentGroup4 = group_construct2(parentNet);

	end_setGroup(end1, parentGroup1);
	end_setGroup(end2, parentGroup1);
	end_setGroup(end3, parentGroup2);
	end_setGroup(end4, parentGroup3);

	group_makeNonLeaf(parentGroup3);
	group_makeNonLeaf(parentGroup4);

	Group *mergedGroup1 = group_mergeGroups(parentGroup1, parentGroup2); //merge two leaf nets
	Group *mergedGroup2 = group_mergeGroups(mergedGroup1, parentGroup3); //merge a leaf and non-leaf net
	Group *finalGroup = group_mergeGroups(parentGroup4, mergedGroup2); //merge two non-leaf nets

	CuAssertTrue(testCase, net_getGroupNumber(parentNet) == 1);
	CuAssertTrue(testCase, net_getFirstGroup(parentNet) == finalGroup);
	CuAssertTrue(testCase, net_getEndNumber(parentNet) == 4);
	CuAssertTrue(testCase, end_getGroup(end1) == finalGroup);
	CuAssertTrue(testCase, end_getGroup(end2) == finalGroup);
	CuAssertTrue(testCase, end_getGroup(end3) == finalGroup);
	CuAssertTrue(testCase, end_getGroup(end4) == finalGroup);

	CuAssertTrue(testCase, !group_isLeaf(finalGroup));
	Net *nestedNet = group_getNestedNet(finalGroup);
	CuAssertTrue(testCase, net_getEndNumber(nestedNet) == 4);
	CuAssertTrue(testCase, net_getEnd(nestedNet, end_getName(end1)) != NULL);
	CuAssertTrue(testCase, net_getEnd(nestedNet, end_getName(end2)) != NULL);
	CuAssertTrue(testCase, net_getEnd(nestedNet, end_getName(end3)) != NULL);
	CuAssertTrue(testCase, net_getEnd(nestedNet, end_getName(end4)) != NULL);

	cactusGroupTestTeardown();
}*/

void testGroup_serialisation(CuTest* testCase) {
	cactusGroupTestSetup();
	int32_t i;
	Name name = group_getName(group);
	void *vA = binaryRepresentation_makeBinaryRepresentation(group,
			(void (*)(void *, void (*)(const void *, size_t, size_t)))group_writeBinaryRepresentation, &i);
	CuAssertTrue(testCase, i > 0);
	group_destruct(group);
	void *vA2 = vA;
	group = group_loadFromBinaryRepresentation(&vA2, net);
	free(vA);
	CuAssertTrue(testCase, group_getName(group) == name);
	CuAssertTrue(testCase, group_getNet(group) == net);
	CuAssertTrue(testCase, group_getNestedNet(group) == nestedNet);
	Group_EndIterator *iterator = group_getEndIterator(group);
	CuAssertTrue(testCase, group_getNextEnd(iterator) == end1);
	CuAssertTrue(testCase, group_getNextEnd(iterator) == end2);
	CuAssertTrue(testCase, group_getNextEnd(iterator) == NULL);
	group_destructEndIterator(iterator);

	cactusGroupTestTeardown();
}

CuSuite* cactusGroupTestSuite(void) {
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, testGroup_updateContainedEnds);
	SUITE_ADD_TEST(suite, testGroup_addEnd);
	SUITE_ADD_TEST(suite, testGroup_isLeaf);
	SUITE_ADD_TEST(suite, testGroup_makeNonLeaf);
	SUITE_ADD_TEST(suite, testGroup_getLink);
	SUITE_ADD_TEST(suite, testGroup_isLink);
	SUITE_ADD_TEST(suite, testGroup_isTangle);
	SUITE_ADD_TEST(suite, testGroup_getNet);
	SUITE_ADD_TEST(suite, testGroup_getNestedNetName);
	SUITE_ADD_TEST(suite, testGroup_getNestedNet);
	SUITE_ADD_TEST(suite, testGroup_getChain);
	SUITE_ADD_TEST(suite, testGroup_getEnd);
	SUITE_ADD_TEST(suite, testGroup_getEndNumber);
	SUITE_ADD_TEST(suite, testGroup_endIterator);
	SUITE_ADD_TEST(suite, testGroup_getTotalBaseLength);
	//SUITE_ADD_TEST(suite, testGroup_mergeGroups);
	SUITE_ADD_TEST(suite, testGroup_serialisation);
	SUITE_ADD_TEST(suite, testGroup_construct);
	return suite;
}
