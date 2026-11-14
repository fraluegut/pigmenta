#include "Translator.h"

#include <QSettings>
#include <QLocale>
#include <QHash>
#include <QPair>

namespace pigmenta::desktop {

namespace {
constexpr const char* kSettingsKey = "app/language";

const QHash<QString, QPair<QString, QString>>& table() {
    static const QHash<QString, QPair<QString, QString>> t = {
        // App chrome / menu
        {"menu.file", {"&File", "&Archivo"}},
        {"menu.openImage", {"&Open image...", "&Abrir imagen..."}},
        {"menu.openSample", {"Open &sample", "Abrir &ejemplo"}},
        {"menu.exportImage", {"&Export image...", "&Exportar imagen..."}},
        {"menu.exit", {"E&xit", "&Salir"}},
        {"menu.view", {"&View", "&Ver"}},
        {"menu.help", {"&Help", "&Ayuda"}},
        {"menu.documentation", {"&Documentation", "&Documentación"}},
        {"menu.about", {"&About Pigmenta", "&Acerca de Pigmenta"}},

        {"mode.after", {"After", "Después"}},
        {"mode.before", {"Before", "Antes"}},
        {"mode.wipe", {"Wipe", "Cortina"}},
        {"mode.sideBySide", {"Side by side", "Lado a lado"}},

        {"sidebar.exportButton", {"Export image", "Exportar imagen"}},
        {"status.noImage", {"No image loaded", "Sin imagen cargada"}},
        {"status.failedLoad", {"Failed to load: ", "Error al cargar: "}},
        {"status.failedExport", {"Failed to export", "Error al exportar"}},
        {"status.exported", {"Exported: ", "Exportado: "}},
        {"dialog.openImage", {"Open image", "Abrir imagen"}},
        {"dialog.exportImage", {"Export image", "Exportar imagen"}},
        {"dialog.imageFilter",
         {"Images (*.png *.jpg *.jpeg *.bmp *.tif *.tiff)",
          "Imágenes (*.png *.jpg *.jpeg *.bmp *.tif *.tiff)"}},

        // DropArea
        {"dropArea.text",
         {"Drop image here\nor click to browse", "Suelta la imagen aquí\no haz clic para buscar"}},

        // CompareView
        {"compareView.empty", {"Drop an image to begin", "Suelta una imagen para empezar"}},

        // FilterPanel
        {"filterPanel.filters", {"FILTERS", "FILTROS"}},
        {"filterPanel.parameters", {"PARAMETERS", "PARÁMETROS"}},

        // Engine / filter specs
        {"filter.original", {"Original", "Original"}},
        {"filter.decorrelation", {"Decorrelation Stretch", "Decorrelation Stretch"}},
        {"filter.clahe", {"CLAHE (local contrast)", "CLAHE (contraste local)"}},
        {"filter.gamma", {"Gamma", "Gamma"}},
        {"filter.saturation", {"Saturation", "Saturación"}},
        {"filter.relief", {"Relief Shade", "Relieve"}},
        {"filter.irSimulate", {"IR Simulate", "Simulación IR"}},

        {"param.colorspace", {"Color space", "Espacio de color"}},
        {"param.targetStd", {"Target std.", "Desv. objetivo"}},
        {"param.intensity", {"Intensity", "Intensidad"}},
        {"param.tileSize", {"Tile size", "Tamaño de tesela"}},
        {"param.clipLimit", {"Clip limit", "Límite de recorte"}},
        {"param.gamma", {"Gamma", "Gamma"}},
        {"param.factor", {"Factor", "Factor"}},
        {"param.angle", {"Light angle", "Ángulo de luz"}},
        {"param.strength", {"Strength", "Intensidad"}},

        // Welcome dialog
        {"welcome.title", {"Welcome to Pigmenta", "Bienvenido a Pigmenta"}},
        {"welcome.brand", {"PIGMENTA", "PIGMENTA"}},
        {"welcome.subtitle",
         {"Rock art & petroglyph image enhancement", "Mejora de imágenes de arte rupestre y petroglifos"}},
        {"welcome.blurb",
         {"Decorrelation stretch, CLAHE, relief shading and other DStretch-style "
          "filters, running on the same native engine used by the mobile app and "
          "the command-line tool. Drop an image to get started, or try one of "
          "the examples below.",
          "Decorrelation stretch, CLAHE, relieve y otros filtros estilo DStretch, "
          "corriendo sobre el mismo motor nativo que usan la app móvil y la "
          "herramienta de línea de comandos. Suelta una imagen para empezar, o "
          "prueba uno de los ejemplos de abajo."}},
        {"welcome.examples", {"EXAMPLES", "EJEMPLOS"}},
        {"welcome.about", {"ABOUT", "ACERCA DE"}},
        {"welcome.dontShowAgain", {"Don't show this again", "No mostrar esto de nuevo"}},
        {"welcome.getStarted", {"Get started", "Empezar"}},
        {"welcome.close", {"Close", "Cerrar"}},

        {"about.version", {"Version %1 — built %2", "Versión %1 — compilado %2"}},
        {"about.createdBy",
         {"Created by Francisco Javier Luengo Gutiérrez — ",
          "Creado por Francisco Javier Luengo Gutiérrez — "}},
        {"about.projectBy", {"A project by ", "Un proyecto de "}},
        {"about.license", {"License: GNU GPLv3", "Licencia: GNU GPLv3"}},
        {"about.source", {"Source, issues and contributions: ", "Código, incidencias y contribuciones: "}},
        {"about.credits",
         {"Example images are CC-licensed from Wikimedia Commons — ",
          "Las imágenes de ejemplo son CC de Wikimedia Commons — "}},
        {"about.creditsLink", {"credits", "créditos"}},

        {"sample.huashan1", {"Huashan, China", "Huashan, China"}},
        {"sample.zuojiang", {"Zuojiang, China", "Zuojiang, China"}},
        {"sample.thailand", {"Thailand cave", "Cueva en Tailandia"}},
        {"sample.paraguay", {"Lorito Picada, Paraguay", "Lorito Picada, Paraguay"}},

        // Docs dialog
        {"docs.title", {"Documentation", "Documentación"}},
        {"docs.overview", {"Overview", "Resumen"}},
        {"docs.architecture", {"Architecture", "Arquitectura"}},
        {"docs.building", {"Building", "Compilación"}},
        {"docs.notFound", {"Document not found.", "Documento no encontrado."}},
    };
    return t;
}
}  // namespace

Translator& Translator::instance() {
    static Translator instance;
    return instance;
}

Translator::Translator() {
    QSettings settings;
    if (settings.contains(kSettingsKey)) {
        m_lang = settings.value(kSettingsKey).toString() == "es" ? Lang::Es : Lang::En;
    } else {
        m_lang = QLocale::system().name().startsWith("es") ? Lang::Es : Lang::En;
    }
}

void Translator::setLanguage(Lang lang) {
    if (lang == m_lang)
        return;
    m_lang = lang;
    QSettings settings;
    settings.setValue(kSettingsKey, lang == Lang::Es ? "es" : "en");
    emit languageChanged();
}

QString Translator::t(const QString& key) {
    const auto& tbl = table();
    auto it = tbl.find(key);
    if (it == tbl.end())
        return key;
    return instance().language() == Lang::Es ? it->second : it->first;
}

}  // namespace pigmenta::desktop
