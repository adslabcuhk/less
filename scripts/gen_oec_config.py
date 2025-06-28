import sys
import argparse
import sys
import subprocess
from pathlib import Path
import configparser
import xml.etree.cElementTree as ET

# import common configs
import common

class DictToObject:
    def __init__(self, dictionary):
        for key, value in dictionary.items():
            setattr(self, key, value)

def parseArgs(cmdArgs):
    argParser = argparse.ArgumentParser(description="generate OpenEC config file") 

    argParser.add_argument("-f", type=str, help="Evaluation config file (.ini)", default=common.EXP_SCRIPT_DIR + "/settings.ini")
    argParser.add_argument("-o", type=str, help="OpenEC config file (.xml)", default=common.CONFIG_DIR + "/sysSetting.xml")
    
    args = argParser.parse_args(cmdArgs)
    return args

def execCmd(cmd, exec=True, timeout=None):
    print("Execute Command: {}".format(cmd))
    msg = ""
    success=False
    if exec == True:
        with subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE) as p:
            try:
                retStr, stderr = p.communicate(timeout=timeout)
                msg = retStr.decode().strip()
                success=True
            except Exception as e:
                print(e)
                p.terminate()
        print(msg)
    return msg, success

def addAttrToXMLTree(root, attrName, attrVal):
    attribute = ET.SubElement(root, "attribute")

    name = ET.SubElement(attribute, "name")
    name.text = attrName

    value = ET.SubElement(attribute, "value")
    value.text = attrVal

def addAttrListToXMLTree(root, attrName, attrValList):
    attribute = ET.SubElement(root, "attribute")

    name = ET.SubElement(attribute, "name")
    name.text = attrName

    for item in attrValList:
        value = ET.SubElement(attribute, "value")
        value.text = item

def addAttrTupleListToXMLTree(root, attrName, attrValList):
    attribute = ET.SubElement(root, "attribute")

    name = ET.SubElement(attribute, "name")
    name.text = attrName

    for tp in attrValList:
        value = ET.SubElement(attribute, "value")
        for item in tp:
            key, val = item
            keyElem = ET.SubElement(value, key)
            keyElem.text = val

def getCodeList(codeTestListFile):
    # code name
    codeList = []
    with open(codeTestListFile, 'r') as f:
        for line in f.readlines():
            if len(line.strip()) == 0:
                continue
            items = line.strip().split(' ')
            codeName = items[0]
            codeN = int(items[1])
            codeK = int(items[2])
            codeW = int(items[3])
            codeList.append((codeName, codeN, codeK, codeW))
    return codeList

def main():
    args = parseArgs(sys.argv[1:])
    if not args:
        exit()
    
    # Input parameters: configFile
    configFile = args.f
    outputConfigXMLFile = args.o

    # parse configs
    configsRaw = configparser.ConfigParser()
    configsRaw.read(configFile)
    configs = DictToObject({section: dict(configsRaw[section]) for section in configsRaw.sections()})

    oec = DictToObject(configs.OpenEC)
    experiment = DictToObject(configs.Experiment)
    cluster = DictToObject(configs.Cluster)

    # read node list
    nodeIpList = []
    with open(cluster.node_list_file, 'r') as f:
        for line in f.readlines():
            nodeIpList.append(line.strip())

    # generate xml file
    root = ET.Element("setting")
    addAttrToXMLTree(root, "controller.addr", nodeIpList[int(cluster.controller_id)])
    agentIds = cluster.agent_ids.split(',')
    # set default rack
    addAttrListToXMLTree(root, "agents.addr", [("/default/" + nodeIpList[int(agentId)]) for agentId in agentIds])
    addAttrToXMLTree(root, "oec.controller.thread.num", oec.controller_thread_num)
    addAttrToXMLTree(root, "oec.agent.thread.num", oec.agent_thread_num)
    addAttrToXMLTree(root, "oec.cmddist.thread.num", oec.cmddist_thread_num)
    # set to controller address
    addAttrToXMLTree(root, "local.addr", nodeIpList[int(cluster.controller_id)])
    addAttrToXMLTree(root, "packet.size", oec.packet_size_byte)
    addAttrToXMLTree(root, "dss.type", oec.dss_type)
    addAttrToXMLTree(root, "dss.parameter", nodeIpList[int(cluster.controller_id)] + "," + oec.controller_port)
    addAttrToXMLTree(root, "ec.concurrent.num", oec.ec_concurrent_num)
    # ec policy
    codeList = getCodeList(experiment.code_test_list_file)
    ECPolicyXMLList = []
    for item in codeList:
        codeName, codeN, codeK, codeW = item
        codeId = "_".join((codeName, str(codeN), str(codeK), str(codeW)))
        opt = "-1"
        param = "-"
        tp = []
        tp.append(("ecid", codeId))
        tp.append(("class", codeName))
        tp.append(("n", str(codeN)))
        tp.append(("k", str(codeK)))
        tp.append(("w", str(codeW)))
        tp.append(("opt", opt))
        
        # set param for specific codes
        if codeName == "Clay":
            param = str(codeN - 1)
        elif codeName == "ETRSConv":
            param = str(codeW)
        tp.append(("param", param))
        ECPolicyXMLList.append(tp)
    addAttrTupleListToXMLTree(root, "ec.policy", ECPolicyXMLList)
    # ec offline pool
    ECOfflinePoolXMLList = []
    for item in codeList:
        codeName, codeN, codeK, codeW = item
        codeId = "_".join((codeName, str(codeN), str(codeK), str(codeW)))
        codePoolId = codeId + "_pool"
        blockSizeMiB = str(int(int(oec.block_size_byte) / 1024 / 1024))
        tp = []
        tp.append(("poolid", codePoolId))
        tp.append(("ecid", codeId))
        tp.append(("base", blockSizeMiB))

        ECOfflinePoolXMLList.append(tp)
    addAttrTupleListToXMLTree(root, "offline.pool", ECOfflinePoolXMLList)

    # write xml to file
    tree = ET.ElementTree(root)
    ET.indent(tree, space="\t", level=0)
    print("Generated OpenEC config, save to {}".format(outputConfigXMLFile))

    Path(common.CONFIG_DIR).mkdir(parents=True, exist_ok=True)
    tree.write(outputConfigXMLFile)



if __name__ == '__main__':
    main()