// test_EditorBridge.cpp — v0.3.1 (CRIT-06 + CRIT-05)
#include <catch2/catch_test_macros.hpp>
#include "app/EditorBridge.h"
#include "document/Document.h"
#include "document/DocumentController.h"

using namespace vela;

TEST_CASE("EditorBridge — invokeCommand returns a positive id and resolves synchronously", "[bridge]") {
    Document d; DocumentController c(&d);
    EditorBridge bridge(&c);
    bridge.setWatchdogMs(50);

    QVariantMap args; args["path"] = QString();
    QSignalSpy spy(&bridge, &EditorBridge::commandResult);
    const qint64 id = bridge.invokeCommand("newDocument", args);
    CHECK(id > 0);
    REQUIRE(spy.wait(200));
    CHECK(spy.count() == 1);
}
