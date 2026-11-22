// ════════════════════════════════════════════════════════════════════════════
//  test_zip.cpp — Smoke tests for the Zip utility
// ════════════════════════════════════════════════════════════════════════════
#include <catch2/catch_test_macros.hpp>

#include "util/Zip.h"

#include <QHash>
#include <QString>
#include <QTemporaryFile>
#include <QDir>

using namespace vela;

TEST_CASE("Zip round-trip writes and reads back the same entries", "[zip]")
{
    // Build a unique temp file path
    const QString tmpPath = QDir::tempPath() + QStringLiteral("/vela-test-XXXXXX.zip");
    QTemporaryFile* tmp = new QTemporaryFile(tmpPath);
    tmp->setAutoRemove(false);
    REQUIRE(tmp->open());
    tmp->close();
    const QString path = tmp->fileName();
    delete tmp;

    QHash<QString, QByteArray> entries;
    entries.insert(QStringLiteral("[Content_Types].xml"),
                   QByteArray("<?xml version=\"1.0\"?><Types/>"));
    entries.insert(QStringLiteral("word/document.xml"),
                   QByteArray("<?xml version=\"1.0\"?><doc/>"));
    entries.insert(QStringLiteral("word/styles.xml"),
                   QByteArray("<?xml version=\"1.0\"?><styles/>"));

    REQUIRE(Zip::write(path, entries));

    QHash<QString, QByteArray> roundtrip;
    REQUIRE(Zip::openForReading(path, roundtrip));
    REQUIRE(roundtrip.size() == entries.size());
    REQUIRE(roundtrip.value(QStringLiteral("word/document.xml"))
            == QByteArray("<?xml version=\"1.0\"?><doc/>"));

    QFile::remove(path);
}

TEST_CASE("Zip::openForReading returns false for missing files", "[zip]")
{
    QHash<QString, QByteArray> entries;
    REQUIRE_FALSE(Zip::openForReading(
        QStringLiteral("Z:/this/path/should/never/exist.zip"), entries));
}
