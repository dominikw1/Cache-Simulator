// No-op Skeleton Pass taken from https://github.com/sampsyo/llvm-pass-skeleton/tree/master and adapted.
// Inspiration also taken from https://www.bitsand.cloud/subposts/llvm-pass/llvm-pass-part6

#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

struct MemoryAnalyser : public PassInfoMixin<MemoryAnalyser> {
    std::vector<Type*> getLogFunctionArgs(LLVMContext& context, bool isWrite) {
        if (isWrite) {
            // void logWrite(void* address, uint64_t value)
            return {PointerType::getUnqual(Type::getInt8Ty(context)), Type::getInt64Ty(context)};
        } else {
            // void logRead(void* address)
            return {PointerType::getUnqual(Type::getInt8Ty(context))};
        }
    }

    FunctionCallee constructLogFunctionCallee(Module& module, LLVMContext& context, const std::vector<Type*>& args,
                                              bool isWrite) {
        auto* logFunctionType = FunctionType::get(Type::getVoidTy(context), args, false);
        if (isWrite) {
            return module.getOrInsertFunction("logWrite", logFunctionType);
        } else {
            return module.getOrInsertFunction("logRead", logFunctionType);
        }
    }

    template <typename T> Value* extractAddress(T* instr, Type* addressType, IRBuilder<>& builder) {
        return builder.CreatePointerCast(instr->getPointerOperand(), addressType, "address");
    }

    Value* extractValue(StoreInst& instr) { return instr.getOperand(0); }

    Value* convertValueToType(Value* val, Type* valueType, IRBuilder<>& builder) {
        // not sure whether this is necessary
        if (val->getType()->isPointerTy()) {
            return builder.CreatePtrToInt(val, valueType, "value");
        } else {
            return builder.CreateIntCast(val, valueType, false, "value");
        }
    }

    void instrument(Module& module, Instruction& instr, bool isWrite) {
        auto& context = module.getContext();
        auto logFunctionArgs = getLogFunctionArgs(context, isWrite);
        auto logFunction = constructLogFunctionCallee(module, context, logFunctionArgs, isWrite);

        IRBuilder<> builder{instr.getNextNode()};

        if (!isWrite) {
            LoadInst* loadInstr = dyn_cast<LoadInst>(&instr);
            if (!loadInstr) {
                exit(1);
            }
            auto* address = extractAddress(loadInstr, logFunctionArgs[0], builder);
            builder.CreateCall(logFunction, {address});
        } else {
            StoreInst* storeInstr = dyn_cast<StoreInst>(&instr);
            if (!storeInstr) {
                exit(1);
            }
            auto* address = extractAddress(storeInstr, logFunctionArgs[0], builder);
            auto* value = convertValueToType(extractValue(*storeInstr), logFunctionArgs[1], builder);
            builder.CreateCall(logFunction, {address, value});
        }
    }

    PreservedAnalyses run(Module& module, ModuleAnalysisManager& analysisManager) {
        for (auto& func : module) {
            if (func.getName() == "logWrite" || func.getName() == "logRead") {
                continue;
            }
            for (auto& bb : func) {
                for (auto& instr : bb) {
                    if (instr.getOpcode() == Instruction::Load || instr.getOpcode() == Instruction::Store) {
                        instrument(module, instr, instr.getOpcode() == Instruction::Store);
                    }
                }
            }
        }
        return PreservedAnalyses::none();
    }
};

} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return {.APIVersion = LLVM_PLUGIN_API_VERSION,
            .PluginName = "MemoryAnalyser",
            .PluginVersion = "v1.0",
            .RegisterPassBuilderCallbacks = [](PassBuilder& passBuilder) {
                passBuilder.registerPipelineStartEPCallback(
                    [](ModulePassManager& modulePassManager, OptimizationLevel level) {
                        modulePassManager.addPass(MemoryAnalyser());
                    });
            }};
}