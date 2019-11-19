#include "rewriter/rewriter.h"
#include "ast/treemap/treemap.h"
#include "ast/verifier/verifier.h"
#include "common/typecase.h"
#include "main/pipeline/semantic_extension/SemanticExtension.h"
#include "rewriter/ClassNew.h"
#include "rewriter/Command.h"
#include "rewriter/DSLBuilder.h"
#include "rewriter/DefaultArgs.h"
#include "rewriter/Delegate.h"
#include "rewriter/InterfaceWrapper.h"
#include "rewriter/Mattr.h"
#include "rewriter/Minitest.h"
#include "rewriter/MixinEncryptedProp.h"
#include "rewriter/Private.h"
#include "rewriter/Prop.h"
#include "rewriter/ProtobufDescriptorPool.h"
#include "rewriter/Rails.h"
#include "rewriter/Regexp.h"
#include "rewriter/Struct.h"
#include "rewriter/TEnum.h"
#include "rewriter/TypeMembers.h"
#include "rewriter/attr_reader.h"
#include "rewriter/flatten.h"
#include "rewriter/module_function.h"

using namespace std;

namespace sorbet::rewriter {

class Rewriterer {
    friend class Rewriter;

public:
    unique_ptr<ast::ClassDef> postTransformClassDef(core::MutableContext ctx, unique_ptr<ast::ClassDef> classDef) {
        Command::run(ctx, classDef.get());
        Rails::run(ctx, classDef.get());
        TEnum::run(ctx, classDef.get());
        Prop::run(ctx, classDef.get());
        TypeMembers::run(ctx, classDef.get());
        DefaultArgs::run(ctx, classDef.get());

        for (auto &extension : ctx.state.semanticExtensions) {
            extension->run(ctx, classDef.get());
        }

        ast::Expression *prevStat = nullptr;
        UnorderedMap<ast::Expression *, vector<unique_ptr<ast::Expression>>> replaceNodes;
        for (auto &stat : classDef->rhs) {
            typecase(
                stat.get(),
                [&](ast::Assign *assign) {
                    vector<unique_ptr<ast::Expression>> nodes;

                    nodes = Struct::run(ctx, assign);
                    if (!nodes.empty()) {
                        replaceNodes[stat.get()] = std::move(nodes);
                        return;
                    }

                    nodes = ClassNew::run(ctx, assign);
                    if (!nodes.empty()) {
                        replaceNodes[stat.get()] = std::move(nodes);
                        return;
                    }

                    nodes = ProtobufDescriptorPool::run(ctx, assign);
                    if (!nodes.empty()) {
                        replaceNodes[stat.get()] = std::move(nodes);
                        return;
                    }

                    nodes = Regexp::run(ctx, assign);
                    if (!nodes.empty()) {
                        replaceNodes[stat.get()] = std::move(nodes);
                        return;
                    }
                },

                [&](ast::Send *send) {
                    vector<unique_ptr<ast::Expression>> nodes;

                    nodes = MixinEncryptedProp::run(ctx, send);
                    if (!nodes.empty()) {
                        replaceNodes[stat.get()] = std::move(nodes);
                        return;
                    }

                    nodes = Minitest::run(ctx, send);
                    if (!nodes.empty()) {
                        replaceNodes[stat.get()] = move(nodes);
                        return;
                    }

                    nodes = DSLBuilder::run(ctx, send);
                    if (!nodes.empty()) {
                        replaceNodes[stat.get()] = std::move(nodes);
                        return;
                    }

                    nodes = Private::run(ctx, send);
                    if (!nodes.empty()) {
                        replaceNodes[stat.get()] = std::move(nodes);
                        return;
                    }

                    nodes = Delegate::run(ctx, send);
                    if (!nodes.empty()) {
                        replaceNodes[stat.get()] = std::move(nodes);
                        return;
                    }

                    // This one is different: it gets an extra prevStat argument.
                    nodes = AttrReader::run(ctx, send, prevStat);
                    if (!nodes.empty()) {
                        replaceNodes[stat.get()] = std::move(nodes);
                        return;
                    }

                    // This one is also a little different: it gets the ClassDef kind
                    nodes = Mattr::run(ctx, send, classDef->kind);
                    if (!nodes.empty()) {
                        replaceNodes[stat.get()] = std::move(nodes);
                        return;
                    }
                },

                [&](ast::Expression *e) {});

            prevStat = stat.get();
        }
        if (replaceNodes.empty()) {
            ModuleFunction::run(ctx, classDef.get());
            return classDef;
        }

        auto oldRHS = std::move(classDef->rhs);
        classDef->rhs.clear();
        classDef->rhs.reserve(oldRHS.size());

        for (auto &stat : oldRHS) {
            if (replaceNodes.find(stat.get()) == replaceNodes.end()) {
                classDef->rhs.emplace_back(std::move(stat));
            } else {
                for (auto &newNode : replaceNodes.at(stat.get())) {
                    classDef->rhs.emplace_back(std::move(newNode));
                }
            }
        }
        ModuleFunction::run(ctx, classDef.get());

        return classDef;
    }

    unique_ptr<ast::Expression> postTransformSend(core::MutableContext ctx, unique_ptr<ast::Send> send) {
        return InterfaceWrapper::run(ctx, std::move(send));
    }

private:
    Rewriterer() = default;
};

unique_ptr<ast::Expression> Rewriter::run(core::MutableContext ctx, unique_ptr<ast::Expression> tree) {
    auto ast = std::move(tree);

    Rewriterer rewriter;
    ast = ast::TreeMap::apply(ctx, rewriter, std::move(ast));
    // This AST flattening pass requires that we mutate the AST in a way that our previous DSL passes were not designed
    // around, which is why it runs all at once and is not expressed as a `patch` method like the other DSL passes. This
    // is a rare case: in general, we should *not* add new DSL passes here.
    auto flattened = Flatten::run(ctx, std::move(ast));
    auto verifiedResult = ast::Verifier::run(ctx, std::move(flattened));
    return verifiedResult;
}

}; // namespace sorbet::rewriter
