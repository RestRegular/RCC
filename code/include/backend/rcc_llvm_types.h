//
// LLVM Type Manager for RCC Backend
// Manages LLVM type mappings for Rio language types
//

#ifndef RCC_LLVM_TYPES_H
#define RCC_LLVM_TYPES_H

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>

namespace backend {

// Rio type enumeration for internal type tracking
enum class RioType {
    INT,        // i64
    FLOAT,      // double
    BOOL,       // i1 (stored as i64)
    CHAR,       // i8
    STRING,     // RioString* (i8*)
    LIST,       // RioList* (i8*)
    DICT,       // RioDict* (i8*)
    NULL_TYPE,  // nullptr (i8*)
    VOID,       // void
    ANY,        // dynamic type
    UNKNOWN     // type inference failed
};

class LLVMTypeManager {
public:
    explicit LLVMTypeManager(llvm::LLVMContext& ctx);

    // Get LLVM types
    [[nodiscard]] llvm::Type* getIntType() const;       // i64
    [[nodiscard]] llvm::Type* getFloatType() const;     // double
    [[nodiscard]] llvm::Type* getBoolType() const;      // i1
    [[nodiscard]] llvm::Type* getCharType() const;      // i8
    [[nodiscard]] llvm::Type* getStringType() const;    // RioString* (i8*)
    [[nodiscard]] llvm::Type* getObjectType() const;    // RioObject* (i8*)
    [[nodiscard]] llvm::Type* getVoidType() const;      // void
    [[nodiscard]] llvm::Type* getPtrType() const;       // i8*

    // Generic value type (for uniform representation of all Rio values)
    // We use i64 for int/bool/char, double for float, i8* for object references
    [[nodiscard]] llvm::Type* getValueType() const;         // i64 (generic value type)
    [[nodiscard]] llvm::Type* getFloatValueType() const;    // double

    // Convert RioType to LLVM Type
    [[nodiscard]] llvm::Type* toLLVMType(RioType rioType) const;

    // Get the LLVM context reference
    [[nodiscard]] llvm::LLVMContext& getContext() const { return context; }

private:
    llvm::LLVMContext& context;
};

} // namespace backend

#endif // RCC_LLVM_TYPES_H
