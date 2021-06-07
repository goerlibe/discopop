/*
 * This file is part of the DiscoPoP software (http://www.discopop.tu-darmstadt.de)
 *
 * Copyright (c) 2020, Technische Universitaet Darmstadt, Germany
 *
 * This software may be modified and distributed under the terms of
 * the 3-Clause BSD License. See the LICENSE file in the package base
 * directory for details.
 *
 */

#include "llvm/Transforms/Instrumentation.h"
#include "llvm/ADT/ilist.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/User.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/RegionIterator.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/RegionInfo.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Pass.h"
#include "llvm/PassAnalysisSupport.h"
#include "llvm/PassSupport.h"
#include "llvm-c/Core.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/PassRegistry.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Analysis/CallGraph.h"
#include <llvm/IR/DebugLoc.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include "llvm/Analysis/TargetLibraryInfo.h"

#include "DPUtils.h"

#include <stack>
#include <utility>
#include <iomanip>
#include <algorithm>
#include <string.h>
#include <iostream>

using namespace llvm;
using namespace std;
using namespace dputil;

// Command line options
static cl::opt<string> ClInputFile("inputFile", cl::desc("path to input file"), cl::Hidden);
static cl::opt<string> ClOutputFile("outputFile", cl::desc("path to output file"), cl::Hidden);

namespace
{
    struct relevantSection
    {
        string filePath;
        int startLine;
        int endLine;
        string varName;
    };

    struct BehaviorExtraction : public FunctionPass
    {
        static char ID;
        virtual bool runOnFunction(Function &F);
        StringRef getPassName() const;
        BehaviorExtraction() : FunctionPass(ID) {}
        bool doInitialization(Module &M);
        stack<relevantSection> sections;

    }; // end of struct BehaviorExtraction

} // end of anonymous namespace

bool BehaviorExtraction::runOnFunction(Function &F)
{
    ofstream outputFile(ClOutputFile);
    outputFile << "Hello from LLVM!\n";
    outputFile.close();
    
    return false;
}

char BehaviorExtraction::ID = 0;

static RegisterPass<BehaviorExtraction> X("BehaviorExtraction", "BehaviorExtraction: determine computation units.", false, false);

static void loadPass(const PassManagerBuilder &Builder, legacy::PassManagerBase &PM)
{
    PM.add(new LoopInfoWrapperPass());
    PM.add(new RegionInfoPass());
    PM.add(new BehaviorExtraction());
}

static RegisterStandardPasses BehaviorExtractionLoader_Ox(PassManagerBuilder::EP_OptimizerLast, loadPass);
static RegisterStandardPasses BehaviorExtractionLoader_O0(PassManagerBuilder::EP_EnabledOnOptLevel0, loadPass);

FunctionPass *createBehaviorExtractionPass()
{

    initializeLoopInfoWrapperPassPass(*PassRegistry::getPassRegistry());
    initializeRegionInfoPassPass(*PassRegistry::getPassRegistry());
    return new BehaviorExtraction();
}

bool BehaviorExtraction::doInitialization(Module &M){
    errs() << "Initialization  -  Read Input file to stack\n";
    // read input file
    ifstream inputFile(ClInputFile);
    string line;
    string columnDelimiter = ";";
    while(getline(inputFile, line)){
        // store line contents on sections-stack
        string tmp[4];
        string token;
        int counter = 0;
        size_t pos = 0;
        while ((pos = line.find(columnDelimiter)) != std::string::npos) {
            token = line.substr(0, pos);
            line.erase(0, pos + columnDelimiter.length());
            tmp[counter] = token;
            counter++;
        }
        struct relevantSection curSection;
        curSection.filePath = tmp[0];
        curSection.startLine = stoi(tmp[1]);
        curSection.endLine = stoi(tmp[2]);
        curSection.varName = tmp[3];
        sections.push(curSection);
    }
    inputFile.close();

    errs() << "Initialization finished.\n";
}

StringRef BehaviorExtraction::getPassName() const
{
    return "BehaviorExtraction";
}
