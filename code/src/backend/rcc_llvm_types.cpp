//
// LLVM Type Manager Implementation for RCC Backend
//

#include "../../include/backend/rcc_llvm_types.h"

namespace backend {

LLVMTypeManager::LLVMTypeManager(llvm::LLVMContext& ctx)
    : context(ctx) {}

llvm::Type* LLVMTypeManager::getIntType() const {
    return llvm::Type::getInt64Ty(context);
}

llvm::Type* LLVMTypeManager::getFloatType() const {
    return llvm::Type::getDoubleTy(context);
}

llvm::Type* LLVMTypeManager::getBoolType() const {
    return llvm::Type::getInt1Ty(context);
}

llvm::Type* LLVMTypeManager::getCharType() const {
    return llvm::Type::getInt8Ty(context);
}

llvm::Type* LLVMTypeManager::getStringType() const {
    return llvm::Type::getInt8PtrTy(context);
}

llvm::Type* LLVMTypeManager::getObjectType() const {
    return llvm::Type::getInt8PtrTy(context);
}

llvm::Type* LLVMTypeManager::getVoidType() const {
    return llvm::Type::getVoidTy(context);
}

llvm::Type* LLVMTypeManager::getPtrType() const {
    return llvm::Type::getInt8PtrTy(context);
}

llvm::Type* LLVMTypeManager::getValueType() const {
    return llvm::Type::getInt64Ty(context);
}

llvm::Type* LLVMTypeManager::getFloatValueType() const {
    return llvm::Type::getDoubleTy(context);
}

llvm::Type* LLVMTypeManager::toLLVMType(RioType rioType) const {
    switch (rioType) {
        case RioType::INT:
            return getIntType();
        case RioType::FLOAT:
            return getFloatType();
        case RioType::BOOL:
            return getIntType(); // stored as i64
        case RioType::CHAR:
            return getIntType(); // stored as i64
        case RioType::STRING:
            return getStringType();
        case RioType::LIST:
            return getObjectType();
        case RioType::DICT:
            return getObjectType();
        case RioType::NULL_TYPE:
            return getPtrType();
        case RioType::VOID:
            return getVoidType();
        case RioType::ANY:
        case RioType::UNKNOWN:
        default:
            return getValueType();
    }
}

} // namespace backend
