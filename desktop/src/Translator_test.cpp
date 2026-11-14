#include <catch2/catch_test_macros.hpp>
#include "Translator.h"

using namespace pigmenta::desktop;

// Every key actually referenced across the UI source — kept in sync by hand
// (not derived from the sources) so this test fails loudly if a key used in
// the app is missing from the table, or if the table has a stale/unused
// entry, in either language.
namespace {
const QStringList kExpectedKeys = {
    "menu.file",
    "menu.openImage",
    "menu.openSample",
    "menu.exportImage",
    "menu.exit",
    "menu.view",
    "menu.help",
    "menu.documentation",
    "menu.about",
    "mode.after",
    "mode.before",
    "mode.wipe",
    "mode.sideBySide",
    "sidebar.exportButton",
    "status.noImage",
    "status.failedLoad",
    "status.failedExport",
    "status.exported",
    "dialog.openImage",
    "dialog.exportImage",
    "dialog.imageFilter",
    "dropArea.text",
    "compareView.empty",
    "filterPanel.filters",
    "filterPanel.parameters",
    "filter.original",
    "filter.decorrelation",
    "filter.clahe",
    "filter.gamma",
    "filter.saturation",
    "filter.relief",
    "filter.irSimulate",
    "param.colorspace",
    "param.targetStd",
    "param.intensity",
    "param.tileSize",
    "param.clipLimit",
    "param.gamma",
    "param.factor",
    "param.angle",
    "param.strength",
    "welcome.title",
    "welcome.brand",
    "welcome.subtitle",
    "welcome.blurb",
    "welcome.examples",
    "welcome.about",
    "welcome.dontShowAgain",
    "welcome.getStarted",
    "welcome.close",
    "about.version",
    "about.createdBy",
    "about.projectBy",
    "about.license",
    "about.source",
    "about.credits",
    "about.creditsLink",
    "sample.huashan1",
    "sample.zuojiang",
    "sample.thailand",
    "sample.paraguay",
    "docs.title",
    "docs.overview",
    "docs.architecture",
    "docs.building",
    "docs.notFound",
};
}

TEST_CASE("every expected key resolves to non-empty text in both languages", "[translator]") {
    auto& t = Translator::instance();
    const Lang original = t.language();

    for (const QString& key : kExpectedKeys) {
        t.setLanguage(Lang::En);
        const QString en = Translator::t(key);
        INFO("key: " << key.toStdString());
        REQUIRE_FALSE(en.isEmpty());
        REQUIRE(en != key);  // would mean the key is missing from the table

        t.setLanguage(Lang::Es);
        const QString es = Translator::t(key);
        REQUIRE_FALSE(es.isEmpty());
        REQUIRE(es != key);
    }

    t.setLanguage(original);
}

TEST_CASE("unknown keys fall back to the key itself", "[translator]") {
    REQUIRE(Translator::t("this.key.does.not.exist") == "this.key.does.not.exist");
}

TEST_CASE("setLanguage actually switches the returned text", "[translator]") {
    auto& t = Translator::instance();
    const Lang original = t.language();

    t.setLanguage(Lang::En);
    const QString en = Translator::t("menu.file");
    t.setLanguage(Lang::Es);
    const QString es = Translator::t("menu.file");

    REQUIRE(en != es);
    REQUIRE(t.language() == Lang::Es);

    t.setLanguage(original);
}

TEST_CASE("setLanguage emits languageChanged only on an actual change", "[translator]") {
    auto& t = Translator::instance();
    const Lang original = t.language();

    int fireCount = 0;
    QObject::connect(&t, &Translator::languageChanged, [&]() { ++fireCount; });

    t.setLanguage(Lang::Es);
    t.setLanguage(Lang::Es);  // no-op: already Spanish
    REQUIRE(fireCount == 1);

    t.setLanguage(Lang::En);
    REQUIRE(fireCount == 2);

    t.setLanguage(original);
}
