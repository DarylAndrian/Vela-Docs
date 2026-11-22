// test_MainWindow.cpp — v0.3.1 (CRIT-05)
#include <catch2/catch_test_macros.hpp>

#include <QApplication>
#include "app/MainWindow.h"
#include "document/Document.h"

TEST_CASE("MainWindow — constructs without throwing", "[mainwindow]") {
    if (!QApplication::instance()) {
        static int argc = 1;
        static char appName[] = "vela-tests";
        static char* argv[] = { appName, nullptr };
        (void)new QApplication(argc, argv);
    }
    Document d;
    vela::MainWindow w(&d);
    SUCCEED("constructed");
    w.close();
}
