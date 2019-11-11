#include "main/lsp/LSPTypecheckerCoordinator.h"

#include "absl/synchronization/notification.h"

namespace sorbet::realmain::lsp {
using namespace std;

LSPTypecheckerCoordinator::LSPTypecheckerCoordinator(const shared_ptr<const LSPConfiguration> &config)
    : shouldTerminate(false), typechecker(config), config(config), hasDedicatedThread(false) {}

void LSPTypecheckerCoordinator::asyncRunInternal(function<void()> &&lambda, bool canPreemptSlowPath) {
    if (hasDedicatedThread) {
        lambdas.push(LSPTypecheckerLambda{move(lambda), canPreemptSlowPath}, 1);
        if (canPreemptSlowPath) {
            // TODO: Install preemption handler...? It'll run all preemptible things in queue.
        }
    } else {
        lambda();
    }
}

void LSPTypecheckerCoordinator::asyncRun(function<void(LSPTypechecker &)> &&lambda, bool canPreemptSlowPath) {
    asyncRunInternal([&typechecker = this->typechecker, lambda]() -> void { lambda(typechecker); }, canPreemptSlowPath);
}

void LSPTypecheckerCoordinator::syncRun(function<void(LSPTypechecker &)> &&lambda, bool canPreemptSlowPath) {
    absl::Notification notification;
    CounterState typecheckerCounters;
    // If typechecker is running on a dedicated thread, then we need to merge its metrics w/ coordinator thread's so we
    // report them.
    // Note: Capturing notification by reference is safe here, we we wait for the notification to happen prior to
    // returning.
    asyncRunInternal(
        [&typechecker = this->typechecker, lambda, &notification, &typecheckerCounters,
         hasDedicatedThread = this->hasDedicatedThread]() -> void {
            lambda(typechecker);
            if (hasDedicatedThread) {
                typecheckerCounters = getAndClearThreadCounters();
            }
            notification.Notify();
        },
        canPreemptSlowPath);
    notification.WaitForNotification();
    if (hasDedicatedThread) {
        counterConsume(move(typecheckerCounters));
    }
}

unique_ptr<core::GlobalState> LSPTypecheckerCoordinator::shutdown() {
    unique_ptr<core::GlobalState> gs;
    syncRun(
        [&](auto &typechecker) -> void {
            shouldTerminate = true;
            gs = typechecker.destroy();
        },
        false);
    return gs;
}

unique_ptr<Joinable> LSPTypecheckerCoordinator::startTypecheckerThread() {
    if (hasDedicatedThread) {
        Exception::raise("Typechecker already started on a dedicated thread.");
    }

    hasDedicatedThread = true;
    return runInAThread("Typechecker", [&]() -> void {
        typechecker.changeThread();

        while (!shouldTerminate) {
            LSPTypecheckerLambda lambda;
            auto result = lambdas.wait_pop_timed(lambda, WorkerPool::BLOCK_INTERVAL(), *config->logger);
            if (result.gotItem()) {
                lambda.lambda();
            }
        }
    });
}

}; // namespace sorbet::realmain::lsp