#pragma once

#include "rx/ast/program.hpp"
#include <memory>

namespace rx::semantic {
    
    std::unordered_set<std::string> parameter_names;
    std::unordered_set<std::string> field_names;

    bool checkDeclarations(
        const std::shared_ptr<rx::ast::Program>& program
    );

    bool checkFunctionDeclaration(
        const std::shared_ptr<rx::ast::FunctionItem>& function
    );

    bool checkStructDeclaration(
        const std::shared_ptr<rx::ast::StructItem>& structure
    );

    bool checkConstantDeclaration(
        const std::shared_ptr<rx::ast::ConstantItem>& constant
    );
}