#include "sonLib.h"
#include "cactus.h"
#include "stPinchGraphs.h"
#include "stCactusGraphs.h"
#include "stCaf.h"

///////////////////////////////////////////////////////////////////////////
// Core functions for melting
///////////////////////////////////////////////////////////////////////////

static bool isThreadEnd(stPinchBlock *pinchBlock) {
    stPinchSegment *pinchSegment = stPinchBlock_getFirst(pinchBlock);
    bool threadEnd = pinchSegment != NULL && (stPinchSegment_get3Prime(pinchSegment) == NULL || stPinchSegment_get5Prime(pinchSegment)
            == NULL);
    if (threadEnd) {
        assert(stPinchBlock_getLength(pinchBlock) == 1);
    }
    return threadEnd;
} //Adding dummy comment

static void processChain(stCactusEdgeEnd *cactusEdgeEnd, void(*edgeEndFn)(stPinchBlock *, void *), void *extraArg, bool recursive) {
    while (1) {
        stPinchEnd *pinchEnd = stCactusEdgeEnd_getObject(cactusEdgeEnd);
        assert(pinchEnd != NULL);
        stPinchBlock *pinchBlock = stPinchEnd_getBlock(pinchEnd);
        assert(pinchBlock != NULL);
        edgeEndFn(pinchBlock, extraArg);
        assert(stCactusEdgeEnd_getOtherEdgeEnd(stCactusEdgeEnd_getOtherEdgeEnd(cactusEdgeEnd)) == cactusEdgeEnd);
        cactusEdgeEnd = stCactusEdgeEnd_getOtherEdgeEnd(cactusEdgeEnd);
        if (stCactusEdgeEnd_isChainEnd(cactusEdgeEnd)) {
            break;
        }
        if (recursive) {
            stCactusNode *node = stCactusEdgeEnd_getNode(cactusEdgeEnd);
            stCactusNodeEdgeEndIt it = stCactusNode_getEdgeEndIt(node);
            stCactusEdgeEnd *cactusEdgeEnd2;
            while ((cactusEdgeEnd2 = stCactusNodeEdgeEndIt_getNext(&it)) != NULL) {
                if (stCactusEdgeEnd_isChainEnd(cactusEdgeEnd2) && stCactusEdgeEnd_getLinkOrientation(cactusEdgeEnd2)) {
                    processChain(cactusEdgeEnd2, edgeEndFn, extraArg, 1);
                }
            }
        }
        assert(stCactusEdgeEnd_getLink(stCactusEdgeEnd_getLink(cactusEdgeEnd)) == cactusEdgeEnd);
        cactusEdgeEnd = stCactusEdgeEnd_getLink(cactusEdgeEnd);
    }
}

static void addBlock(stPinchBlock *block, void *extraArg) {
    if (!isThreadEnd(block)) {
        stList_append(extraArg, block);
    }
}

static void addChainBlocksToBlocksToDelete(stCactusEdgeEnd *cactusEdgeEnd, stList *blocksToDelete) {
    processChain(cactusEdgeEnd, addBlock, blocksToDelete, 0);
}

static void addLength(stPinchBlock *block, void *extraArg) {
    *((int64_t *) extraArg) += stPinchBlock_getLength(block);
}

static int64_t getChainLength(stCactusEdgeEnd *cactusEdgeEnd) {
    int64_t length = 0;
    processChain(cactusEdgeEnd, addLength, &length, 0);
    return length;
}

static stList *stCaf_getBlocksInChainsLessThanGivenLength(stCactusGraph *cactusGraph, int64_t minimumChainLength) {
    stList *blocksToDelete = stList_construct3(0, (void(*)(void *)) stPinchBlock_destruct);
    stCactusGraphNodeIt *nodeIt = stCactusGraphNodeIterator_construct(cactusGraph);
    stCactusNode *cactusNode;
    while ((cactusNode = stCactusGraphNodeIterator_getNext(nodeIt)) != NULL) {
        stCactusNodeEdgeEndIt cactusEdgeEndIt = stCactusNode_getEdgeEndIt(cactusNode);
        stCactusEdgeEnd *cactusEdgeEnd;
        while ((cactusEdgeEnd = stCactusNodeEdgeEndIt_getNext(&cactusEdgeEndIt)) != NULL) {
            if (stCactusEdgeEnd_isChainEnd(cactusEdgeEnd) && stCactusEdgeEnd_getLinkOrientation(cactusEdgeEnd)) {
                if (getChainLength(cactusEdgeEnd) < minimumChainLength) {
                    addChainBlocksToBlocksToDelete(cactusEdgeEnd, blocksToDelete);
                }
            }
        }
    }
    stCactusGraphNodeIterator_destruct(nodeIt);
    return blocksToDelete;
}

static void trimAlignments(stPinchThreadSet *threadSet, int64_t blockEndTrim) {
    stPinchThreadSetBlockIt blockIt = stPinchThreadSet_getBlockIt(threadSet);
    stPinchBlock *block = stPinchThreadSetBlockIt_getNext(&blockIt);
    while (block != NULL) {
        stPinchBlock *block2 = stPinchThreadSetBlockIt_getNext(&blockIt);
        if (!isThreadEnd(block)) {
            stPinchBlock_trim(block, blockEndTrim);
        }
        block = block2;
    }
}

static void filterAlignments(stPinchThreadSet *threadSet, bool(*blockFilterFn)(stPinchBlock *)) {
    stPinchThreadSetBlockIt blockIt = stPinchThreadSet_getBlockIt(threadSet);
    stPinchBlock *block = stPinchThreadSetBlockIt_getNext(&blockIt);
    while (block != NULL) {
        stPinchBlock *block2 = stPinchThreadSetBlockIt_getNext(&blockIt);
        if (!isThreadEnd(block) && blockFilterFn(block)) {
            stPinchBlock_destruct(block);
        }
        block = block2;
    }
}

void stCaf_melt(Flower *flower, stPinchThreadSet *threadSet, bool blockFilterfn(stPinchBlock *), int64_t blockEndTrim,
        int64_t minimumChainLength, bool breakChainsAtReverseTandems, int64_t maximumMedianSpacingBetweenLinkedEnds) {
    //First trim
    if (blockEndTrim > 0) {
        trimAlignments(threadSet, blockEndTrim);
    }

    //Then filter blocks
    if (blockFilterfn != NULL) {
        filterAlignments(threadSet, blockFilterfn);
    }

    //Now apply the minimum chain length filter
    if (minimumChainLength > 1) {
        stCactusNode *startCactusNode;
        stList *deadEndComponent;
        stCactusGraph *cactusGraph = stCaf_getCactusGraphForThreadSet(flower, threadSet, &startCactusNode, &deadEndComponent, 0, INT64_MAX,
                0.0, breakChainsAtReverseTandems, maximumMedianSpacingBetweenLinkedEnds);
        stList *blocksToDelete = stCaf_getBlocksInChainsLessThanGivenLength(cactusGraph, minimumChainLength);
        //Cleanup cactus
        stCactusGraph_destruct(cactusGraph);
        stList_destruct(blocksToDelete); //This will destroy the blocks
    }
    //Now heal up the trivial boundaries
    stCaf_joinTrivialBoundaries(threadSet);
}

Flower *debugFlower;

static char *getEndStr(stPinchEnd *end) {
    stPinchBlockIt it = stPinchBlock_getSegmentIterator(end->block);
    stList *segmentNames = stList_construct3(0, free);
    stPinchSegment *segment;
    while ((segment = stPinchBlockIt_getNext(&it)) != NULL) {
        Cap *cap = flower_getCap(debugFlower, stPinchSegment_getName(segment));
        const char *header = sequence_getHeader(cap_getSequence(cap));
        const char *genome = event_getHeader(cap_getEvent(cap));
        stList_append(segmentNames, stString_print("%s.%s|%" PRIi64 "-%" PRIi64, genome, header, stPinchSegment_getStart(segment), stPinchSegment_getStart(segment) + stPinchSegment_getLength(segment)));
    }
    char *ret = stString_join2(",", segmentNames);
    stList_destruct(segmentNames);
    return ret;
}

// Determine whether the child chain is recoverable given the parent
// chain (i.e. will bar phase be expected to pick it back up if the
// parent chain sticks around?).
//
// A chain is called recoverable if its ends link to different parent
// chain ends ("anchor ends") and its ends do not link to each other.
static bool chainIsRecoverableGivenParent(stCactusEdgeEnd *childChainEnd, stCactusEdgeEnd *incidentParentChainEdgeEnd) {
    stPinchEnd *anchorEnd1 = stCactusEdgeEnd_getObject(incidentParentChainEdgeEnd);
    stPinchEnd *anchorEnd2 = stCactusEdgeEnd_getObject(stCactusEdgeEnd_getLink(incidentParentChainEdgeEnd));
    stPinchEnd *childEnd1 = stCactusEdgeEnd_getObject(childChainEnd);
    stPinchEnd *childEnd2 = stCactusEdgeEnd_getObject(stCactusEdgeEnd_getLink(childChainEnd));

    stSet *connectedEnds1 = stPinchEnd_getConnectedPinchEnds(childEnd1);
    stSet *connectedEnds2 = stPinchEnd_getConnectedPinchEnds(childEnd2);

    printf("c1: %s c2: %s\np1: %s p2: %s\n", getEndStr(childEnd1), getEndStr(childEnd2), getEndStr(anchorEnd1), getEndStr(anchorEnd2));

    printf("c1->p1 %p c1->p2 %p c2->p1 %p c2->p2 %p c1->c2 %p c2->c1 %p\n",
           stSet_search(connectedEnds1, anchorEnd1),
           stSet_search(connectedEnds1, anchorEnd2),
           stSet_search(connectedEnds2, anchorEnd1),
           stSet_search(connectedEnds2, anchorEnd2),
           stSet_search(connectedEnds1, childEnd2),
           stSet_search(connectedEnds2, childEnd1));

    bool recoverable = false;
    if (stSet_search(connectedEnds1, anchorEnd1) && stSet_search(connectedEnds2, anchorEnd2)) {
        recoverable = !(stSet_search(connectedEnds2, anchorEnd1) || stSet_search(connectedEnds1, anchorEnd2));
    } else if (stSet_search(connectedEnds1, anchorEnd2) && stSet_search(connectedEnds2, anchorEnd1)) {
        recoverable = !(stSet_search(connectedEnds2, anchorEnd2) || stSet_search(connectedEnds1, anchorEnd1));
    }

    // Check for a duplication (link connecting the two child chain ends).
    if (stSet_search(connectedEnds1, childEnd2)) {
        assert(stSet_search(connectedEnds2, childEnd1));
        recoverable = false;
    }
    stSet_destruct(connectedEnds1);
    stSet_destruct(connectedEnds2);
    return recoverable;
}

// FIXME: remove
static int64_t numNodesVisited = 0;
static int64_t numChainsVisited = 0;
static int64_t numRecoverableChains = 0;

// For a given cactus node, recurse through all nodes below it and
// find recoverable chains below them. Then find recoverable chains
// below the current node given its parent chain.
static void getRecoverableChains_R(stCactusNode *cactusNode, stCactusEdgeEnd *parentChain, int64_t maxRecoverableLength, stList *recoverableChains) {
    numNodesVisited++;
    stCactusNodeEdgeEndIt cactusEdgeEndIt = stCactusNode_getEdgeEndIt(cactusNode);
    stCactusEdgeEnd *cactusEdgeEnd;
    while ((cactusEdgeEnd = stCactusNodeEdgeEndIt_getNext(&cactusEdgeEndIt)) != NULL) {
        if ((parentChain == NULL
             || (cactusEdgeEnd != parentChain
                 && cactusEdgeEnd != stCactusEdgeEnd_getLink(parentChain)))
            && stCactusEdgeEnd_getLinkOrientation(cactusEdgeEnd)
            && stCactusEdgeEnd_getOtherNode(cactusEdgeEnd) != cactusNode) {
            // Found a new chain below this node.
            assert(stCactusEdgeEnd_isChainEnd(cactusEdgeEnd));
            getRecoverableChains_R(stCactusEdgeEnd_getOtherNode(cactusEdgeEnd),
                                   stCactusEdgeEnd_getOtherEdgeEnd(cactusEdgeEnd),
                                   maxRecoverableLength, recoverableChains);
        }
    }

    if (parentChain != NULL) {
        // Visit the next node on this chain (unless it's where we started).
        stCactusEdgeEnd *nextEdgeEnd = stCactusEdgeEnd_getOtherEdgeEnd(stCactusEdgeEnd_getLink(parentChain));
        if (!stCactusEdgeEnd_isChainEnd(nextEdgeEnd)) {
            getRecoverableChains_R(stCactusEdgeEnd_getNode(nextEdgeEnd), nextEdgeEnd, maxRecoverableLength, recoverableChains);
        }
    }

    cactusEdgeEndIt = stCactusNode_getEdgeEndIt(cactusNode);
    while ((cactusEdgeEnd = stCactusNodeEdgeEndIt_getNext(&cactusEdgeEndIt)) != NULL) {
        if (stCactusEdgeEnd_isChainEnd(cactusEdgeEnd) && stCactusEdgeEnd_getLinkOrientation(cactusEdgeEnd)) {
            if (parentChain != NULL && chainIsRecoverableGivenParent(cactusEdgeEnd, parentChain)) {
                numRecoverableChains++;
                stList_append(recoverableChains, cactusEdgeEnd);
            } else if (parentChain == NULL) {
                stPinchEnd *childEnd1 = stCactusEdgeEnd_getObject(cactusEdgeEnd);
                stPinchEnd *childEnd2 = stCactusEdgeEnd_getObject(stCactusEdgeEnd_getLink(cactusEdgeEnd));

                stSet *connectedEnds1 = stPinchEnd_getConnectedPinchEnds(childEnd1);
                stSet *connectedEnds2 = stPinchEnd_getConnectedPinchEnds(childEnd2);

                stSet *sharedEnds = stSet_getIntersection(connectedEnds1, connectedEnds2);

                if (stSet_size(sharedEnds) == 0 && stSet_size(connectedEnds1) == 1 && stSet_size(connectedEnds2) == 1) {
                    printf("recoverable\n");
                    stList_append(recoverableChains, cactusEdgeEnd);
                } else {
                    printf("UNRECOVERABLE %" PRIi64 " %" PRIi64 "% " PRIi64 "\n", stSet_size(sharedEnds), stSet_size(connectedEnds1), stSet_size(connectedEnds2));
                }

                printf("c1: %s c2: %s\n", getEndStr(childEnd1), getEndStr(childEnd2));
                printf("ends connected to c1:\n");
                stSetIterator *it = stSet_getIterator(connectedEnds1);
                stPinchEnd *end;
                while ((end = stSet_getNext(it)) != NULL) {
                    printf("%s\n", getEndStr(end));
                }
                stSet_destructIterator(it);
                printf("ends connected to c2:\n");
                it = stSet_getIterator(connectedEnds2);
                while ((end = stSet_getNext(it)) != NULL) {
                    printf("%s\n", getEndStr(end));
                }
                stSet_destructIterator(it);

                stSet_destruct(sharedEnds);
                stSet_destruct(connectedEnds1);
                stSet_destruct(connectedEnds2);
            }
            numChainsVisited++;
        }
    }
}

static stList *getRecoverableChains(stCactusNode *startCactusNode, int64_t maxRecoverableLength) {
    stList *ret = stList_construct();
    getRecoverableChains_R(startCactusNode, NULL, maxRecoverableLength, ret);
    printf("Visited %" PRIi64 " cactus nodes while getting recoverable chains\n", numNodesVisited);
    printf("Found %" PRIi64 " / %" PRIi64 " recoverable chains\n", numRecoverableChains, numChainsVisited);
    return ret;
}

void stCaf_meltRecoverableChains(Flower *flower, stPinchThreadSet *threadSet, bool breakChainsAtReverseTandems, int64_t maximumMedianSpacingBetweenLinkedEnds, int64_t maxRecoverableLength) {
    debugFlower = flower;
    stCactusNode *startCactusNode;
    stList *deadEndComponent;
    stCactusGraph *cactusGraph = stCaf_getCactusGraphForThreadSet(flower, threadSet, &startCactusNode, &deadEndComponent, 0, INT64_MAX,
                                                                  0.0, breakChainsAtReverseTandems, maximumMedianSpacingBetweenLinkedEnds);

    stList *recoverableChains = getRecoverableChains(startCactusNode, maxRecoverableLength);
    stList *blocksToDelete = stList_construct3(0, (void(*)(void *)) stPinchBlock_destruct);
    for (int64_t i = 0; i < stList_length(recoverableChains); i++) {
        stCactusEdgeEnd *chainEnd = stList_get(recoverableChains, i);
        addChainBlocksToBlocksToDelete(chainEnd, blocksToDelete);
    }
    printf("Destroying %" PRIi64 " recoverable blocks\n", stList_length(blocksToDelete));
    stList_destruct(recoverableChains);
    stList_destruct(blocksToDelete);

    // FIXME: remove
    stCactusGraphNodeIt *it = stCactusGraphNodeIterator_construct(cactusGraph);
    stCactusNode *node;
    int64_t numCactusNodes = 0, numChains = 0;
    while ((node = stCactusGraphNodeIterator_getNext(it)) != NULL) {
        numCactusNodes++;
        numChains += stCactusNode_getChainNumber(node);
    }
    printf("There were actually %" PRIi64 " nodes and %" PRIi64 " chains in the graph\n", numCactusNodes, numChains);
    stCactusGraphNodeIterator_destruct(it);

    stCactusGraph_destruct(cactusGraph);
}

///////////////////////////////////////////////////////////////////////////
// Functions for calculating required species/tree coverage
///////////////////////////////////////////////////////////////////////////

Event *getEvent(stPinchSegment *segment, Flower *flower) {
    Event *event = cap_getEvent(flower_getCap(flower, stPinchSegment_getName(segment)));
    assert(event != NULL);
    return event;
}

bool stCaf_containsRequiredSpecies(stPinchBlock *pinchBlock, Flower *flower, int64_t minimumIngroupDegree,
        int64_t minimumOutgroupDegree, int64_t requiredAllSpecies) {
    int64_t outgroupSequences = 0;
    int64_t ingroupSequences = 0;
    stPinchBlockIt segmentIt = stPinchBlock_getSegmentIterator(pinchBlock);
    stPinchSegment *segment;
    while ((segment = stPinchBlockIt_getNext(&segmentIt)) != NULL) {
        Event *event = getEvent(segment, flower);
        if (event_isOutgroup(event)) {
            outgroupSequences++;
        } else {
            ingroupSequences++;
        }
    }
    return ingroupSequences >= minimumIngroupDegree && outgroupSequences >= minimumOutgroupDegree && outgroupSequences
            + ingroupSequences >= requiredAllSpecies;
}

bool stCaf_treeCoverage(stPinchBlock *pinchBlock, Flower *flower) {
    EventTree *eventTree = flower_getEventTree(flower);
    Event *commonAncestorEvent = NULL;
    stPinchSegment *segment;
    stPinchBlockIt segmentIt = stPinchBlock_getSegmentIterator(pinchBlock);
    while ((segment = stPinchBlockIt_getNext(&segmentIt))) {
        Event *event = getEvent(segment, flower);
        commonAncestorEvent = commonAncestorEvent == NULL ? event : eventTree_getCommonAncestor(event, commonAncestorEvent);
    }
    assert(commonAncestorEvent != NULL);
    float treeCoverage = 0.0;
    stHash *hash = stHash_construct();

    segmentIt = stPinchBlock_getSegmentIterator(pinchBlock);
    while ((segment = stPinchBlockIt_getNext(&segmentIt))) {
        Event *event = getEvent(segment, flower);
        while (event != commonAncestorEvent && stHash_search(hash, event) == NULL) {
            treeCoverage += event_getBranchLength(event);
            stHash_insert(hash, event, event);
            event = event_getParent(event);
        }
    }

    float wholeTreeCoverage = event_getSubTreeBranchLength(event_getChild(eventTree_getRootEvent(eventTree), 0));
    assert(wholeTreeCoverage >= 0.0);
    if (wholeTreeCoverage <= 0.0) { //deal with case all leaf branches are not empty.
        return 0.0;
    }
    treeCoverage /= wholeTreeCoverage;
    assert(treeCoverage >= -0.001);
    assert(treeCoverage <= 1.0001);
    return treeCoverage;
}
