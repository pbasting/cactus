#!/usr/bin/env python

#Copyright (C) 2011 by Glenn Hickey
#
#Released under the MIT license, see LICENSE.txt

"""Interface to the cactus experiment xml file used
to read and modify an existing experiment"""
import os
import xml.etree.ElementTree as ET
from xml.dom import minidom

from cactus.progressive.multiCactusTree import MultiCactusTree
from sonLib.nxnewick import NXNewick
from cactus.shared.common import cactusRootPath

class DbElemWrapper(object):
    def __init__(self, confElem):
        typeString = confElem.attrib["type"]
        dbElem = confElem.find(typeString)
        self.dbElem = dbElem
        self.confElem = confElem
        self.dbElem.attrib["database_dir"] = "fakepath"

    def check(self):
        """Function checks the database conf is as expected and creates useful exceptions
        if not"""
        dataString = ET.tostring(self.confElem)
        if self.confElem.tag != "st_kv_database_conf":
            raise RuntimeError("The database conf string is improperly formatted: %s" % dataString)
        if not self.confElem.attrib.has_key("type"):
            raise RuntimeError("The database conf string does not have a type attrib: %s" % dataString)
        typeString = self.confElem.attrib["type"]
        if typeString == "tokyo_cabinet":
            tokyoCabinet = self.confElem.find("tokyo_cabinet")
            if tokyoCabinet == None:
                raise RuntimeError("Database conf is of type tokyo cabinet but there is no nested tokyo cabinet tag: %s" % dataString)
            if not tokyoCabinet.attrib.has_key("database_dir"):
                raise RuntimeError("The tokyo cabinet tag has no database_dir tag: %s" % dataString)
        elif typeString == "kyoto_tycoon":
            kyotoTycoon = self.confElem.find("kyoto_tycoon")
            if kyotoTycoon == None:
                raise RuntimeError("Database conf is of kyoto tycoon but there is no nested kyoto tycoon tag: %s" % dataString)
            if not set(("host", "port", "database_dir")).issubset(set(kyotoTycoon.attrib.keys())):
                raise RuntimeError("The kyoto tycoon tag has a missing attribute: %s" % dataString)
        else:
            raise RuntimeError("Unrecognised database type in conf string: %s" % typeString)

    def getDbElem(self):
        return self.dbElem

    def getConfString(self):
        return ET.tostring(self.confElem)

    def getDbType(self):
        return self.dbElem.tag

    def getDbPort(self):
        assert self.getDbType() == "kyoto_tycoon"
        return int(self.dbElem.attrib["port"])

    def setDbPort(self, port):
        assert self.getDbType() == "kyoto_tycoon"
        self.dbElem.attrib["port"] = str(port)

    def getDbHost(self):
        assert self.getDbType() == "kyoto_tycoon"
        if "host" in self.dbElem.attrib:
            return self.dbElem.attrib["host"]
        return None

    def setDbHost(self, host):
        assert self.getDbType() == "kyoto_tycoon"
        self.dbElem.attrib["host"] = host

    def getDbServerOptions(self):
        assert self.getDbType() == "kyoto_tycoon"
        if "server_options" in self.dbElem.attrib:
            return self.dbElem.attrib["server_options"]
        return None

    def setDbServerOptions(self, options):
        assert self.getDbType() == "kyoto_tycoon"
        self.dbElem.attrib["server_options"] = str(options)

    def getDbTuningOptions(self):
        assert self.getDbType() == "kyoto_tycoon"
        if "tuning_options" in self.dbElem.attrib:
            return self.dbElem.attrib["tuning_options"]
        return None

    def setDbTuningOptions(self, options):
        assert self.getDbType() == "kyoto_tycoon"
        self.dbElem.attrib["tuning_options"] = str(options)

    def getDbCreateTuningOptions(self):
        assert self.getDbType() == "kyoto_tycoon"
        if "create_tuning_options" in self.dbElem.attrib:
            return self.dbElem.attrib["create_tuning_options"]
        return None

    def setDbCreateTuningOptions(self, options):
        assert self.getDbType() == "kyoto_tycoon"
        self.dbElem.attrib["create_tuning_options"] = str(options)

    def getDbReadTuningOptions(self):
        assert self.getDbType() == "kyoto_tycoon"
        if "read_tuning_options" in self.dbElem.attrib:
            return self.dbElem.attrib["read_tuning_options"]
        return None

    def setDbReadTuningOptions(self, options):
        assert self.getDbType() == "kyoto_tycoon"
        self.dbElem.attrib["read_tuning_options"] = str(options)

    def getDbInMemory(self):
        assert self.getDbType() == "kyoto_tycoon"
        if "in_memory" in self.dbElem.attrib:
            val = self.dbElem.attrib["in_memory"]
            retVal = val.lower() == "true" or val == "1"
            #assert (not retVal or "database_name" not in self.dbElem.attrib)
            return retVal
        return False

    def setDbInMemory(self, inMemory):
        assert self.getDbType() == "kyoto_tycoon"
        self.dbElem.attrib["in_memory"] = str(int(inMemory))

    def getDbSnapshot(self):
        assert self.getDbType() == "kyoto_tycoon"
        if "snapshot" in self.dbElem.attrib:
            val = self.dbElem.attrib["snapshot"]
            return val.lower() == "true" or val == "1"
        return self.getDbInMemory()

    def setDbSnapshot(self, snapshot):
        assert self.getDbType() == "kyoto_tycoon"
        self.dbElem.attrib["snapshot"] = str(int(snapshot))

class ExperimentWrapper(DbElemWrapper):
    def __init__(self, xmlRoot):
        self.diskElem = xmlRoot.find("cactus_disk")
        confElem = self.diskElem.find("st_kv_database_conf")
        super(ExperimentWrapper, self).__init__(confElem)
        self.xmlRoot = xmlRoot

        self.seqMap = self.buildSequenceMap()
        self.seqIDMap = None

    @staticmethod
    def createExperimentWrapper(sequences, newickTreeString, outputDir,
                 outgroupEvents=None,
                 databaseConf=None, configFile=None,
                 halFile=None, fastaFile=None,
                 constraints=None, progressive=False,
                 outputSequenceDir=None):
        #Establish the basics
        rootElem =  ET.Element("cactus_workflow_experiment")
        rootElem.attrib['species_tree'] = newickTreeString
        rootElem.attrib['sequences'] = " ".join(sequences)
        #Stuff for the database
        database = ET.SubElement(rootElem, "cactus_disk")
        if databaseConf != None:
            database.append(databaseConf)
        else:
            databaseConf = ET.SubElement(database, "st_kv_database_conf")
            databaseConf.attrib["type"] = "tokyo_cabinet"
            ET.SubElement(databaseConf, "tokyo_cabinet")
        self = ExperimentWrapper(rootElem)
        #Setup the config
        self.setConfigPath("default")
        if progressive == True:
            self.setConfigPath("defaultProgressive")
        if configFile != None:
            self.setConfigPath(configFile)
        #Constraints
        if constraints != None:
            self.setConstraintsFilePath(constraints)
        #Outgroup events
        if outgroupEvents != None:
            self.setOutgroupEvents(outgroupEvents)
        return self

    def writeXML(self, path): #Replacement for writeExperimentFile
        xmlFile = open(path, "w")
        xmlString = ET.tostring(self.xmlRoot)
        xmlString = xmlString.replace("\n", "")
        xmlString = xmlString.replace("\t", "")
        xmlString = minidom.parseString(xmlString).toprettyxml()
        xmlFile.write(xmlString)
        xmlFile.close()

    def getConfig(self):
        return self.xmlRoot.attrib["config"]

    def setConfigID(self, configID):
        self.xmlRoot.attrib["configID"] = str(configID)

    def getConfigID(self):
        return self.xmlRoot.attrib["configID"]

    def getTree(self, onlyThisSubtree=False):
        treeString = self.xmlRoot.attrib["species_tree"]
        ret = NXNewick().parseString(treeString, addImpliedRoots = False)
        if onlyThisSubtree:
            # Get a subtree containing only the reference node and its
            # children, rather than a species tree including the
            # outgroups as well
            multiCactus = MultiCactusTree(ret)
            multiCactus.nameUnlabeledInternalNodes()
            multiCactus.computeSubtreeRoots()
            ret = multiCactus.extractSubTree(self.getReferenceNameFromConfig())
        return ret

    def setSequences(self, sequences):
        self.xmlRoot.attrib["sequences"] = " ".join(sequences)
        self.seqMap = self.buildSequenceMap()

    def setSequenceIDs(self, sequenceIDs):
        self.xmlRoot.attrib["sequenceIDs"] = " ".join(map(str, sequenceIDs))

    def getSequences(self):
        return self.xmlRoot.attrib["sequences"].split()

    def getSequenceIDs(self):
        return self.xmlRoot.attrib["sequenceIDs"].split()

    def getSequence(self, event):
        return self.seqMap[event]

    def setReferenceID(self, refID):
        '''Set the file store ID of the reconstructed ancestral
        genome for this experiment. This should be downloaded
        onto the master node after the experiment has finished running.'''
        refElem = self.xmlRoot.find("reference")
        if refElem is None:
            refElem = ET.SubElement(self.xmlRoot, "reference")
        refElem.attrib["id"] = str(refID)

    def getReferenceID(self):
        refElem = self.xmlRoot.find("reference")
        if refElem is not None and "id" in refElem.attrib:
            return refElem.attrib["id"]
        else:
            return None

    def getReferenceNameFromConfig(self):
        configElem = ET.parse(self.getConfig()).getroot()
        refElem = configElem.find("reference")
        return refElem.attrib["reference"]

    def setHalID(self, halID):
        '''Set the file store ID of the HAL file
        resulting from this experiment.'''
        halElem = self.xmlRoot.find("hal")
        if halElem is None:
            halElem = ET.SubElement(self.xmlRoot, "hal")
        halElem.attrib["halID"] = str(halID)

    def getHalID(self):
        halElem = self.xmlRoot.find("hal")
        return halElem.attrib["halID"]

    def setHalFastaID(self, halFastaID):
        halElem = self.xmlRoot.find("hal")
        if halElem is None:
            halElem = ET.SubElement(self.xmlRoot, "hal")
        halElem.attrib["fastaID"] = str(halFastaID)

    def getHalFastaID(self):
        halElem = self.xmlRoot.find("hal")
        return halElem.attrib["fastaID"]

    def setConstraintsFilePath(self, path):
        self.xmlRoot.attrib["constraints"] = path

    def getConstraintsFilePath(self):
        if "constraints" not in self.xmlRoot.attrib:
            return None
        return self.xmlRoot.attrib["constraints"]

    def setConstraintsID(self, fileID):
        self.xmlRoot.attrib["constraintsID"] = str(fileID)

    def getConstraintsID(self, fileID):
        return self.xmlRoot.attrib["constraintsID"]

    def getOutgroupEvents(self):
        if self.xmlRoot.attrib.has_key("outgroup_events"):
            return self.xmlRoot.attrib["outgroup_events"].split()
        return []

    def setOutgroupEvents(self, outgroupEvents):
        self.xmlRoot.attrib["outgroup_events"] = " ".join(outgroupEvents)

    def getConfigPath(self):
        config = self.xmlRoot.attrib["config"]
        if config == 'default':
            config = os.path.join(cactusRootPath(), "cactus_config.xml")
        if config == 'defaultProgressive':
            config = os.path.join(cactusRootPath(), "cactus_progressive_config.xml")
        return config

    def setConfigPath(self, path):
        self.xmlRoot.attrib["config"] = path

    # map event names to sequence paths
    def buildSequenceMap(self):
        tree = self.getTree()
        sequenceString = self.xmlRoot.attrib["sequences"]
        sequences = sequenceString.split()
        nameIterator = iter(sequences)
        seqMap = dict()
        for node in tree.postOrderTraversal():
            if tree.isLeaf(node) or tree.getName(node) in self.getOutgroupEvents():
                seqMap[tree.getName(node)] = nameIterator.next()

        # Check that there are no sequences left unassigned
        assert len(seqMap.keys()) == len(sequences)

        return seqMap

    # load in a new tree (using input seqMap if specified,
    # current one otherwise
    def updateTree(self, tree, seqMap = None, outgroups = None):
        if seqMap is not None:
            self.seqMap = seqMap
        newMap = dict()
        treeString = NXNewick().writeString(tree)
        self.xmlRoot.attrib["species_tree"] = treeString
        if outgroups is not None and len(outgroups) > 0:
            self.setOutgroupEvents(outgroups)

        sequences = ""
        for node in tree.postOrderTraversal():
            if tree.isLeaf(node) or tree.getName(node) in self.getOutgroupEvents():
                nodeName = tree.getName(node)
                if len(sequences) > 0:
                    sequences += " "
                sequences += seqMap[nodeName]
                newMap[nodeName] = seqMap[nodeName]
        self.xmlRoot.attrib["sequences"] = sequences
        self.seqMap = newMap

    # return internal structure that maps event names to paths
    def getSequenceMap(self):
        return self.seqMap

    def checkSequenceIDs(self, fileStore):
        tree = self.getTree()
        sequences = [fileStore.readGlobalFile(seqID) for seqID in self.seqIDMap.values()]
        nameIter = iter(self.seqIDMap.keys())
        seqIter = iter(sequences)
        for node in tree.postOrderTraversal():
            if tree.isLeaf(node):
                name = nameIter.next()
                seq = seqIter.next()
                if not name == tree.getName(node):
                    raise RuntimeError("name = %s, traversalName = %s" % (name, tree.getName(node)))
                first_line = ""
                with open(seq, 'r') as seqFH:
                    first_line = seqFH.readline()
                first_line = first_line[1:]
                first_line = first_line.split("|")[1]
                if not first_line.startswith(name):
                    raise RuntimeError("First_line = %s, name = %s" % (first_line, name))

