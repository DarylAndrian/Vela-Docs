// test_TabBar.cpp — v0.3.1 (CRIT-02 + CRIT-05)
#include <catch2/catch_test_macros.hpp>
#include "app/TabBar.h"
#include "document/Document.h"
#include "document/DocumentController.h"
#include "document/TabSession.h"

#include <QApplication>
#include <memory>

TEST_CASE("TabBar — addSession registers a tab with title", "[tabbar]") {
    if (!QApplication::instance()) {
        static int argc = 1; static char app[] = "t"; static char* a[] = { app, nullptr };
        (void)new QApplication(argc, a);
    }
    auto session = vela::TabSession::create();
    vela::TabBar tb;
    const int idx = tb.addSession(session.get());
    CHECK(idx >= 0);
    tb.removeSession(idx);
}
